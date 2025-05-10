#include "../type.h"
#include "test_util.h"

int major = 4;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test4_dir_not_contain_dot_entry(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    struct dirent entries[DPB];
    entries[0].inum = DUMMY_INO-1; // don't care
    strcpy(entries[0].name, "..");
    write_block(map, DUMMY_BNO, (char*)entries);
    munmap(map, fs_sz);
}

void build_test4_dir_not_contain_dot_dot_entry(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    struct dirent entries[DPB];
    entries[0].inum = DUMMY_INO;
    strcpy(entries[0].name, ".");
    entries[1].inum = DUMMY_INO+1;
    strcpy(entries[1].name, "foobar");
    write_block(map, DUMMY_BNO, (char*)entries);
    munmap(map, fs_sz);
}

void build_test4_dir_dot_entry_not_point_to_itself(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.addrs[0] = DUMMY_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    struct dirent entries[DPB];
    entries[0].inum = DUMMY_INO+1;
    strcpy(entries[0].name, ".");
    entries[1].inum = DUMMY_INO-1;
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
    build_test4_dir_not_contain_dot_entry();
    build_test4_dir_not_contain_dot_dot_entry();
    build_test4_dir_dot_entry_not_point_to_itself();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
