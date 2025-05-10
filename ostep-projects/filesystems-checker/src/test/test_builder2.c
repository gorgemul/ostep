#include "../type.h"
#include "test_util.h"
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define DUMMY_INODE_NUMBER 50
#define DUMMY_BLOCK_NUMBER 624 // actually not that dummy, need to make sure that block 624 has no data to run test

int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test2_direct_block_img(int offset, uint32_t blockno)
{
    char *name = get_test_name(2, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.addrs[offset] = blockno;
    write_inode(map, sb, DUMMY_INODE_NUMBER, &din);
    munmap(map, fs_sz);
}

// pointer_invalid is used to means that inode.addrs[NDIRECT] is invalid
void build_test2_indirect_block_img(uint32_t blockno, int is_pointer_invalid)
{
    char *name = get_test_name(2, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    if (is_pointer_invalid)
        din.addrs[NDIRECT] = blockno;
    else {
        din.addrs[NDIRECT] = DUMMY_BLOCK_NUMBER;
        memcpy(&map[B2B(DUMMY_BLOCK_NUMBER, 0)], &blockno, sizeof(uint32_t));
    }
    write_inode(map, sb, DUMMY_INODE_NUMBER, &din);
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
    build_test2_direct_block_img(0, 30);
    build_test2_direct_block_img(6, 58);
    build_test2_direct_block_img(10, 1008);
    build_test2_indirect_block_img(30, 1);
    build_test2_indirect_block_img(1003, 0);
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
