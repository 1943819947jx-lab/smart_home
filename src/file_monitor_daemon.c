#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define MONITOR_DIR  "/tmp/monitor"
#define LOG_FILE     "/tmp/file_monitor.log"
#define SCAN_INTERVAL 5

// 全局标志位，用于信号处理优雅退出
static volatile int keep_running = 1;

void sig_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        keep_running = 0;
    }
}

// 记录日志
void write_log(const char *msg) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;
    time_t now = time(NULL);
    fprintf(fp, "[%s] %s\n", ctime(&now), msg);
    fclose(fp);
}

// 守护进程化
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid > 0) exit(0);  // 父进程退出

    setsid();              // 新会话
    chdir("/");
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

int main() {
    daemonize();
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    // 确保监控目录存在
    mkdir(MONITOR_DIR, 0755);

    write_log("===== 文件监控守护进程启动 =====");

    // 记录当前已知文件
    char known_files[1000][256] = {0};
    int known_count = 0;

    while (keep_running) {
        DIR *dir = opendir(MONITOR_DIR);
        if (!dir) { sleep(SCAN_INTERVAL); continue; }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            // 检查是否是新文件
            int is_new = 1;
            for (int i = 0; i < known_count; i++) {
                if (strcmp(known_files[i], entry->d_name) == 0) {
                    is_new = 0;
                    break;
                }
            }

            if (is_new && known_count < 1000) {
                char fullpath[512];
                snprintf(fullpath, sizeof(fullpath), "%s/%s",
                         MONITOR_DIR, entry->d_name);

                struct stat st;
                if (stat(fullpath, &st) == 0) {
                    char logmsg[512];
                    snprintf(logmsg, sizeof(logmsg),
                             "发现新文件: %s (大小: %ld 字节)",
                             entry->d_name, st.st_size);
                    write_log(logmsg);
                }

                strncpy(known_files[known_count], entry->d_name, 255);
                known_count++;
            }
        }
        closedir(dir);
        sleep(SCAN_INTERVAL);
    }

    write_log("===== 守护进程正常退出 =====");
    return 0;
}
