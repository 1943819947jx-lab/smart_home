#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8888

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    const char *msg = "Hello from 客户端!";
    write(sock, msg, strlen(msg));

    char buf[1024] = {0};
    read(sock, buf, sizeof(buf) - 1);
    printf("服务器回复: %s\n", buf);

    close(sock);
    return 0;
}
