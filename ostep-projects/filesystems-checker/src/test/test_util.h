#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include "../type.h"

struct FsInfo { int fd; int sz; };

struct FsInfo *get_fs_info(void);
struct superblock *init_superblock(char *fs);
char *get_test_name(int major, int minor);
char *copy_and_map(char *fs, int fs_sz, char *name);
void write_inode(char *fs, struct superblock *sb, int ino, struct dinode *inode);

#endif // _TEST_UTIL_H_
