#include "../type.h"
#include "test_util.h"

int major = 3;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test3_root_dir_not_dir(short type)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = type;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

void build_test3_root_dir_no_link(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.size = 1;
    din.nlink = 0;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

void build_test3_root_dir_no_size(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.nlink = 1;
    din.size = 0;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    munmap(map, fs_sz);
}

void build_test3_root_dir_inode_not_1(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    struct dirent entries[DPB];
    entries[0].inum = 2;
    strcpy(entries[0].name, ".");
    entries[1].inum = 2;
    strcpy(entries[1].name, "..");
    write_block(map, DUMMY_BNO, (char*)entries);
    munmap(map, fs_sz);
}

void build_test3_root_dir_parent_not_itself(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, ROOT_DIR_INO, &din);
    struct dirent entries[DPB];
    memset(entries, 0, BSIZE);
    entries[0].inum = 1;
    strcpy(entries[0].name, ".");
    entries[1].inum = 0;
    strcpy(entries[1].name, "..");
    write_block(map, DUMMY_BNO, (char*)entries);
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
    build_test3_root_dir_inode_not_1();
    build_test3_root_dir_parent_not_itself();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
