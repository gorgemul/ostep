#include "../type.h"
#include "test_util.h"

int first1 = 1;
int first2 = 1;
int major = 2;
int minor = 1;
char *fs;
int fs_sz;
struct superblock *sb;

void build_test2_direct_block_img(int offset, uint32_t blockno)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    din.addrs[offset] = blockno;
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
    munmap(map, fs_sz);
}

// pointer_invalid is used to means that inode.addrs[NDIRECT] is invalid
void build_test2_indirect_block_img(uint32_t blockno, int is_pointer_invalid)
{
    char *name = get_test_name(major, minor++);
    char *map = copy_and_map(fs, fs_sz, name);
    struct dinode din = {0};
    if (is_pointer_invalid)
        din.addrs[NDIRECT] = blockno;
    else {
        din.addrs[NDIRECT] = DUMMY_BNO;
        char empty_block[BSIZE] = {0};
        write_block(map, DUMMY_BNO, empty_block);
        memcpy(&map[B2B(DUMMY_BNO, 0)], &blockno, sizeof(uint32_t));
    }
    write_inode(map, sb, DUMMY_INO, &din);
    save_file_in_root_dir(map, sb, DUMMY_INO, "foo1");
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
