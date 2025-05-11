#include "type.h"
#include "hash_set.h"
#include "hash_map.h"
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

char *itoa(size_t i)
{
    static char a[64];
    sprintf(a, "%zu", i);
    return a;
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

int bitmap_test(char *fs, struct superblock *sb, int blockno)
{
    char *bitmap_block = read_block(fs, BBLOCK(blockno, sb));
    int m = 1 << (blockno % 8);
    return (bitmap_block[blockno/8] & m) != 0;
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
    if (root_inode->type != T_DIR)
        log_die("ERROR: root directory does not exist.");
    if (root_inode->nlink < 1 || root_inode->size < 1)
        log_die("ERROR: root directory does not exist.");
    struct dirent *entries = (struct dirent*)read_block(fs, root_inode->addrs[0]);
    if (entries[0].inum != 1 || entries[1].inum != 1)
        log_die("ERROR: root directory does not exist.");
    if (strcmp(entries[0].name, ".") != 0 || strcmp(entries[1].name, "..") != 0)
        log_die("ERROR: root directory does not exist.");
}

void check_dir(char *fs)
{
    for (size_t i = 0; i < NINODES; i++) {
        struct dinode *inode = read_inode(fs, i);
        if (inode->type == T_DIR) {
           struct dirent *entries = (struct dirent*)read_block(fs, inode->addrs[0]);
           if (entries[0].inum != i || strcmp(entries[0].name, ".") != 0)
               log_die("ERROR: directory not properly formatted.");
           if (strcmp(entries[1].name, "..") != 0)
               log_die("ERROR: directory not properly formatted.");
        }
        free(inode);
    }
}

void check_datablock_bitmap_set(char *fs)
{
    for (size_t i = 0; i < NINODES; i++) {
        struct dinode *inode = read_inode(fs, i);
        for (size_t j = 0; j < NDIRECT; j++)
            if (inode->addrs[i] != 0 && !bitmap_test(fs, sb, inode->addrs[i]))
                log_die("ERROR: address used by inode but marked free in bitmap.");
        if (inode->addrs[NDIRECT] != 0) {
            if (!bitmap_test(fs, sb, inode->addrs[NDIRECT]))
                log_die("ERROR: address used by inode but marked free in bitmap.");
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++)
                if (buf[i] != 0 && !bitmap_test(fs, sb, buf[i]))
                    log_die("ERROR: address used by inode but marked free in bitmap.");
        }
        free(inode);
    }
}

void check_bitmapset_not_used_in_datablock(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t blockno = 0; blockno < BPB; blockno++)
        if (bitmap_test(fs, sb, blockno)) hash_set_add(set, itoa(blockno));
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        for (size_t i = 0; i < NDIRECT; i++)
            if (inode->addrs[i] != 0) hash_set_remove(set, itoa(inode->addrs[i]));
        if (inode->addrs[NDIRECT] != 0) {
            hash_set_remove(set, itoa(inode->addrs[NDIRECT]));
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++)
                if (buf[i] != 0) hash_set_remove(set, itoa(buf[i]));
        }
        free(inode);
    }
    // because meta data is not in data block
    int nmeta = sb->size - sb->nblocks;
    if (hash_set_size(set) != nmeta) log_die("ERROR: bitmap marks block in use but it is not in use.");
    hash_set_destroy(set);
}

void check_block_number_appear_only_once_in_direct_block(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        for (size_t i = 0; i < NDIRECT; i++) {
            if (inode->addrs[i] == 0) continue;
            if (hash_set_has(set, itoa(inode->addrs[i])))
                log_die("ERROR: direct address used more than once.");
            hash_set_add(set, itoa(inode->addrs[i]));
        }
        free(inode);
    }
    hash_set_destroy(set);
}

void check_block_number_appear_only_once_in_indirect_block(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->addrs[NDIRECT] == 0) continue;
        if (hash_set_has(set, itoa(inode->addrs[NDIRECT]))) log_die("ERROR: indirect address used more than once.");
        hash_set_add(set, itoa(inode->addrs[NDIRECT]));
        uint32_t buf[NINDIRECT];
        memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
        for (size_t i = 0; i < NINDIRECT; i++) {
            if (buf[i] == 0) continue;
            if (hash_set_has(set, itoa(buf[i]))) log_die("ERROR: indirect address used more than once.");
            hash_set_add(set, itoa(buf[i]));
        }
        free(inode);
    }
    hash_set_destroy(set);
}

void check_inode_mark_used_not_found_in_dir(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t ino = 2; ino < NINODES; ino++) { // because root dir should not inside any direntry but . and .. in itself
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_FREE) hash_set_add(set, itoa(ino));
        free(inode);
    }
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_DIR) continue;
        for (size_t i = 0; i < NDIRECT; i++) {
            if (inode->addrs[i] == 0) continue;
            struct dirent *entries = (struct dirent*)read_block(fs, inode->addrs[i]);
            for (size_t j = 0; j < DPB; j++) {
                if (entries[j].inum == 0) continue;
                if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                hash_set_remove(set, itoa(entries[j].inum));
            }
        }
        if (inode->addrs[NDIRECT] != 0) {
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++) {
                if (buf[i] == 0) continue;
                struct dirent *entries = (struct dirent*)read_block(fs, buf[i]);
                for (size_t j = 0; j < DPB; j++) {
                    if (entries[j].inum == 0) continue;
                    if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                    hash_set_remove(set, itoa(entries[j].inum));
                }
            }
        }
        free(inode);
    }
    if (hash_set_size(set) != 0) log_die("ERROR: inode marked use but not found in a directory.");
    hash_set_destroy(set);
}

