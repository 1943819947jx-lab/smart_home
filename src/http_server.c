#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <semaphore.h>
#include <cjson/cJSON.h>

extern sensor_data_t *sensor_shm_init(void);
extern sem_t *sensor_sem_init(void);
extern alert_t *alert_shm_init(void);

#define MAX_EVENTS 10
#define RECV_BUF   4096
#define RESP_BUF   16384

/* ---- 构造 HTTP 响应 ---- */
static void build_http_response(char *buf, size_t size,
                                 int status, const char *content_type,
                                 const char *body) {
    const char *status_text =
        (status == 200) ? "OK" :
        (status == 404) ? "Not Found" :
        (status == 500) ? "Internal Error" : "Unknown";
    snprintf(buf, size,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        status, status_text, content_type, strlen(body), body);
}

/* ---- 处理单个 HTTP 请求 ---- */
static void handle_http(int client_fd,
                         sensor_data_t *sensor_shm, sem_t *sem,
                         alert_t *alert_shm) {
    char raw[RECV_BUF];
    memset(raw, 0, sizeof(raw));
    read(client_fd, raw, sizeof(raw) - 1);

    char method[16] = {0}, path[256] = {0};
    sscanf(raw, "%15s %255s", method, path);

    char response[RESP_BUF];

    /* 路由: GET / 或 /index.html → 仪表盘页面 */
    if (strcmp(method, "GET") == 0 &&
        (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
        FILE *fp = fopen("web/dashboard.html", "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char *html = malloc(size + 1);
            fread(html, 1, size, fp);
            html[size] = '\0';
            fclose(fp);

            char header[256];
            snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n"
                "\r\n", size);
            write(client_fd, header, strlen(header));
            write(client_fd, html, size);
            free(html);
        } else {
            build_http_response(response, sizeof(response), 404,
                               "text/plain", "dashboard.html not found");
            write(client_fd, response, strlen(response));
        }
        close(client_fd);
        return;
    }

    /* 路由: GET /api/data → 最新传感器数据 JSON */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/data") == 0) {
        sem_wait(sem);
        int pos  = sensor_shm[BUFFER_SIZE - 1].lux;
        int last = (pos == 0) ? BUFFER_SIZE - 2 : pos - 1;
        sensor_data_t latest = sensor_shm[last];
        sem_post(sem);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "timestamp", (double)latest.timestamp);
        cJSON_AddNumberToObject(root, "temp", latest.temp);
        cJSON_AddNumberToObject(root, "humidity", latest.humidity);
        cJSON_AddNumberToObject(root, "lux", latest.lux);

        char *json_str = cJSON_PrintUnformatted(root);
        build_http_response(response, sizeof(response), 200,
                           "application/json; charset=utf-8", json_str);
        free(json_str);
        cJSON_Delete(root);

        write(client_fd, response, strlen(response));
        close(client_fd);
        return;
    }

    /* 路由: GET /api/alerts → 告警列表 JSON */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/alerts") == 0) {
        cJSON *arr = cJSON_CreateArray();
        for (int i = 0; i < MAX_ALERTS; i++) {
            if (alert_shm[i].timestamp == 0) continue;
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "timestamp", (double)alert_shm[i].timestamp);
            cJSON_AddNumberToObject(item, "type", alert_shm[i].type);
            cJSON_AddNumberToObject(item, "value", alert_shm[i].value);
            cJSON_AddStringToObject(item, "message", alert_shm[i].message);
            cJSON_AddItemToArray(arr, item);
        }

        char *json_str = cJSON_PrintUnformatted(arr);
        build_http_response(response, sizeof(response), 200,
                           "application/json; charset=utf-8", json_str);
        free(json_str);
        cJSON_Delete(arr);

        write(client_fd, response, strlen(response));
        close(client_fd);
        return;
    }

    /* 其他: 404 */
    build_http_response(response, sizeof(response), 404,
                       "text/plain; charset=utf-8", "404 Not Found");
    write(client_fd, response, strlen(response));
    close(client_fd);
}

/* ---- HTTP 服务线程主函数 ---- */
void *http_server_thread(void *arg) {
    int *keep_running = (int *)arg;

    sensor_data_t *sensor_shm = sensor_shm_init();
    sem_t *sem = sensor_sem_init();
    alert_t *alert_shm = alert_shm_init();

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(HTTP_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 5);

    int epfd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    printf("[HTTP] 服务器启动，端口 %d\n", HTTP_PORT);
    printf("[HTTP] 浏览器访问 http://localhost:%d\n", HTTP_PORT);

    while (*keep_running) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (nfds < 0) break;

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                int client = accept(listen_fd, NULL, NULL);
                if (client >= 0) {
                    ev.events = EPOLLIN;
                    ev.data.fd = client;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
                }
            } else {
                handle_http(fd, sensor_shm, sem, alert_shm);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            }
        }
    }

    close(listen_fd);
    close(epfd);
    sem_close(sem);
    printf("[HTTP] 退出\n");
    return NULL;
}
