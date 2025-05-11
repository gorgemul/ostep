#include "../type.h"
#include "test_util.h"

int major = 11;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test11_file_nlink_not_match_ref_in_dir(void)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.type = T_FILE;
    din.nlink = 1;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo2"); // 2 ref
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
    build_test11_file_nlink_not_match_ref_in_dir();
    munmap(fs, fs_sz);
    free(sb);
    free(fi);
    return 0;
}
