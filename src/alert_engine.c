#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

extern sensor_data_t *sensor_shm_init(void);
extern sem_t *sensor_sem_init(void);
extern alert_t *alert_shm_init(void);
extern void push_alert(alert_t *shm, const alert_t *alert);

void *alert_thread(void *arg) {
    int *keep_running = (int *)arg;

    sensor_data_t *sensor_shm = sensor_shm_init();
    alert_t *alert_shm = alert_shm_init();
    sem_t *sem = sensor_sem_init();

    printf("[告警] 启动，阈值: 温度>%.1f°C, 湿度<%.1f%%\n",
           ALERT_TEMP_HIGH, ALERT_HUM_LOW);

    while (*keep_running) {
        sem_wait(sem);
        int pos  = sensor_shm[BUFFER_SIZE - 1].lux;
        int last = (pos == 0) ? BUFFER_SIZE - 2 : pos - 1;
        sensor_data_t latest = sensor_shm[last];
        sem_post(sem);

        if (latest.timestamp == 0) {
            sleep(SAMPLE_INTERVAL);
            continue;
        }

        if (latest.temp > ALERT_TEMP_HIGH) {
            alert_t alert;
            alert.timestamp = time(NULL);
            alert.type   = 1;
            alert.value  = latest.temp;
            snprintf(alert.message, sizeof(alert.message),
                     "高温告警: %.1f°C (阈值: %.1f°C)",
                     latest.temp, ALERT_TEMP_HIGH);
            push_alert(alert_shm, &alert);
            printf("[告警] ⚠️  %s\n", alert.message);
        }

        if (latest.humidity < ALERT_HUM_LOW) {
            alert_t alert;
            alert.timestamp = time(NULL);
            alert.type   = 2;
            alert.value  = latest.humidity;
            snprintf(alert.message, sizeof(alert.message),
                     "低湿告警: %.1f%% (阈值: %.1f%%)",
                     latest.humidity, ALERT_HUM_LOW);
            push_alert(alert_shm, &alert);
            printf("[告警] ⚠️  %s\n", alert.message);
        }

        sleep(SAMPLE_INTERVAL);
    }

    sem_close(sem);
    printf("[告警] 退出\n");
    return NULL;
}
