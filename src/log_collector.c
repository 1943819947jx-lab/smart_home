#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MQ_NAME  "/log_mq"
#define MAX_MSG  10
#define MSG_SIZE 256

// 日志消息
typedef struct {
    int  thread_id;
    char content[240];
} log_msg_t;

void *collector(void *arg) {
    int id = *(int *)arg;
    mqd_t mq = mq_open(MQ_NAME, O_WRONLY);

    for (int i = 0; i < 10; i++) {
        log_msg_t msg;
        msg.thread_id = id;
        snprintf(msg.content, sizeof(msg.content),
                 "采集线程%d: 第%d条数据, 温度=%.1f",
                 id, i + 1, 20.0 + (rand() % 200) / 10.0);

        mq_send(mq, (char *)&msg, sizeof(msg), 0);
        usleep(500000);  // 0.5秒
    }
    mq_close(mq);
    return NULL;
}

void *aggregator(void *arg) {
    // 手动配置消息队列属性
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MSG;       // 队列最多缓存10条消息
    attr.mq_msgsize = sizeof(log_msg_t); // 单条消息的最大大小
    attr.mq_curmsgs = 0;

mqd_t mq = mq_open(MQ_NAME, O_RDONLY | O_CREAT, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("汇总线程 mq_open 失败");
        return NULL;
    }

    log_msg_t msg;
    int count[4] = {0};

    for (int i = 0; i < 30; i++) {
        // 接收缓冲区长度 = 队列配置的最大消息大小
        ssize_t recv_len = mq_receive(mq, (char *)&msg, sizeof(log_msg_t), NULL);
        if (recv_len == -1) {
            perror("mq_receive 接收失败");
            break;
        }
        count[msg.thread_id]++;
        printf("[汇总] 来自线程%d: %s (该线程已收%d条)\n",
               msg.thread_id, msg.content, count[msg.thread_id]);
    }

    mq_close(mq);
    mq_unlink(MQ_NAME);
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t collectors[3], agg;
    int ids[3] = {1, 2, 3};

    pthread_create(&agg, NULL, aggregator, NULL);
    sleep(1);  // 确保汇总线程的消息队列已创建

    for (int i = 0; i < 3; i++) {
        pthread_create(&collectors[i], NULL, collector, &ids[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(collectors[i], NULL);
    }
    pthread_join(agg, NULL);

    printf("日志采集完成\n");
    return 0;
}
