#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
#endif

#ifndef NO_CJSON
    #include <cjson/cJSON.h>
#endif

#define PORT        8080
#define BUF_SIZE    4096
#define RESP_SIZE   8192
#define BACKLOG     10

void parse_request(const char *raw_request, char *method, char *path, char *version) {
    sscanf(raw_request, "%15s %255s %15s", method, path, version);
}

void build_response(char *buf, size_t size,
                    int status_code, const char *content_type,
                    const char *body) {
    const char *status_text;
    if (status_code == 200) {
        status_text = "OK";
    } else if (status_code == 404) {
        status_text = "Not Found";
    } else {
        status_text = "Error";
    }

    snprintf(buf, size,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_code, status_text,
        content_type, strlen(body),
        body);
}

void handle_request(SOCKET client_fd) {
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    read(client_fd, buf, sizeof(buf) - 1);

    char method[16] = {0};
    char path[256]   = {0};
    sscanf(buf, "%15s %255s", method, path);

    printf("-> request: %s %s\n", method, path);

    char response[RESP_SIZE];
    memset(response, 0, sizeof(response));

    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        const char *html =
            "<!DOCTYPE html>\n"
            "<html lang=\"zh-CN\">\n"
            "<head>\n"
            "    <meta charset=\"UTF-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            "    <title>Smart Home Dashboard</title>\n"
            "    <style>\n"
            "        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
            "        body {\n"
            "            font-family: 'Segoe UI', Arial, sans-serif;\n"
            "            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);\n"
            "            min-height: 100vh; color: #eee;\n"
            "            display: flex; flex-direction: column; align-items: center;\n"
            "        }\n"
            "        h1 { margin: 40px 0 10px; font-size: 28px; color: #e94560; }\n"
            "        .subtitle { color: #888; margin-bottom: 40px; font-size: 14px; }\n"
            "        .cards {\n"
            "            display: flex; gap: 24px; flex-wrap: wrap;\n"
            "            justify-content: center; max-width: 900px;\n"
            "        }\n"
            "        .card {\n"
            "            background: #1f1f3a;\n"
            "            border-radius: 16px; padding: 28px 36px;\n"
            "            min-width: 220px; text-align: center;\n"
            "            box-shadow: 0 8px 32px rgba(0,0,0,0.4);\n"
            "            border: 1px solid #2a2a4a;\n"
            "            transition: transform 0.2s;\n"
            "        }\n"
            "        .card:hover { transform: translateY(-4px); }\n"
            "        .card .icon { font-size: 40px; margin-bottom: 12px; }\n"
            "        .card .label { color: #888; font-size: 13px; text-transform: uppercase; letter-spacing: 2px; }\n"
            "        .card .value { font-size: 36px; font-weight: bold; margin: 10px 0; }\n"
            "        .card .unit { font-size: 14px; color: #666; }\n"
            "        .temp .value { color: #ff6b6b; }\n"
            "        .humidity .value { color: #48dbfb; }\n"
            "        .status .value { color: #0be881; }\n"
            "        .api-link {\n"
            "            margin-top: 40px; padding: 12px 24px;\n"
            "            background: #e94560; color: #fff; border-radius: 8px;\n"
            "            text-decoration: none; font-weight: bold;\n"
            "            transition: background 0.2s;\n"
            "        }\n"
            "        .api-link:hover { background: #c23152; }\n"
            "        footer { margin-top: 60px; color: #555; font-size: 12px; }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "    <h1>Smart Home Dashboard</h1>\n"
            "    <p class=\"subtitle\">HTTP Server Demo - Task 15</p>\n"
            "\n"
            "    <div class=\"cards\">\n"
            "        <div class=\"card temp\">\n"
            "            <div class=\"icon\">T</div>\n"
            "            <div class=\"label\">Temperature</div>\n"
            "            <div class=\"value\">25.5</div>\n"
            "            <div class=\"unit\">C</div>\n"
            "        </div>\n"
            "        <div class=\"card humidity\">\n"
            "            <div class=\"icon\">H</div>\n"
            "            <div class=\"label\">Humidity</div>\n"
            "            <div class=\"value\">60.2</div>\n"
            "            <div class=\"unit\">%RH</div>\n"
            "        </div>\n"
            "        <div class=\"card status\">\n"
            "            <div class=\"icon\">S</div>\n"
            "            <div class=\"label\">System Status</div>\n"
            "            <div class=\"value\">OK</div>\n"
            "            <div class=\"unit\">All normal</div>\n"
            "        </div>\n"
            "    </div>\n"
            "\n"
            "    <a class=\"api-link\" href=\"/api/data\" target=\"_blank\">"
            "JSON API</a>\n"
            "\n"
            "    <footer>Embedded HTTP Server Port 8080</footer>\n"
            "</body>\n"
            "</html>";

        build_response(response, sizeof(response), 200,
                       "text/html; charset=utf-8", html);
    }
    else if (strcmp(path, "/api/data") == 0) {
#ifdef NO_CJSON
        const char *json =
            "{\n"
            "  \"temp\": 25.5,\n"
            "  \"humidity\": 60.2,\n"
            "  \"status\": \"OK\",\n"
            "  \"uptime\": 86400\n"
            "}";
        build_response(response, sizeof(response), 200,
                       "application/json; charset=utf-8", json);
#else
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "temp", 25.5);
        cJSON_AddNumberToObject(root, "humidity", 60.2);
        cJSON_AddStringToObject(root, "status", "OK");
        cJSON_AddNumberToObject(root, "uptime", 86400);

        char *json_str = cJSON_Print(root);
        build_response(response, sizeof(response), 200,
                       "application/json; charset=utf-8", json_str);

        free(json_str);
        cJSON_Delete(root);
#endif
    }
    else {
        const char *not_found = "404 - Page Not Found\n";
        build_response(response, sizeof(response), 404,
                       "text/plain; charset=utf-8", not_found);
    }

    write(client_fd, response, strlen(response));
}

int main() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("socket() failed\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    printf("socket created\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind() failed (port %d)\n", PORT);
        goto cleanup;
    }
    printf("bind port %d OK\n", PORT);

    if (listen(server_fd, BACKLOG) < 0) {
        printf("listen() failed\n");
        goto cleanup;
    }
    printf("listening...\n\n");
    printf("HTTP Server started at http://localhost:%d\n", PORT);
    printf("API endpoint: http://localhost:%d/api/data\n", PORT);
    printf("Press Ctrl+C to stop\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd,
                                  (struct sockaddr *)&client_addr,
                                  &addr_len);

        if (client_fd == INVALID_SOCKET) {
            printf("accept() failed\n");
            continue;
        }

        printf("new connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        handle_request(client_fd);

#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }

cleanup:
#ifdef _WIN32
    if (server_fd != INVALID_SOCKET) closesocket(server_fd);
    WSACleanup();
#else
    if (server_fd >= 0) close(server_fd);
#endif
    return 0;
}
