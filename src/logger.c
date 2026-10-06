#include "config.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "smart_home.log"

void *logger_thread(void *arg) {
    int *keep_running = (int *)arg;

    printf("[日志] 启动，日志文件: %s\n", LOG_FILE);

    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "[%s] ===== 系统启动 =====\n", ctime(&now));
        fclose(fp);
    }

    while (*keep_running) {
        sleep(10);
        fp = fopen(LOG_FILE, "a");
        if (fp) {
            time_t now = time(NULL);
            fprintf(fp, "[%s] 心跳: 系统运行中...\n", ctime(&now));
            fclose(fp);
        }
    }

    fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "[%s] ===== 系统退出 =====\n", ctime(&now));
        fclose(fp);
    }

    printf("[日志] 退出\n");
    return NULL;
}
