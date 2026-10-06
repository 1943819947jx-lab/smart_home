#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_NAME "/sensor_shm"
#define SHM_SIZE 4096

int main() {
    // 创建共享内存
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (fd < 0) { perror("shm_open"); exit(1); }

    // 设置大小
    ftruncate(fd, SHM_SIZE);

    // 映射到进程地址空间
    char *shm_ptr = mmap(NULL, SHM_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) { perror("mmap"); exit(1); }

    // 写入数据
    strcpy(shm_ptr, "温度:25.5C,湿度:60%,光照:8000lux");
    printf("写入了: %s\n", shm_ptr);

    // 清理
    munmap(shm_ptr, SHM_SIZE);
    close(fd);
    // shm_unlink(SHM_NAME);  // 删除共享内存
    return 0;
}
