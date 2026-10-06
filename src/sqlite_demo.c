#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

// ========== 新增：Step3 回调函数，放在 main 外面 ==========
// sqlite3_exec 的回调函数：每查到一行数据，就会调用一次
static int callback(void *data, int argc, char **argv, char **col_names) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s  ", col_names[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main() {
    sqlite3 *db;
    char *err_msg = NULL;

    // 1. 打开数据库（不存在则创建）
    int rc = sqlite3_open("sensor.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 2. 建表
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  temp      REAL,"
        "  humidity  REAL,"
        "  lux       INTEGER"
        ");";
    rc = sqlite3_exec(db, create_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "建表失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    // 3. 插入数据（参数化查询，防SQL注入）
    const char *insert_sql =
        "INSERT INTO sensor_data (temp, humidity, lux) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);

    float temps[] = {25.5, 26.0, 27.3};
    float hums[]  = {60.2, 58.7, 55.1};
    int   luxes[] = {8000, 7500, 9000};

    for (int i = 0; i < 3; i++) {
        sqlite3_bind_double(stmt, 1, temps[i]);
        sqlite3_bind_double(stmt, 2, hums[i]);
        sqlite3_bind_int(stmt, 3, luxes[i]);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
        printf("插入第 %d 条数据\n", i + 1);
    }
    sqlite3_finalize(stmt);

    // 4. 查询数据 —— 修改：第三个参数传 callback 函数名
    printf("\n===== 查询结果 =====\n");
    const char *query_sql = "SELECT * FROM sensor_data ORDER BY id DESC LIMIT 5;";
    rc = sqlite3_exec(db, query_sql, callback, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "查询失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
    printf("\n数据库操作完成\n");
    return 0;
}
