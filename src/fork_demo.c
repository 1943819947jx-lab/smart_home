#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // 子进程：fork返回0
        printf("[子进程] PID = %d, 父进程PID = %d\n", getpid(), getppid());
    } else {
        // 父进程：fork返回子进程PID
        printf("[父进程] PID = %d, 子进程PID = %d\n", getpid(), pid);
        wait(NULL);  // 等待子进程结束
        printf("[父进程] 子进程已结束\n");
    }
    return 0;
}
