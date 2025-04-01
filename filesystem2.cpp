#include <stdio.h>
#include <iostream>

//To Do: add files, get files, delete files, encryption

#define DISKNAME "disk.img"
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1024
#define MAX_FILES 100

using namespace std;

typedef struct {
    char filename[50];
    int size;
    int startBlock;  // Points to the first data block
} File;

typedef struct {
    File files[MAX_FILES];
    int filecount;
}rootstruct;

// Superblock structure
typedef struct {
    int totalBlocks;
    int blockSize;
    int freeBlocks;
    int freeBlockBitmap[NUM_BLOCKS];
} Superblock;

Superblock superblock;
rootstruct root;

void createfilesystem() {

superblock.totalBlocks = NUM_BLOCKS;
superblock.blockSize = BLOCK_SIZE;
superblock.freeBlocks = NUM_BLOCKS;
memset(superblock.freeBlockBitmap, 0, sizeof(superblock.freeBlockBitmap)); // All blocks marked as free


    FILE* file = fopen(DISKNAME, "wb");
    if (!file) {
       cout << "error making" << DISKNAME << endl;
    }
    root.filecount = 0;
    fwrite(&superblock, sizeof(superblock), 1, file);
    fwrite(&root, sizeof(root), 1, file);

    char emptyBlock[BLOCK_SIZE] = { 0 };               
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(emptyBlock, BLOCK_SIZE, 1, file);      // Write empty data blocks math
    }
    fclose(file);
}

bool createfile(char* filename) {
    FILE* file = fopen("disk.img", "r+b");
    if (!file) {
        printf("Error opening disk for writing\n");
        return false;
    }
    FILE* addingfile = fopen(filename, "r");
    if (!addingfile) {
        printf("Error opening input file: %s\n", filename);
        fclose(file);
        return false;
    }

    fseek(addingfile, 0, SEEK_END);
    int addingfilesize = ftell(addingfile);
    fseek(addingfile, 0, SEEK_SET);

    printf("Adding file size: %d bytes\n", addingfilesize);
    printf("Free blocks available: %d\n", superblock.freeBlocks);

    int blocksNeeded = (addingfilesize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (superblock.freeBlocks < blocksNeeded) {
        printf("Not enough free space on the image\n");
        fclose(file);
        fclose(addingfile);
        return false;
    }

    int startBlock = -1;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (superblock.freeBlockBitmap[i] == 0) { // Find the first free block
            if (startBlock == -1) startBlock = i;
            blocksNeeded--;
            superblock.freeBlockBitmap[i] = 1; // Mark block as used
            if (blocksNeeded == 0) break;
        }
    }

    if (blocksNeeded > 0) {
        printf("Error: Unable to allocate contiguous blocks\n");
        fclose(file);
        fclose(addingfile);
        return false;
    }

    // Write file data to disk image
    fseek(file, sizeof(Superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);

    char buffer[BLOCK_SIZE];
    size_t bytesRead;
    int blocksWritten = 0;
    while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, addingfile)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
        printf("Writing block %d for file %s...\n", blocksWritten + 1, filename);
        blocksWritten++;
    }

    // Update file metadata
    File newFile;
    strncpy(newFile.filename, filename, sizeof(newFile.filename) - 1);
    newFile.filename[sizeof(newFile.filename) - 1] = '\0'; // Ensure null termination
    newFile.size = addingfilesize;
    newFile.startBlock = startBlock;

    root.files[root.filecount++] = newFile;
    superblock.freeBlocks -= blocksWritten;

    // Write updated structures to disk
    fseek(file, 0, SEEK_SET);
    fwrite(&superblock, sizeof(Superblock), 1, file);
    fwrite(&root, sizeof(rootstruct), 1, file);

    printf("File added successfully! Current file count: %d, Free space: %d blocks\n",
           root.filecount, superblock.freeBlocks);

    fclose(file);
    fclose(addingfile);

    return true;
}



bool readfile(char *files) {
    FILE *file = fopen("disk.img", "r+b");
    if (!file) {
        printf("Error opening %s on the img\n", files);
    }

    for (int i = 0; i < root.filecount; i++) {
        if (strcmp(root.files[i].filename, files) == 0) {
            int startBlock = root.files[i].startBlock;
            int fileSize = root.files[i].size;

            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);

            char *buffer = (char *) malloc(fileSize * sizeof(char));
            fread(buffer, 1, fileSize, file);
            buffer[fileSize] = '\0';
            printf("Data from %s\n\n%s\n", files, buffer);
            fclose(file);
            free(buffer);
            return true;
        } 
    }
    printf("File not found...\n");
    fclose(file);
    return false;

}
int main(int argc, char* argv[]) {

FILE* check = fopen("disk.img", "rb");
if (!check) {
     createfilesystem();
    cout << DISKNAME << "is a binary filesystem" << endl;
}
else {
    fread(&superblock, sizeof(superblock), 1, check);
    fread(&root, sizeof(root), 1, check);
    cout << DISKNAME << " already exist" << endl;
}

if (strcmp(argv[1], "add") == 0) {
if (createfile(argv[2])) {
    printf("Successfully wrote %s to image file\n", argv[2]);
} else { 
    printf("Failed creating file...\n");
    return 0;
}
}

if (strcmp(argv[1], "read") == 0) {
    if (readfile(argv[2])) {
        printf("Reading from file...\n");
    } else {
        printf("Failed reading file from img...\n");
    }
return 0;
}

fclose(check);
}
