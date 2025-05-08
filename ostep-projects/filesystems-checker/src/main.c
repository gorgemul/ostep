#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

struct superblock *sb;

void log_die(char *err_msg)
{
    fprintf(stderr, "%s", err_msg);
    exit(1);
}

int fd2sz(int fd)
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

struct dinode *read_inode(char *fs, int ino)
{
    struct dinode *inode = malloc(sizeof(struct dinode));
    assert(inode && "malloc");
    memcpy(inode, &fs[B2B(IBLOCK(ino, sb), (ino % IPB) * sizeof(struct dinode))], sizeof(struct dinode));
    return inode;
}

void check_inode_type(char *fs)
{
    for (int i = 0; i < NINODES; i++) {
        struct dinode *inode = read_inode(fs, i);
        if (inode->type != T_FREE && inode->type != T_DIR && inode->type != T_FILE && inode->type != T_DEV)
            log_die("ERROR: bad inode.");
        free(inode);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) log_die("Usage: xcheck <file_system_image>");
    int fd = open(argv[1], O_RDONLY);
    assert(fd != -1 && "open");
    int file_sz = fd2sz(fd);
    char *fs = mmap(NULL, file_sz, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(fs != MAP_FAILED && "mmap");
    sb = init_superblock(fs);
    check_inode_type(fs);
    return 0;
}
