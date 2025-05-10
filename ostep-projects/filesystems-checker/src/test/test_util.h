#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include "../type.h"
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


#define DUMMY_INO 50
#define DUMMY_BNO 100

struct FsInfo { int fd; int sz; };

struct FsInfo *get_fs_info(void);
struct superblock *init_superblock(char *fs);
char *get_test_name(int major, int minor);
char *copy_and_map(char *fs, int fs_sz, char *name);
void write_inode(char *fs, struct superblock *sb, int ino, struct dinode *inode);
void write_block(char *fs, int blockno, char *block);
void bitmap_set(char *fs, struct superblock *sb, int blockno);
void bitmap_clear(char *fs, struct superblock *sb, int blockno);

#endif // _TEST_UTIL_H_
