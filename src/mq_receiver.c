#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>

#define MQ_NAME "/sensor_mq"

int main() {
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY);
    if (mq == (mqd_t)-1) { perror("mq_open"); exit(1); }

    char buf[256];
    for (int i = 0; i < 5; i++) {
        ssize_t n = mq_receive(mq, buf, sizeof(buf), NULL);
        if (n > 0) {
            printf("收到: %s\n", buf);
        }
    }

    mq_close(mq);
    mq_unlink(MQ_NAME);  // 删除消息队列
    return 0;
}
