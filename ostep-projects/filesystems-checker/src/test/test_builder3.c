#include "../type.h"
#include "test_util.h"
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define ROOT_DIR_INO 1

int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test3_root_dir_not_dir(short type)
{
    char *name = get_test_name(3, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = type;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

void build_test3_root_dir_no_link(void)
{
    char *name = get_test_name(3, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.size = 1;
    din.nlink = 0;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

void build_test3_root_dir_no_size()
{
    char *name = get_test_name(3, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.nlink = 1;
    din.size = 0;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

int main(void)
{
    struct FsInfo *fi = get_fs_info();
    fs_sz = fi->sz;
    fs = mmap(NULL, fs_sz, PROT_READ, MAP_PRIVATE, fi->fd, 0);
    assert(fs != MAP_FAILED && "mmap");
    close(fi->fd);
    sb = init_superblock(fs);
    build_test3_root_dir_not_dir(T_FREE);
    build_test3_root_dir_not_dir(T_FILE);
    build_test3_root_dir_not_dir(T_DEV);
    build_test3_root_dir_no_link();
    build_test3_root_dir_no_size();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
