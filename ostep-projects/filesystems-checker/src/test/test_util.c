#include "test_util.h"
#include "../type.h"
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

static int fd2sz(int fd)
{
    struct stat s;
    int rc = fstat(fd, &s);
    assert(rc != -1 && "fstat");
    return (int)s.st_size;
}

struct superblock *init_superblock(char *fs)
{
    struct superblock *sb = malloc(sizeof(struct superblock));
    assert(sb && "malloc");
    memcpy(sb, &fs[B2B(SUPERBLOCK_START, 0)], sizeof(struct superblock));
    return sb;
}

struct FsInfo *get_fs_info(void)
{
    int fd = open("fs.img", O_RDONLY);
    assert(fd != -1 && "open");
    int sz = fd2sz(fd);
    struct FsInfo *fs = malloc(sizeof(*fs));
    assert(fs && "malloc");
    fs->fd = fd;
    fs->sz = sz;
    return fs;
}

char *get_test_name(int major, int minor)
{
    static char name[256];
    sprintf(name, "test%d_%d.img", major, minor);
    return name;
}

char *copy_and_map(char *fs, int fs_sz, char *name)
{
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    int write_sz = write(fd, fs, fs_sz);
    assert(write_sz == fs_sz && "write");
    char *map = mmap(NULL, fs_sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(map != MAP_FAILED && "mmap");
    close(fd);
    return map;
}

void write_inode(char *fs, struct superblock *sb, int ino, struct dinode *inode)
{
    memcpy(&fs[B2B(IBLOCK(ino, sb), (ino % IPB) * sizeof(struct dinode))], inode, sizeof(struct dinode));
}

static char *read_block(char *fs, int blockno)
{
    static char block[BSIZE] = {0};
    memset(block, 0, BSIZE);
    memcpy(block, &fs[B2B(blockno, 0)], BSIZE);
    return block;
}

void write_block(char *fs, int blockno, char *block)
{
    memcpy(&fs[B2B(blockno, 0)], block, BSIZE);
}

void bitmap_set(char *fs, struct superblock *sb, int blockno)
{
    char *bitmap_block = read_block(fs, BBLOCK(blockno, sb));
    int m = 1 << (blockno % 8);
    bitmap_block[blockno/8] |= m;
    write_block(fs, BBLOCK(blockno, sb), bitmap_block);
}

void bitmap_clear(char *fs, struct superblock *sb, int blockno)
{
    char *bitmap_block = read_block(fs, BBLOCK(blockno, sb));
    int m = 1 << (blockno % 8);
    bitmap_block[blockno/8] &= ~m;
    write_block(fs, BBLOCK(blockno, sb), bitmap_block);
}
