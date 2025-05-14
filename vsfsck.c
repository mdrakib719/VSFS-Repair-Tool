// CSE321 LAB GROUP 08 PROJECT 02
// Authors: Md. Rakib Hossain, Dewan M.I. Mukto, Mahmuda Akter Nipa
// Student IDs: 21301188, 24201311, 20301086

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* From the Project docx file */
#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS 64
#define INODE_BITMAP_BLOCK 1 // Block 1: Inode bitmap
#define DATA_BITMAP_BLOCK 2 // Block 2: Data bitmap
#define INODE_TABLE_START 3 // Blocks 3–7: Inode table (5 blocks)
#define DATA_BLOCK_START 8 // Blocks 8–63: Data blocks
#define INODE_SIZE 256 // Inodes: 256 Bytes each
#define SUPERBLOCK_MAGIC 0xD34D

// 2 bytes = unsigned 16-bit int (2x8)
// 4 bytes = unsigned 32-bit int (4x8)

// Derived constants
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define TOTAL_INODES ((5 * BLOCK_SIZE) / INODE_SIZE) // 5 blocks allocated for inode table

typedef struct {
    uint16_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_bitmap_block;
    uint32_t data_bitmap_block;
    uint32_t inode_table_start;
    uint32_t data_block_start;
    uint32_t inode_size;
    uint32_t inode_count;
    char reserved[4058];
} __attribute__((packed)) SuperBlock;

typedef struct {
    uint32_t mode;
    uint32_t user_identifier;
    uint32_t group_identifier;
    uint32_t file_size;
    uint32_t access_time;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t deletion_time;
    uint32_t links_count;
    uint32_t block_count;
    uint32_t direct_pointer;
    uint32_t single_indirect_pointer;
    uint32_t double_indirect_pointer;
    uint32_t triple_indirect_pointer;
    char reserved[156];
} __attribute__((packed)) Inode;

/* ANSI escape codes for terminal colors */
char *SUCCESS_MESSAGE = "\033[32m [SUCCESS]\033[0m";
char *INFORMATION_MESSAGE = "\033[36m [INFO]\033[0m";
char *YELLOW_COLOR = "\x1b[33m";
char *GREEN_COLOR = "\x1b[32m";
char *RESET_COLOR = "\x1b[0m";

int inode_bitmap[BLOCK_SIZE];
int data_bitmap[BLOCK_SIZE];
int block_reference_count[TOTAL_BLOCKS] = {0};
int block_owner_inode[TOTAL_BLOCKS] = {-1};

void read_block(FILE *file_pointer, int block_number, void *buffer) {
    fseek(file_pointer, block_number * BLOCK_SIZE, SEEK_SET);
    fread(buffer, BLOCK_SIZE, 1, file_pointer);
}

void write_block(FILE *file_pointer, int block_number, void *buffer) {
    fseek(file_pointer, block_number * BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, BLOCK_SIZE, 1, file_pointer);
}

void write_inode(FILE *file_pointer, SuperBlock *superblock, int inode_number, Inode *inode) {
    int block_index = superblock->inode_table_start + (inode_number / INODES_PER_BLOCK);
    int offset = (inode_number % INODES_PER_BLOCK) * INODE_SIZE;

    uint8_t block_buffer[BLOCK_SIZE];
    read_block(file_pointer, block_index, block_buffer);
    memcpy(block_buffer + offset, inode, sizeof(Inode));
    write_block(file_pointer, block_index, block_buffer);
}

void validate_superblock(SuperBlock *superblock) {
    printf("Superblock Validation:\n");

    if (superblock->magic != SUPERBLOCK_MAGIC)
        printf("  Invalid magic number: 0x%x\n", superblock->magic);
    else
        printf("  Magic number OK\n");

    if (superblock->block_size != BLOCK_SIZE)
        printf("  Invalid block size: %u\n", superblock->block_size);
    else
        printf("  Block size OK: %u\n", superblock->block_size);

    if (superblock->total_blocks != TOTAL_BLOCKS)
        printf("  Invalid total blocks: %u\n", superblock->total_blocks);
    else
        printf("  Total blocks OK: %u\n",superblock->total_blocks);
}

