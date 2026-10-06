#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *fire_and_forget(void *arg) {
    sleep(2);
    printf("分离线程完成，无需join\n");
    return NULL;
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, fire_and_forget, NULL);
    pthread_detach(t);  // 标记为分离状态，结束后系统自动回收
    
    printf("主线程继续干活，不用等这个线程\n");
    sleep(3);  // 主线程停3秒，不然进程直接退出，分离线程还没跑完就被销毁了
    printf("主线程结束\n");
    return 0;
}
