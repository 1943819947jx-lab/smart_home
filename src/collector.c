#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sqlite3.h>

extern int fifo_init_for_read(void);
extern sensor_data_t *sensor_shm_init(void);
extern sem_t *sensor_sem_init(void);

static int db_init(sqlite3 **db) {
    int rc = sqlite3_open("sensor.db", db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[采集] 打开数据库失败: %s\n", sqlite3_errmsg(*db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  temp      REAL NOT NULL,"
        "  humidity  REAL NOT NULL,"
        "  lux       INTEGER DEFAULT 0"
        ");";

    char *err = NULL;
    if (sqlite3_exec(*db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[采集] 建表失败: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

void *collector_thread(void *arg) {
    int *keep_running = (int *)arg;

    int fd = fifo_init_for_read();
    sensor_data_t *shm = sensor_shm_init();
    sem_t *sem = sensor_sem_init();
    sqlite3 *db = NULL;
    if (db_init(&db) != 0) return NULL;

    int write_pos = 0;
    printf("[采集] 启动，等待FIFO管道数据...\n");

    while (*keep_running) {
        sensor_data_t data;
        ssize_t n = read(fd, &data, sizeof(data));
        if (n <= 0) continue;

        /* 写入共享内存环形缓冲区（信号量保护） */
        sem_wait(sem);
        memcpy(&shm[write_pos], &data, sizeof(data));
        shm[BUFFER_SIZE - 1].lux = write_pos;
        write_pos = (write_pos + 1) % BUFFER_SIZE;
        sem_post(sem);

        /* 写入 SQLite 持久化 */
        const char *sql = "INSERT INTO sensor_data (temp, humidity, lux) VALUES (?, ?, ?);";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_double(stmt, 1, data.temp);
            sqlite3_bind_double(stmt, 2, data.humidity);
            sqlite3_bind_int(stmt, 3, data.lux);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_close(db);
    sem_close(sem);
    munmap(shm, sizeof(sensor_data_t) * BUFFER_SIZE);
    close(fd);

    printf("[采集] 退出\n");
    return NULL;
}
