#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define PORT    8889
#define MAX_EVENTS 10

int main() {
    // 1. 创建监听socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 5);

    // 2. 创建 epoll 实例
    int epfd = epoll_create1(0);
    if (epfd < 0) { perror("epoll_create1"); exit(1); }

    // 3. 把监听socket加入epoll
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;          // 监听"可读"事件
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    printf("epoll服务器监听端口 %d ...\n", PORT);

    // 4. 事件循环
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) { perror("epoll_wait"); break; }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                // 新连接
                int client_fd = accept(listen_fd, NULL, NULL);
                printf("新客户端 fd=%d 已连接\n", client_fd);

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
            } else {
                // 客户端有数据
                char buf[256] = {0};
                ssize_t n = read(fd, buf, sizeof(buf) - 1);

                if (n <= 0) {
                    printf("客户端 fd=%d 已断开\n", fd);
                    close(fd);
                } else {
                    printf("fd=%d: %s\n", fd, buf);
                    write(fd, "OK\n", 3);
                }
            }
        }
    }

    close(listen_fd);
    close(epfd);
    return 0;
}
