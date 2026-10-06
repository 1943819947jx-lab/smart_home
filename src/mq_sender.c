#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#define MQ_NAME "/sensor_mq"

int main() {
    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = 10,     // 最多10条消息
        .mq_msgsize = 256,    // 每条最大256字节
        .mq_curmsgs = 0
    };

    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == (mqd_t)-1) { perror("mq_open"); exit(1); }

    for (int i = 0; i < 5; i++) {
        char msg[256];
        snprintf(msg, sizeof(msg), "传感器数据#%d: temp=%.1f, hum=%.1f",
                 i, 20.0 + i * 2.5, 50.0 + i * 3.0);
        if (mq_send(mq, msg, strlen(msg) + 1, 0) == 0) {
            printf("发送: %s\n", msg);
        }
        sleep(1);
    }

    mq_close(mq);
    return 0;
}
