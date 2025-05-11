#include "../type.h"
#include "test_util.h"

int major = 10;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test10_inode_in_dir_but_mark_free(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_DIR;
    din.addrs[0] = NO_USED_BNO;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    struct dirent entries[DPB];
    entries[0].inum = DUMMY_INO;
    strcpy(entries[0].name, ".");
    entries[1].inum = DUMMY_INO-1; // don't care
    strcpy(entries[1].name, "..");
    entries[2].inum = DUMMY_INO+1;
    strcpy(entries[2].name, "foo2");
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
    build_test10_inode_in_dir_but_mark_free();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
