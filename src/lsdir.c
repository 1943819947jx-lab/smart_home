#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <string.h>

// 原有：打印单个文件/目录的stat信息
void print_file_info(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        perror("stat");
        return;
    }
    printf("文件: %s\n", path);
    printf("  大小: %ld 字节\n", st.st_size);
    printf("  权限: %o\n", st.st_mode & 0777);
    printf("  inode: %lu\n", st.st_ino);
    printf("  最后修改: %s", ctime(&st.st_mtime));
    printf("  是目录? %s\n\n", S_ISDIR(st.st_mode) ? "是" : "否");
}

// 新增：递归遍历子目录函数
void list_dir(const char *path, int depth) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (entry->d_name[0] == '.')
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) < 0) {
            perror("stat");
            continue;
        }

        // 层级缩进
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("%s (%lld bytes)\n", entry->d_name, st.st_size);

        // 如果是目录，递归深入
        if (S_ISDIR(st.st_mode)) {
            list_dir(fullpath, depth + 1);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <文件或目录路径>\n", argv[0]);
        exit(1);
    }

    const char *target = argv[1];
    // 第一步：打印目标自身stat信息
    print_file_info(target);

    // 判断如果传入的是目录，执行递归遍历
    struct stat st;
    stat(target, &st);
    if (S_ISDIR(st.st_mode)) {
        printf("========== 目录递归遍历结果 ==========\n");
        list_dir(target, 0);
    }

    return 0;
}

