#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <源文件> <目标文件>\n", argv[0]);
        exit(1);
    }

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) { perror("打开源文件失败"); exit(1); }

    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) { perror("创建目标文件失败"); close(src_fd); exit(1); }

    char buf[BUFSIZE];
    ssize_t n;
    while ((n = read(src_fd, buf, BUFSIZE)) > 0) {
        write(dst_fd, buf, n);
    }

    close(src_fd);
    close(dst_fd);
    printf("复制完成: %s → %s\n", argv[1], argv[2]);
    return 0;
}
