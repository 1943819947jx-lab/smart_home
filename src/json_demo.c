#include <stdio.h>
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

int main() {
    // 构造 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temp", 25.5);
    cJSON_AddNumberToObject(root, "humidity", 60.2);
    cJSON_AddNumberToObject(root, "lux", 8000);
    cJSON_AddStringToObject(root, "time", "2026-07-05 14:30:01");

    char *json_str = cJSON_Print(root);
    printf("构造的JSON: %s\n", json_str);

    // 解析 JSON
    cJSON *parsed = cJSON_Parse(json_str);
    if (parsed) {
        cJSON *temp = cJSON_GetObjectItem(parsed, "temp");
        cJSON *hum  = cJSON_GetObjectItem(parsed, "humidity");
        printf("解析: 温度=%.1f, 湿度=%.1f\n",
               temp->valuedouble, hum->valuedouble);
        cJSON_Delete(parsed);
    }

    cJSON_Delete(root);
    free(json_str);
    return 0;
}
