#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
int count = 0;  // 缓冲区当前数据量

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_producer = PTHREAD_COND_INITIALIZER;  // "可以生产了"
pthread_cond_t  cond_consumer = PTHREAD_COND_INITIALIZER;  // "可以消费了"

void *producer(void *arg) {
    for (int i = 0; i < 20; i++) {
        pthread_mutex_lock(&mutex);

        // 缓冲区满 → 等待
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&cond_producer, &mutex);
        }

        buffer[count] = i;
        printf("[生产者] 生产了 %d (缓冲区: %d/%d)\n", i, count + 1, BUFFER_SIZE);
        count++;

        // 通知消费者
        pthread_cond_signal(&cond_consumer);
        pthread_mutex_unlock(&mutex);
        usleep(100000);  // 0.1秒
    }
    return NULL;
}

void *consumer(void *arg) {
    for (int i = 0; i < 20; i++) {
        pthread_mutex_lock(&mutex);

        // 缓冲区空 → 等待
        while (count == 0) {
            pthread_cond_wait(&cond_consumer, &mutex);
        }

        int item = buffer[count - 1];
        printf("[消费者] 消费了 %d (缓冲区: %d/%d)\n", item, count - 1, BUFFER_SIZE);
        count--;

        // 通知生产者
        pthread_cond_signal(&cond_producer);
        pthread_mutex_unlock(&mutex);
        usleep(150000);  // 0.15秒
    }
    return NULL;
}

int main() {
    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    printf("全部完成\n");
    return 0;
}
