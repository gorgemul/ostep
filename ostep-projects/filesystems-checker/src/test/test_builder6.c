#include "../type.h"
#include "test_util.h"

#define NOT_USED_BLOCKNO 800

int major = 6;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test6_bitmapset_but_no_used(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    bitmap_set(map, sb, NOT_USED_BLOCKNO);
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
    build_test6_bitmapset_but_no_used();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
