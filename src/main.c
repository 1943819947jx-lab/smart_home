#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "dht11_reader.h"

extern void *sensor_simulator_thread(void *arg);
extern void *collector_thread(void *arg);
extern void *alert_thread(void *arg);
extern void *http_server_thread(void *arg);
extern void *logger_thread(void *arg);

static volatile int keep_running = 1;

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        keep_running = 0;
        printf("\n[主线程] 收到退出信号，正在优雅关闭...\n");
    }
}

int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
dht11_init(); 

    pthread_t sim_tid, col_tid, alert_tid, http_tid, log_tid;

    pthread_create(&sim_tid,   NULL, sensor_simulator_thread, (void *)&keep_running);
    pthread_create(&col_tid,   NULL, collector_thread,        (void *)&keep_running);
    pthread_create(&alert_tid, NULL, alert_thread,            (void *)&keep_running);
    pthread_create(&http_tid,  NULL, http_server_thread,      (void *)&keep_running);
    pthread_create(&log_tid,   NULL, logger_thread,           (void *)&keep_running);

    printf("========================================\n");
    printf(" 智能家居数据采集与监控系统\n");
    printf(" 端口: %d\n", HTTP_PORT);
    printf(" 浏览器访问 http://localhost:%d\n", HTTP_PORT);
    printf(" 按 Ctrl+C 退出\n");
    printf("========================================\n");

    pthread_join(sim_tid, NULL);
    pthread_join(col_tid, NULL);
    pthread_join(alert_tid, NULL);
    pthread_join(http_tid, NULL);
    pthread_join(log_tid, NULL);

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    unlink(FIFO_NAME);
    shm_unlink(ALERT_SHM_NAME);
    sem_unlink(ALERT_SEM_NAME);

    printf("[主线程] 系统已安全退出，资源已清理\n");
dht11_cleanup();     

return 0;
}
