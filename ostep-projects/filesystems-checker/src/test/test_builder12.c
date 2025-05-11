#include "../type.h"
#include "test_util.h"

int major = 12;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test12_dir_appear_more_than_once_in_fs(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.addrs[0] = NO_USED_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo2");
    struct dirent entries[DPB] = {0};
    entries[0].inum = DUMMY_INO;
    strcpy(entries[0].name, ".");
    entries[1].inum = DUMMY_INO-1;
    strcpy(entries[1].name, "..");
    write_block(map, NO_USED_BNO, (char*)entries);
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
    build_test12_dir_appear_more_than_once_in_fs();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
