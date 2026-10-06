#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <cjson/cJSON.h>
#include <arpa/inet.h>

#define PORT 8080
#define DB_NAME "sensor.db"

// ========== 数据库初始化 ==========
sqlite3 *db_init() {
    sqlite3 *db;
    if (sqlite3_open(DB_NAME, &db) != SQLITE_OK) {
        fprintf(stderr, "打开数据库失败: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  temp      REAL NOT NULL,"
        "  humidity  REAL NOT NULL"
        ");";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "建表失败: %s\n", err);
        sqlite3_free(err);
        exit(1);
    }
    printf("[DB] 数据库初始化完成\n");
    return db;
}

// ========== HTTP 响应构建 ==========
void build_response(char *buf, size_t size,
                    int status, const char *content_type,
                    const char *body) {
    const char *text = (status == 200) ? "OK" :
                       (status == 400) ? "Bad Request" :
                       (status == 404) ? "Not Found" :
                       (status == 500) ? "Internal Server Error" : "OK";
    snprintf(buf, size,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        status, text, content_type, strlen(body), body);
}

// ========== GET /api/latest ==========
cJSON *get_latest_record(sqlite3 *db) {
    const char *sql =
        "SELECT id, timestamp, temp, humidity "
        "FROM sensor_data ORDER BY id DESC LIMIT 1;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "error", "数据库查询失败");
        return err;
    }

    cJSON *result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = cJSON_CreateObject();
        cJSON_AddNumberToObject(result, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(result, "time", (const char *)sqlite3_column_text(stmt, 1));
        cJSON_AddNumberToObject(result, "temp", sqlite3_column_double(stmt, 2));
        cJSON_AddNumberToObject(result, "humidity", sqlite3_column_double(stmt, 3));
    } else {
        result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "error", "暂无数据");
    }

    sqlite3_finalize(stmt);
    return result;
}

// ========== POST /api/sensor ==========
int insert_sensor_data(sqlite3 *db, double temp, double humidity) {
    const char *sql =
        "INSERT INTO sensor_data (temp, humidity) VALUES (?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_double(stmt, 1, temp);
    sqlite3_bind_double(stmt, 2, humidity);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ========== 路由分发 ==========
void handle_request(int client_fd, sqlite3 *db) {
    char raw[4096] = {0};
    ssize_t n = read(client_fd, raw, sizeof(raw) - 1);
    if (n <= 0) { close(client_fd); return; }
    raw[n] = '\0';

    char method[16] = {0}, path[256] = {0};
    sscanf(raw, "%15s %255s", method, path);

    printf("[REQ] %s %s\n", method, path);  // 调试打印

    char response[8192] = {0};

    // ─── 路由1: GET / → 返回测试页面 ───
    if (strcmp(method, "GET") == 0 &&
        (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
        const char *html =
            "<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<title>REST API Test</title></head><body>"
            "<h2>Smart Home — REST API 测试</h2>"
            "<button onclick=\"fetch('/api/latest').then(r=>r.json()).then(j=>"
            "document.getElementById('out').textContent=JSON.stringify(j,null,2))\">"
            "GET /api/latest</button>"
            "<pre id='out' style='background:#f4f4f4;padding:10px;'>点击按钮查看</pre>"
            "</body></html>";
        build_response(response, sizeof(response), 200,
                       "text/html; charset=utf-8", html);
    }

    // ─── 路由2: GET /api/latest → 返回最新记录JSON ───
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/latest") == 0) {
        cJSON *json = get_latest_record(db);
        char *json_str = cJSON_Print(json);
        build_response(response, sizeof(response), 200,
                       "application/json; charset=utf-8", json_str);
        free(json_str);
        cJSON_Delete(json);
    }

    // ─── 路由3: POST /api/sensor → 写入数据库 ───
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/sensor") == 0) {
        char *body = strstr(raw, "\r\n\r\n");
        if (!body) {
            build_response(response, sizeof(response), 400,
                           "application/json; charset=utf-8",
                           "{\"error\":\"Bad Request\"}");
        } else {
            body += 4;

            cJSON *json = cJSON_Parse(body);
            if (!json) {
                printf("[ERR] JSON解析失败, body=%.200s\n", body);
                build_response(response, sizeof(response), 400,
                               "application/json; charset=utf-8",
                               "{\"error\":\"无效JSON\"}");
            } else {
                cJSON *t = cJSON_GetObjectItem(json, "temp");
                cJSON *h = cJSON_GetObjectItem(json, "humidity");

                if (!t || !h) {
                    build_response(response, sizeof(response), 400,
                                   "application/json; charset=utf-8",
                                   "{\"error\":\"缺少 temp 或 humidity 字段\"}");
                } else {
                    double temp = t->valuedouble;
                    double hum  = h->valuedouble;

                    if (insert_sensor_data(db, temp, hum) == 0) {
                        printf("[DB] 写入成功: temp=%.1f, humidity=%.1f\n", temp, hum);
                        build_response(response, sizeof(response), 200,
                                       "application/json; charset=utf-8",
                                       "{\"status\":\"ok\"}");
                    } else {
                        build_response(response, sizeof(response), 500,
                                       "application/json; charset=utf-8",
                                       "{\"error\":\"数据库写入失败\"}");
                    }
                }
                cJSON_Delete(json);
            }
        }
    }

    // ─── 其他路由 → 404 ───
    else {
        printf("[404] %s %s 未匹配\n", method, path);
        build_response(response, sizeof(response), 404,
                       "text/plain; charset=utf-8", "404 Not Found");
    }

    write(client_fd, response, strlen(response));
    close(client_fd);
    printf("[DONE] 响应已发送, %s %s\n", method, path);
}

// ========== 主函数 ==========
int main() {
    sqlite3 *db = db_init();

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(listen_fd, 5) < 0) {
        perror("listen"); exit(1);
    }

    printf("========================================\n");
    printf(" REST API 服务器已启动\n");
    printf(" 端口: %d\n", PORT);
    printf(" 测试页面: http://localhost:%d/\n", PORT);
    printf(" 最新数据: http://localhost:%d/api/latest\n", PORT);
    printf(" POST接口: http://localhost:%d/api/sensor\n", PORT);
    printf(" 按 Ctrl+C 退出\n");
    printf("========================================\n");

    while (1) {
        int client = accept(listen_fd, NULL, NULL);
        if (client < 0) { perror("accept"); continue; }
        handle_request(client, db);
    }

    sqlite3_close(db);
    close(listen_fd);
    return 0;
}