void fix_inodes(FILE *file_pointer, SuperBlock *superblock, int *changes_made) {
    printf("\nInode and Data Bitmap Consistency:\n");
    *changes_made = 0;

    for (int inode_index = 0; inode_index < TOTAL_INODES; inode_index++) {
        int block_index = superblock->inode_table_start + (inode_index / INODES_PER_BLOCK);
        int offset = (inode_index % INODES_PER_BLOCK) * INODE_SIZE;

        uint8_t block_buffer[BLOCK_SIZE];
        read_block(file_pointer, block_index, block_buffer);

        Inode inode;
        memcpy(&inode, block_buffer + offset, sizeof(Inode));

        int bitmap_flag = (inode_bitmap[inode_index / 8] >> (inode_index % 8)) & 1;
        int is_inode_valid = (inode.links_count > 0 && inode.deletion_time == 0);

        if (bitmap_flag && !is_inode_valid) {
            printf("  FIX: Clearing inode bitmap for invalid inode %d\n", inode_index);
            inode_bitmap[inode_index / 8] &= ~(1 << (inode_index % 8));
            *changes_made = 1;
        }
        if (!bitmap_flag && is_inode_valid) {
            printf("  FIX: Setting inode bitmap for valid inode %d\n", inode_index);
            inode_bitmap[inode_index / 8] |= (1 << (inode_index % 8));
            *changes_made = 1;
        }

        if (is_inode_valid) {
            
            if (inode.direct_pointer != 0) {
                uint32_t block_number = inode.direct_pointer;

                if (block_number < DATA_BLOCK_START || block_number >= TOTAL_BLOCKS) {
                    printf("  FIX: Inode %d has invalid block %u, zeroing pointer\n", inode_index, block_number);
                    inode.direct_pointer = 0;
                    inode.block_count = 0;
                    write_inode(file_pointer, superblock, inode_index, &inode);
                    *changes_made = 1;
                } else {
                    if (block_reference_count[block_number] > 0) {
                        printf("  FIX: Duplicate block %u in inode %d, removing pointer\n", block_number, inode_index);
                        inode.direct_pointer = 0;
                        inode.block_count = 0;
                        write_inode(file_pointer, superblock, inode_index, &inode);
                        *changes_made = 1;  
                    } else {
                        block_reference_count[block_number]++;
                        block_owner_inode[block_number] = inode_index;
                    }

                    if (((data_bitmap[block_number / 8] >> (block_number % 8)) & 1) == 0) {
                        printf("  FIX: Setting data bitmap for used block %u (inode %d)\n", block_number, inode_index);
                        data_bitmap[block_number / 8] |= (1 << (block_number % 8));
                        *changes_made = 1;
                    }
                }
            }
        }
    }
}

void fix_data_bitmap(int *changes_made) {
    printf("\nFixing data bitmap inconsistencies:\n");
    *changes_made = 0;

    for (int block_index = DATA_BLOCK_START; block_index < TOTAL_BLOCKS; block_index++) {
        int is_marked = (data_bitmap[block_index / 8] >> (block_index % 8)) & 1;
        if (is_marked && block_reference_count[block_index] == 0) {
            printf("  FIX: Clearing data bitmap for unused block %d\n", block_index);
            data_bitmap[block_index / 8] &= ~(1 << (block_index % 8));
            *changes_made = 1;
        }
    }
}

int main() {
    printf("%s CSE321-19 Lab Project 02, Group 08\n", INFORMATION_MESSAGE);
    printf("%s \u00A9 Rakib (21301188), Mukto (24201311), Nipa (20301086)\n", INFORMATION_MESSAGE);

    char file_name[256];
    printf("Enter the file name of the VSFS image: ");
    scanf("%s", file_name);

    FILE *file_pointer = fopen(file_name, "r+b");
    if (!file_pointer) {
        perror("Error opening the specified file");
        return 1;
    }

    SuperBlock superblock;
    read_block(file_pointer, 0, &superblock);
    validate_superblock(&superblock);

    read_block(file_pointer, superblock.inode_bitmap_block, inode_bitmap);
    read_block(file_pointer, superblock.data_bitmap_block, data_bitmap);

    int changes_made;
    do {
        fix_inodes(file_pointer, &superblock, &changes_made);
        fix_data_bitmap(&changes_made);

        write_block(file_pointer, superblock.inode_bitmap_block, inode_bitmap);
        write_block(file_pointer, superblock.data_bitmap_block, data_bitmap);

    } while (changes_made);

    printf("\nAll detected issues have been fixed. Re-check with this tool to verify.\n");

    fclose(file_pointer);
    return 0;
}
