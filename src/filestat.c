#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <string.h>

void print_file_info(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) { perror("stat"); return; }

    printf("文件: %s\n", path);
    printf("  大小: %ld 字节\n", st.st_size);
    printf("  权限: %o\n", st.st_mode & 0777);
    printf("  inode: %lu\n", st.st_ino);
    printf("  最后修改: %s", ctime(&st.st_mtime));
    printf("  是目录? %s\n", S_ISDIR(st.st_mode) ? "是" : "否");
}

void list_dir(const char *path, int depth) {
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat st;
        stat(fullpath, &st);

        // 缩进表示层级
        for (int i = 0; i < depth; i++) printf("  ");
        printf("%s (%ld bytes)\n", entry->d_name, st.st_size);

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

    const char *path = argv[1];
    struct stat st;

    if (stat(path, &st) < 0) {
        perror("stat");
        exit(1);
    }

    // 先打印文件/目录的基本信息
    print_file_info(path);

    // 如果是目录，递归列出内容
    if (S_ISDIR(st.st_mode)) {
        printf("\n目录内容:\n");
        list_dir(path, 0);
    }

    return 0;
}
