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
    int fd = shm_open(SHM_NAME, O_RDONLY, 0644);
    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    printf("读到: %s\n", shm_ptr);
    munmap(shm_ptr, SHM_SIZE);
    close(fd);
    return 0;
}
