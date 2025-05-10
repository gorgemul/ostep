#include "../type.h"
#include "test_util.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test1_wrong_inode_type(short type, int ino)
{
    char *name = get_test_name(1, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = type;
    write_inode(map, sb, ino, &din);
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
    build_test1_wrong_inode_type(5, 1);
    build_test1_wrong_inode_type(8, 100);
    build_test1_wrong_inode_type(10, 199);
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
