#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(){
int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
if (fd < 0){ perror("open");exit(1); }

const char *msg = "Hello,Linux File IO\n传感器数据temp=36°c\n";
ssize_t written = write(fd, msg, strlen(msg));
printf("写入了%ld字节\n",written);
close(fd);

fd = open("test.txt",O_RDONLY);
if(fd < 0)
{
perror("open");
exit(1);
}

char buf[256] = {0};
ssize_t n = read(fd,buf,sizeof(buf) -1);
printf("读到了%ld字节\n%s",n,buf);
close(fd);
return 0;
}



