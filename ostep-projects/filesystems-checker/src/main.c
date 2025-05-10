#include "type.h"
#include <stdio.h>
#include <unistd.h>
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

char *read_block(char *fs, int blockno)
{
    static char block[BSIZE] = {0};
    memset(block, 0, BSIZE);
    memcpy(block, &fs[B2B(blockno, 0)], BSIZE);
    return block;
}

int is_valid_datablock_region(uint32_t blockno)
{
    if (blockno == 0) return 1;
    uint32_t nmeta = sb->size - sb->nblocks;
    if (blockno < nmeta || blockno >= sb->size) return 0;
    return 1;
}

void check_inode_type(struct dinode *inode)
{
    if (inode->type != T_FREE && inode->type != T_DIR && inode->type != T_FILE && inode->type != T_DEV)
        log_die("ERROR: bad inode.");
}

void check_inode_direct_block(struct dinode *inode)
{
    for (size_t i = 0; i < NDIRECT; i++)
        if (!is_valid_datablock_region(inode->addrs[i])) log_die("ERROR: bad direct address in inode.");
}

void check_inode_indirect_block(char *fs, struct dinode *inode)
{
    if (!is_valid_datablock_region(inode->addrs[NDIRECT])) log_die("ERROR: bad indirect address in inode.");
    uint32_t buf[NINDIRECT];
    memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
    for (size_t i = 0; i < NINDIRECT; i++)
        if (!is_valid_datablock_region(buf[i])) log_die("ERROR: bad indirect address in inode.");
}

void check_inode(char *fs)
{
    for (size_t i = 0; i < NINODES; i++) {
        struct dinode *inode = read_inode(fs, i);
        check_inode_type(inode);
        check_inode_direct_block(inode);
        check_inode_indirect_block(fs, inode);
        free(inode);
    }
}

void check_root_dir(char *fs)
{
    struct dinode *root_inode = read_inode(fs, ROOT_DIR_INO);
    if (root_inode->type != T_DIR) log_die("ERROR: root directory does not exist.");
    if (root_inode->nlink < 1 || root_inode->size < 1) log_die("ERROR: root directory does not exist.");
    struct dirent *entries = (struct dirent*)read_block(fs, root_inode->addrs[0]);
    if (entries[0].inum != 1 || entries[1].inum != 1) log_die("ERROR: root directory does not exist.");
    if (strcmp(entries[0].name, ".") != 0 || strcmp(entries[1].name, "..") != 0) log_die("ERROR: root directory does not exist.");
}

int main(int argc, char **argv)
{
    if (argc != 2) log_die("Usage: xcheck <file_system_image>");
    int fd = open(argv[1], O_RDONLY);
    assert(fd != -1 && "open");
    int file_sz = fd2sz(fd);
    char *fs = mmap(NULL, file_sz, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(fs != MAP_FAILED && "mmap");
    close(fd);
    sb = init_superblock(fs);
    check_inode(fs);
    check_root_dir(fs);
    munmap(fs, file_sz);
    free(sb);
    return 0;
}
