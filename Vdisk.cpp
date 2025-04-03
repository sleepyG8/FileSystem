#include <stdio.h>
#include <iostream>

//To Do: *add files, *get files, *delete files, encryption

#define DISKNAME "disk.img"
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1024
#define MAX_FILES 100

using namespace std;

typedef struct {
    char filename[50];
    int size;
    int startBlock;  
} File;

typedef struct {
    File files[MAX_FILES];
    int filecount;
}rootstruct;

// superblock used to track metadata
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
memset(superblock.freeBlockBitmap, 0, sizeof(superblock.freeBlockBitmap)); // mark as free


    FILE* file = fopen(DISKNAME, "wb");
    if (!file) {
       cout << "error making" << DISKNAME << endl;
    }
    root.filecount = 0;
    fwrite(&superblock, sizeof(superblock), 1, file);
    fwrite(&root, sizeof(root), 1, file);

    char emptyBlock[BLOCK_SIZE] = { 0 };               
    for (int i = 0; i < NUM_BLOCKS; i++) {
        fwrite(emptyBlock, BLOCK_SIZE, 1, file);      // Write empty data blocks aka intense math
    }
    fclose(file);
}
// this function was a pain in the **** to get working lol but alas...
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

    //printf("Adding file size: %d bytes\n", addingfilesize);
    //printf("Free blocks available: %d\n", superblock.freeBlocks);

    int blocksNeeded = (addingfilesize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (superblock.freeBlocks < blocksNeeded) {
        printf("Not enough free space on the image\n");
        fclose(file);
        fclose(addingfile);
        return false;
    }

    int startBlock = -1;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (superblock.freeBlockBitmap[i] == 0) { // find free blocks
            if (startBlock == -1) startBlock = i;
            blocksNeeded--;
            superblock.freeBlockBitmap[i] = 1; // marks blocks used
            if (blocksNeeded == 0) break;
        }
    }

    if (blocksNeeded > 0) {
        printf("Error: Unable to allocate contiguous blocks\n");
        fclose(file);
        fclose(addingfile);
        return false;
    }

    // write data to the img
    fseek(file, sizeof(Superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);

    char buffer[BLOCK_SIZE];
    size_t bytesRead;
    int blocksWritten = 0;
    while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, addingfile)) > 0) {
        fwrite(buffer, 1, bytesRead, file);
        //printf("Writing block %d for file %s...\n", blocksWritten + 1, filename);
        blocksWritten++;
    }


    File newFile;
    strncpy(newFile.filename, filename, sizeof(newFile.filename) - 1);
    newFile.filename[sizeof(newFile.filename) - 1] = '\0'; // Ensure null termination
    newFile.size = addingfilesize;
    newFile.startBlock = startBlock;

    root.files[root.filecount++] = newFile;
    superblock.freeBlocks -= blocksWritten;

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
            printf("%s\n", buffer);
            fclose(file);
            free(buffer);
            return true;
        } 
    }
    printf("File not found...\n");
    fclose(file);
    return false;

}
// easy stuff
bool deletefile(char* files) {
FILE* file = fopen("disk.img", "r+b");
if (!file) {
    printf("Error reading file\n");
    return false;
}
    for (int i = 0; i < root.filecount; i++) {
        if (strcmp(root.files[i].filename, files) == 0) {
            int startBlock = root.files[i].startBlock;
            int filesize = root.files[i].size;

            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
            char *buffer = (char *)malloc(filesize);

            for (int block = startBlock; block < startBlock + (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE; block++) {
                superblock.freeBlockBitmap[block] = 0;
            }
            root.filecount--;

            for (int j = i; j < root.filecount; j++) {
                root.files[j] = root.files[j + 1]; 
            }

            superblock.freeBlocks += (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE;

            memset(buffer, '0', sizeof(root.files[i].size));

            fseek(file, 0, SEEK_SET);
            fwrite(&superblock, sizeof(superblock), 1, file);
            fwrite(&root, sizeof(root), 1, file);
            
            memset(buffer, '0', sizeof(root.files[i].size));
            fwrite(buffer, sizeof(buffer), 1, file);
           
        }
    }
    fclose(file);
    return true;
}
// listing from the root struct 
bool listfilenames() {
    FILE *file = fopen("disk.img", "r+b");
    if (!file) {
        printf("Error reading img\n");
        return false;
    }
    fseek(file, sizeof(superblock), SEEK_SET);
    fread(&root, sizeof(root), 1, file);
    if (root.filecount == 0) {
        printf("No Files stored in the img\n");
    }

    for (int i = 0; i < root.filecount; i++) {
    printf(" + %s\n", root.files[i].filename);
    }
    return true;
    fclose(file);
}
// if statements seperate args used functions to keep my main code clean
// I probably will create a .h and .c files for the functions so this doesnt get to 300+ lines of reading
int main(int argc, char* argv[]) {

FILE* check = fopen("disk.img", "rb");
if (!check) {
     createfilesystem();
    cout << DISKNAME << "is a binary filesystem" << endl;
}
else {
    fread(&superblock, sizeof(superblock), 1, check);
    fread(&root, sizeof(root), 1, check);
    cout << "Using: +" << DISKNAME << endl;
}
if (argc < 2) {
    printf("Usage:\n list\n read <filename>\n add <filename/path>\n");
}

if (strcmp(argv[1], "add") == 0) {
if (createfile(argv[2])) {
    //printf("Successfully wrote %s to image file\n", argv[2]);
    return 0;
} else { 
    printf("Failed creating file...\n");
    return 1;
}
}

if (strcmp(argv[1], "read") == 0) {
    if (readfile(argv[2])) {
        //printf("Reading from file...\n");
    } else {
        printf("Failed reading file from img...\n");
        return 1;
    }
return 0;
}

if (strcmp(argv[1], "delete") == 0) {
    if (deletefile(argv[2])) {
        printf("file deleted");
    } else {
        printf("failed to delete file\n");
    }
}

if (strcmp(argv[1], "list") == 0) {
    if (listfilenames()) {
        return 0;
    } else {
        printf("error listing files\n");
        return 1;
    }
}

fclose(check);
}


// if you notice the random print statements, you can uncomment for more file data, I was using to debug my createfile()
