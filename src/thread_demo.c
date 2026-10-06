#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 3

// 线程要执行的函数
void *worker(void *arg) {
    int id = *(int *)arg;
    printf("[线程 %d] ID = %lu, 开始工作\n", id, pthread_self());
    sleep(1);
    printf("[线程 %d] 工作完成\n", id);
    return (void *)(long)(id * 100);  // 返回值
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int ids[THREAD_COUNT];

    // 创建3个线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
        printf("[主线程] 创建了线程 %d\n", i + 1);
    }

    // 等待所有线程结束，获取返回值
    for (int i = 0; i < THREAD_COUNT; i++) {
        void *ret;
        pthread_join(threads[i], &ret);
        printf("[主线程] 线程 %d 返回: %ld\n", i + 1, (long)ret);
    }

    printf("[主线程] 全部结束\n");
    return 0;
}
