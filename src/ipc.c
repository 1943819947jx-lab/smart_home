#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <unistd.h>

/* ====== 传感器数据共享内存 ====== */
sensor_data_t *sensor_shm_init(void) {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (fd < 0) { perror("shm_open sensor"); exit(1); }
    ftruncate(fd, sizeof(sensor_data_t) * BUFFER_SIZE);
    sensor_data_t *ptr = mmap(NULL, sizeof(sensor_data_t) * BUFFER_SIZE,
                               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap sensor"); exit(1); }
    close(fd);
    memset(ptr, 0, sizeof(sensor_data_t) * BUFFER_SIZE);
    return ptr;
}

void sensor_shm_cleanup(sensor_data_t *ptr) {
    munmap(ptr, sizeof(sensor_data_t) * BUFFER_SIZE);
    shm_unlink(SHM_NAME);
}

/* ====== 信号量 ====== */
sem_t *sensor_sem_init(void) {
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); exit(1); }
    return sem;
}

void sensor_sem_cleanup(sem_t *sem) {
    sem_close(sem);
    sem_unlink(SEM_NAME);
}

/* ====== 消息队列 ====== */
int fifo_init_for_write(void) {
    mkfifo(FIFO_NAME, 0644);
    int fd = open(FIFO_NAME, O_RDWR);
    if (fd < 0) { perror("fifo open write"); exit(1); }
    return fd;
}

int fifo_init_for_read(void) {
        mkfifo(FIFO_NAME, 0644);
    int fd = open(FIFO_NAME, O_RDWR);
    if (fd < 0) { perror("fifo open read"); exit(1); }
    return fd;
}

void fifo_cleanup(int fd) {
    close(fd);
    unlink(FIFO_NAME);
}

/* ====== 告警共享内存 ====== */
alert_t *alert_shm_init(void) {
    size_t size = sizeof(alert_t) * (MAX_ALERTS + 1);
    int fd = shm_open(ALERT_SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (fd < 0) { perror("shm_open alert"); exit(1); }
    ftruncate(fd, size);
    alert_t *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) { perror("mmap alert"); exit(1); }
    close(fd);
    memset(ptr, 0, size);
    return ptr;
}

void push_alert(alert_t *shm, const alert_t *alert) {
    int *pos = (int *)&shm[MAX_ALERTS];
    memcpy(&shm[*pos], alert, sizeof(alert_t));
    *pos = (*pos + 1) % MAX_ALERTS;
}
