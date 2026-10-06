#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#define SEM_NAME "/sensor_sem"

int main() {
    // 创建信号量，初始值1（互斥信号量）
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) { perror("sem_open"); exit(1); }

    sem_wait(sem);    // P操作：获取锁
    // ... 临界区：读写共享内存 ...
    sem_post(sem);    // V操作：释放锁

    sem_close(sem);
    sem_unlink(SEM_NAME);
    return 0;
}
