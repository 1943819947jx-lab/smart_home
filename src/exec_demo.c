#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // 子进程用exec替换自己为 /bin/ls
        printf("[子进程] 即将变身...\n");
        execl("/bin/ls", "ls", "-l", NULL);
        // exec成功后下面这行不会执行
        perror("execl失败");
    } else {
        wait(NULL);
        printf("[父进程] 子进程已执行完毕\n");
    }
    return 0;
}
