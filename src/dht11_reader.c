/*
 * DHT11 用户态读取 — 基于 /dev/mem 直写 GPIO 寄存器
 * 正点原子阿尔法 IMX6ULL 适配
 * GPIO1 基址 = 0x0209C000, IO01 = bit 1
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

/* GPIO1 寄存器 */
#define GPIO1_BASE      0x0209C000
#define GPIO_DR         0x00    /* 数据寄存器 */
#define GPIO_GDIR       0x04    /* 方向寄存器 */
#define GPIO_PSR        0x08    /* 引脚状态寄存器 */

#define DHT11_PIN       1       /* GPIO1_IO01 = bit 1 */

static volatile unsigned int *gpio1_base = NULL;
static int mem_fd = -1;

/* ====== 寄存器位操作 ====== */
static void gpio_dir_out(void) {
    gpio1_base[GPIO_GDIR / 4] |= (1 << DHT11_PIN);
}

static void gpio_dir_in(void) {
    gpio1_base[GPIO_GDIR / 4] &= ~(1 << DHT11_PIN);
}

static void gpio_write(int val) {
    if (val)
        gpio1_base[GPIO_DR / 4] |= (1 << DHT11_PIN);
    else
        gpio1_base[GPIO_DR / 4] &= ~(1 << DHT11_PIN);
}

static int gpio_read(void) {
    return (gpio1_base[GPIO_PSR / 4] >> DHT11_PIN) & 1;
}

/* 微秒延时（systick 循环） */
static void delay_us(int us) {
    volatile int i;
    for (; us > 0; us--) {
        for (i = 0; i < 50; i++);  /* 调整循环次数匹配实际 CPU 频率 */
    }
}

/* ====== DHT11 协议 ====== */
static int dht11_start(void)
{
    /* ① 拉低 18ms */
    gpio_dir_out();
    gpio_write(0);
    usleep(18000);

    /* ② 拉高 30us */
    gpio_write(1);
    delay_us(30);

    /* ③ 切换输入，等 DHT11 回应 */
    gpio_dir_in();

    /* 等 DHT11 拉低（最多等 100us） */
    int timeout = 0;
    while (gpio_read() == 1) {
        delay_us(1);
        if (++timeout > 100) return -1;
    }

    /* 等 DHT11 拉高 */
    timeout = 0;
    while (gpio_read() == 0) {
        delay_us(1);
        if (++timeout > 100) return -2;
    }

    /* 等 DHT11 再次拉低 */
    timeout = 0;
    while (gpio_read() == 1) {
        delay_us(1);
        if (++timeout > 100) return -3;
    }

    return 0;
}

static unsigned char dht11_read_byte(void)
{
    unsigned char data = 0;

    for (int i = 0; i < 8; i++) {
        /* 等低电平结束 */
        int timeout = 0;
        while (gpio_read() == 0) {
            delay_us(1);
            if (++timeout > 100) return 0;
        }

        /* 延时 40us 后采样 */
        delay_us(40);

        if (gpio_read() == 1) {
            data |= (1 << (7 - i));

            /* 等高电平结束 */
            timeout = 0;
            while (gpio_read() == 1) {
                delay_us(1);
                if (++timeout > 100) break;
            }
        }
    }
    return data;
}

int dht11_read(float *temp, float *humidity)
{
    unsigned char data[5] = {0};

    if (dht11_start() != 0) {
        fprintf(stderr, "[DHT11] 传感器未响应，检查接线\n");
        return -1;
    }

    for (int i = 0; i < 5; i++) {
        data[i] = dht11_read_byte();
    }

    unsigned char checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        fprintf(stderr, "[DHT11] 校验失败: calc=%d, recv=%d\n",
                checksum, data[4]);
        return -1;
    }

    *humidity = data[0] + data[1] * 0.1f;
    *temp     = data[2] + data[3] * 0.1f;

    return 0;
}

int dht11_init(void)
{
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("/dev/mem open");
        return -1;
    }

    gpio1_base = (volatile unsigned int *)mmap(
        NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED,
        mem_fd, GPIO1_BASE
    );
    if (gpio1_base == MAP_FAILED) {
        perror("mmap");
        close(mem_fd);
        return -1;
    }

    printf("[DHT11] GPIO1 寄存器已映射 (mmap), IO01 就绪\n");
    usleep(100000);  /* 等传感器上电稳定 */
    return 0;
}

void dht11_cleanup(void)
{
    if (gpio1_base && gpio1_base != MAP_FAILED)
        munmap((void *)gpio1_base, 0x1000);
    if (mem_fd >= 0)
        close(mem_fd);
}

/* ====== 测试入口 ====== */
#ifdef DHT11_TEST
int main(void)
{
    if (dht11_init() != 0) {
        fprintf(stderr, "DHT11 初始化失败 (需要 root 权限访问 /dev/mem)\n");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        float temp, hum;
        if (dht11_read(&temp, &hum) == 0) {
            printf("[DHT11] 温度=%.1f°C  湿度=%.1f%%\n", temp, hum);
        }
        sleep(2);
    }

    dht11_cleanup();
    return 0;
}
#endif
