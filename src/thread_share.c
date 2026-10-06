#include <stdio.h>
#include <pthread.h>

int shared_counter = 0;  // 所有线程共享

void *increment(void *arg) {
    for (int i = 0; i < 100000; i++) {
        shared_counter++;  // 危险！无锁竞争
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("预期: 200000, 实际: %d\n", shared_counter);
    // 结果大概率小于200000 — 这就是竞争条件
    return 0;
}