void check_inode_refer_in_dir_but_not_exist(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_DIR) continue;
        for (size_t i = 0; i < NDIRECT; i++) {
            if (inode->addrs[i] == 0) continue;
            struct dirent *entries = (struct dirent*)read_block(fs, inode->addrs[i]);
            for (size_t j = 0; j < DPB; j++) {
                if (entries[j].inum == 0) continue;
                if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                hash_set_add(set, itoa(entries[j].inum));
            }
        }
        if (inode->addrs[NDIRECT] != 0) {
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++) {
                if (buf[i] == 0) continue;
                struct dirent *entries = (struct dirent*)read_block(fs, buf[i]);
                for (size_t j = 0; j < DPB; j++) {
                    if (entries[j].inum == 0) continue;
                    if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                    hash_set_add(set, itoa(entries[j].inum));
                }
            }
        }
        free(inode);
    }
    for (size_t ino = 2; ino < NINODES; ino++) { // because root dir should not inside any direntry but . and .. in itself
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_FREE) hash_set_remove(set, itoa(ino));
        free(inode);
    }
    if (hash_set_size(set) != 0)
        log_die("ERROR: inode referred to in directory but marked free.");
    hash_set_destroy(set);
}

void check_file_link_match_ref_in_dir(char *fs)
{
    struct HashMap *map = hash_map_init();
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_DIR) continue;
        for (size_t i = 0; i < NDIRECT; i++) {
            if (inode->addrs[i] == 0) continue;
            struct dirent *entries = (struct dirent*)read_block(fs, inode->addrs[i]);
            for (size_t j = 0; j < DPB; j++) {
                if (entries[j].inum == 0) continue;
                if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                hash_map_add(map, itoa(entries[j].inum));
            }
        }
        if (inode->addrs[NDIRECT] != 0) {
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++) {
                if (buf[i] == 0) continue;
                struct dirent *entries = (struct dirent*)read_block(fs, buf[i]);
                for (size_t j = 0; j < DPB; j++) {
                    if (entries[j].inum == 0) continue;
                    if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                    hash_map_add(map, itoa(entries[j].inum));
                }
            }
        }
        free(inode);
    }
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type == T_FILE && inode->nlink != hash_map_get(map, itoa(ino)))
            log_die("ERROR: bad reference count for file.");
        free(inode);
    }
    hash_map_destroy(map);
}

void check_dir_appear_twice_in_dir(char *fs)
{
    struct HashSet *set = hash_set_init();
    for (size_t ino = 0; ino < NINODES; ino++) {
        struct dinode *inode = read_inode(fs, ino);
        if (inode->type != T_DIR) continue;
        for (size_t i = 0; i < NDIRECT; i++) {
            if (inode->addrs[i] == 0) continue;
            struct dirent *entries = (struct dirent*)read_block(fs, inode->addrs[i]);
            for (size_t j = 0; j < DPB; j++) {
                if (entries[j].inum == 0) continue;
                if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                struct dinode *din = read_inode(fs, entries[j].inum);
                if (din->type == T_DIR) {
                    if (hash_set_has(set, itoa(entries[j].inum))) log_die("ERROR: directory appears more than once in file system.");
                    hash_set_add(set, itoa(entries[j].inum));
                }
            }
        }
        if (inode->addrs[NDIRECT] != 0) {
            uint32_t buf[NINDIRECT];
            memcpy(buf, &fs[B2B(inode->addrs[NDIRECT], 0)], BSIZE);
            for (size_t i = 0; i < NINDIRECT; i++) {
                if (buf[i] == 0) continue;
                struct dirent *entries = (struct dirent*)read_block(fs, buf[i]);
                for (size_t j = 0; j < DPB; j++) {
                    if (entries[j].inum == 0) continue;
                    if (strcmp(entries[j].name, ".") == 0 || strcmp(entries[j].name, "..") == 0) continue;
                    struct dinode *din = read_inode(fs, entries[j].inum);
                    if (din->type == T_DIR) {
                        if (hash_set_has(set, itoa(entries[j].inum))) log_die("ERROR: directory appears more than once in file system.");
                        hash_set_add(set, itoa(entries[j].inum));
                    }
                }
            }
        }
        free(inode);
    }
    hash_set_destroy(set);
}

void fs_check(char *fs)
{
    sb = init_superblock(fs);
    check_inode(fs);
    check_root_dir(fs);
    check_dir(fs);
    check_datablock_bitmap_set(fs);
    check_bitmapset_not_used_in_datablock(fs);
    check_block_number_appear_only_once_in_direct_block(fs);
    check_block_number_appear_only_once_in_indirect_block(fs);
    check_inode_mark_used_not_found_in_dir(fs);
    check_inode_refer_in_dir_but_not_exist(fs);
    check_file_link_match_ref_in_dir(fs);
    check_dir_appear_twice_in_dir(fs);
    free(sb);
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
    fs_check(fs);
    munmap(fs, file_sz);
    return 0;
}
