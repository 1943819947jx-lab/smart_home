#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8888
#define BACKLOG 5

int main() {
    // 1. 创建 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    // 2. 设置地址复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. bind 绑定地址
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY  // 监听所有网卡
    };
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    // 4. listen 开始监听
    listen(server_fd, BACKLOG);
    printf("服务器监听端口 %d ...\n", PORT);

    // 5. accept 接受连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);
    printf("客户端 %s:%d 已连接\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    // 6. 收发数据
    char buf[1024];
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("收到: %s\n", buf);

    const char *reply = "服务器收到了！";
    write(client_fd, reply, strlen(reply));

    close(client_fd);
    close(server_fd);
    return 0;
}
