#include "../type.h"
#include "test_util.h"

int major = 5;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test5_inode_direct_address_not_set(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_FILE;
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    bitmap_clear(map, sb, DUMMY_BNO);
    munmap(map, fs_sz);
}

void build_test5_inode_indirect_address_pointer_not_set(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_FILE;
    din.addrs[NDIRECT] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    bitmap_clear(map, sb, DUMMY_BNO);
    char empty_block[BSIZE] = {0};
    write_block(map, DUMMY_BNO, empty_block);
    munmap(map, fs_sz);
}

void build_test5_inode_indirect_address_not_set(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_FILE;
    din.addrs[NDIRECT] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    char empty_block[BSIZE] = {0};
    write_block(map, DUMMY_BNO, empty_block);
    int blockno = DUMMY_BNO+1;
    memcpy(&map[B2B(DUMMY_BNO, 0)], &blockno, sizeof(uint32_t));
    bitmap_clear(map, sb, blockno);
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
    build_test5_inode_direct_address_not_set();
    build_test5_inode_indirect_address_pointer_not_set();
    build_test5_inode_indirect_address_not_set();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
