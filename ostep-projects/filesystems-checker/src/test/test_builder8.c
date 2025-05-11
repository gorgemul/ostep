#include "../type.h"
#include "test_util.h"

int major = 8;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test8_indirect_block_pointer_use_twice(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_FILE;
    din.addrs[NDIRECT] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    write_inode(map, sb, DUMMY_INO+1, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO+1, "foo2");
    char empty_block[BSIZE] = {0};
    write_block(map, DUMMY_BNO, empty_block);
    munmap(map, fs_sz);
}

void build_test8_indirect_block_use_twice(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din= {0};
    din.type = T_FILE;
    din.addrs[NDIRECT] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    char empty_block[BSIZE] = {0};
    write_block(map, DUMMY_BNO, empty_block);
    int blockno = DUMMY_BNO+1;
    memcpy(&map[B2B(DUMMY_BNO, 0)], &blockno, sizeof(uint32_t));
    memcpy(&map[B2B(DUMMY_BNO, sizeof(uint32_t))], &blockno, sizeof(uint32_t));
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
    build_test8_indirect_block_pointer_use_twice();
    build_test8_indirect_block_use_twice();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
