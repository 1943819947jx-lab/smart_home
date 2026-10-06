#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "dht11_reader.h"

extern int fifo_init_for_write(void);

void *sensor_simulator_thread(void *arg) {
    int *keep_running = (int *)arg;
    int fd = fifo_init_for_write();
    srand(time(NULL));

    printf("[模拟器] 启动（温湿度=DHT11真实传感器, 光照=随机模拟）\n");
    printf("[模拟器] 采样间隔=%d秒\n", SAMPLE_INTERVAL);

    while (*keep_running) {
        sensor_data_t data;
        data.timestamp = time(NULL);

        /* 尝试读取 DHT11 真实传感器 */
        float temp, hum;
        if (dht11_read(&temp, &hum) == 0) {
            data.temp     = temp;                      /* 真实温度 */
            data.humidity = hum;                       /* 真实湿度 */
            data.lux      = 5000 + (rand() % 5000);    /* 光照无传感器，随机模拟 */
        } else {
            /* DHT11 读失败 → 使用默认值 */
            fprintf(stderr, "[模拟器] DHT11 读取失败，使用默认值\n");
            data.temp     = 25.0f;
            data.humidity = 50.0f;
            data.lux      = 5000;
        }

        if (write(fd, &data, sizeof(data)) == -1) {
            perror("[模拟器] write FIFO 失败");
        }

        sleep(SAMPLE_INTERVAL);
    }

    close(fd);
    printf("[模拟器] 已退出\n");
    return NULL;
}
