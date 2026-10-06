#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>

/* ====== IPC 命名 ====== */
#define SHM_NAME        "/sensor_shm"    // 传感器数据共享内存
#define SEM_NAME        "/sensor_sem"     // 保护传感器数据的信号量
#define FIFO_NAME      "/sensor_fifo"    // 模拟器→采集的FIFO管道
#define ALERT_SHM_NAME  "/alert_shm"     // 告警数据共享内存
#define ALERT_SEM_NAME  "/alert_sem"     // 保护告警数据的信号量

/* ====== 系统参数 ====== */
#define SAMPLE_INTERVAL 2      // 采样间隔(秒)
#define BUFFER_SIZE     3600   // 环形缓冲区容量(可存1小时数据)
#define HTTP_PORT       8080   // Web服务端口
#define MAX_ALERTS      100    // 告警列表最大条数

/* ====== 告警阈值 ====== */
#define ALERT_TEMP_HIGH 40.0   // 高温告警: >40°C
#define ALERT_HUM_LOW   20.0   // 低湿告警: <20%

/* ====== 数据结构 ====== */

// 一条传感器数据
typedef struct {
    time_t timestamp;
    float  temp;        // 温度 °C
    float  humidity;    // 湿度 %
    int    lux;         // 光照 lux
} sensor_data_t;

// 一条告警记录
typedef struct {
    time_t timestamp;
    int    type;        // 1=高温告警, 2=低湿告警
    float  value;       // 触发时的实际值
    char   message[128];// 告警描述文字
} alert_t;

#endif
