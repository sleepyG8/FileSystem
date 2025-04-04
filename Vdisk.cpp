#include <stdio.h>
#include <iostream>

//To Do: encryption and start building a kernel, ive got a head start on process manager and the terminal or maybe something else...

#define DISKNAME "disk.img"
#define BLOCK_SIZE 512
#define NUM_BLOCKS 1024
#define MAX_FILES 100

using namespace std;

typedef struct {
    char filename[50];
    int size;
    int startBlock;  
    int isDirectory;
} File;

typedef struct {
    char name[50];
    int childCount;
    File children[MAX_FILES];
    File *parent;
} DirectoryData;

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
        if (root.files[i].isDirectory) {
            printf(" + %s\n", root.files[i].filename);
        }
    printf(" - %s\n", root.files[i].filename);
    }
    return true;
    fclose(file);
}

//this is when it got real when making directories

bool createdir(char *dirname) {
    FILE *file = fopen("disk.img", "r+b");
    if (!file) {
        printf("Error opening disk image.\n");
        return false;
    }

    if (root.filecount >= MAX_FILES) {
        printf("Root directory is full.\n");
        fclose(file);
        return false;
    }

    // Find a free block for the directory
    int startBlock = -1;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (superblock.freeBlockBitmap[i] == 0) {
            startBlock = i;
            superblock.freeBlockBitmap[i] = 1;
            superblock.freeBlocks--;
            break;
        }
    }
    if (startBlock == -1) {
        printf("No free blocks available.\n");
        fclose(file);
        return false;
    }

    // Create directory metadata
    File dir = {0};
    strncpy(dir.filename, dirname, sizeof(dir.filename) - 1);
    dir.filename[sizeof(dir.filename) - 1] = '\0'; // Ensure null termination
    dir.isDirectory = 1;
    dir.startBlock = startBlock;
    root.files[root.filecount++] = dir;

    DirectoryData dirData = {0};
    fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
    fwrite(&dirData, sizeof(DirectoryData), 1, file);

    // Update superblock and root in the disk image
    fseek(file, 0, SEEK_SET);
    fwrite(&superblock, sizeof(Superblock), 1, file);
    fwrite(&root, sizeof(rootstruct), 1, file);

    printf("Directory '%s' created successfully.\n", dirname);
    fclose(file);
    return true;
}

bool addFileToDirectory(char *dirname, char *filename) {
    FILE *file = fopen(DISKNAME, "r+b");
    if (!file) {
        printf("Error opening disk image.\n");
        return false;
    }

    // Locate the target directory
    for (int i = 0; i < root.filecount; i++) {
        if (strcmp(root.files[i].filename, dirname) == 0 && root.files[i].isDirectory) {
            int startBlock = root.files[i].startBlock;

            // Read directory metadata
            DirectoryData dirData;
            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
            fread(&dirData, sizeof(DirectoryData), 1, file);

            // Check if the directory has space for more files
            if (dirData.childCount >= MAX_FILES) {
                printf("Directory '%s' is full.\n", dirname);
                fclose(file);
                return false;
            }

            // Open the file to be added
            FILE *inputFile = fopen(filename, "rb");
            if (!inputFile) {
                printf("Error opening input file: %s\n", filename);
                fclose(file);
                return false;
            }

            // Get the size of the input file
            fseek(inputFile, 0, SEEK_END);
            int fileSize = ftell(inputFile);
            fseek(inputFile, 0, SEEK_SET);

            // Allocate blocks for the file
            int blocksNeeded = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
            int fileStartBlock = -1;
            for (int j = 0, allocated = 0; j < NUM_BLOCKS && allocated < blocksNeeded; j++) {
                if (superblock.freeBlockBitmap[j] == 0) {
                    if (fileStartBlock == -1) fileStartBlock = j;
                    superblock.freeBlockBitmap[j] = 1; // Mark block as used
                    superblock.freeBlocks--;
                    allocated++;
                }
            }

            if (fileStartBlock == -1) {
                printf("No free blocks available to store file '%s'.\n", filename);
                fclose(file);
                fclose(inputFile);
                return false;
            }

            // Write file data to the allocated blocks
            fseek(file, sizeof(superblock) + sizeof(root) + (fileStartBlock * BLOCK_SIZE), SEEK_SET);
            char buffer[BLOCK_SIZE] = {0};
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, BLOCK_SIZE, inputFile)) > 0) {
                fwrite(buffer, 1, bytesRead, file);
            }

            // Add file metadata to the directory
            File newFile = {0};
            strncpy(newFile.filename, filename, sizeof(newFile.filename) - 1);
            newFile.filename[sizeof(newFile.filename) - 1] = '\0'; // Ensure null termination
            newFile.size = fileSize;
            newFile.startBlock = fileStartBlock;
            newFile.isDirectory = 0;

            dirData.children[dirData.childCount++] = newFile;

            // Write updated directory metadata back to disk
            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
            fwrite(&dirData, sizeof(DirectoryData), 1, file);

            // Update superblock and root
            fseek(file, 0, SEEK_SET);
            fwrite(&superblock, sizeof(Superblock), 1, file);
            fwrite(&root, sizeof(rootstruct), 1, file);

            printf("File '%s' added to directory '%s'.\n", filename, dirname);

            fclose(file);
            fclose(inputFile);
            return true;
        }
    }

    printf("Directory '%s' not found.\n", dirname);
    fclose(file);
    return false;
}


bool readFromDirectoryFile(char *dirname, char *filename) {
    FILE *file = fopen("disk.img", "rb");
    if (!file) {
        printf("Error opening disk image.\n");
        return false;
    }

    // Step 1: Locate the directory
    for (int i = 0; i < root.filecount; i++) {
        if (strcmp(root.files[i].filename, dirname) == 0 && root.files[i].isDirectory) {
            int startBlock = root.files[i].startBlock;

            // Step 2: Read the directory metadata
            DirectoryData dirData;
            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
            fread(&dirData, sizeof(DirectoryData), 1, file);

            // Step 3: Locate the file in the directory
            for (int j = 0; j < dirData.childCount; j++) {
                if (strcmp(dirData.children[j].filename, filename) == 0) {
                    int fileStartBlock = dirData.children[j].startBlock;
                    int fileSize = dirData.children[j].size;

                    // Step 4: Read the file's data
                    fseek(file, sizeof(superblock) + sizeof(root) + (fileStartBlock * BLOCK_SIZE), SEEK_SET);
                    char *buffer = (char *)malloc(fileSize + 1);
                    fread(buffer, 1, fileSize, file);
                    buffer[fileSize] = '\0';

                    printf("Contents of file '%s' in directory '%s':\n%s\n", filename, dirname, buffer);

                    free(buffer);
                    fclose(file);
                    return true;
                }
            }

            printf("File '%s' not found in directory '%s'.\n", filename, dirname);
            fclose(file);
            return false;
        }
    }

    printf("Directory '%s' not found.\n", dirname);
    fclose(file);
    return false;
}

bool listDirectoryContents(char *dirname) {
    FILE *file = fopen("disk.img", "rb");
    if (!file) {
        printf("Error opening disk image.\n");
        return false;
    }

    // Step 1: Locate the directory in the root
    for (int i = 0; i < root.filecount; i++) {
        if (strcmp(root.files[i].filename, dirname) == 0 && root.files[i].isDirectory) {
            int startBlock = root.files[i].startBlock;

            // Step 2: Read the directory metadata
            DirectoryData dirData;
            fseek(file, sizeof(superblock) + sizeof(root) + (startBlock * BLOCK_SIZE), SEEK_SET);
            fread(&dirData, sizeof(DirectoryData), 1, file);

            // Step 3: Print the contents of the directory
            printf("Contents of directory '%s':\n", dirname);
            for (int j = 0; j < dirData.childCount; j++) {
                printf(" + %s\n", dirData.children[j].filename);
            }

            fclose(file);
            return true;
        }
    }

    printf("Directory '%s' not found.\n", dirname);
    fclose(file);
    return false;
}
// if statements seperate args used functions to keep my main code clean
// I probably will create a .h and .c files for the functions so this doesnt get to 1000+ lines of reading
int main(int argc, char* argv[]) {

FILE* check = fopen("disk.img", "rb");
if (!check) {
     createfilesystem();
    printf("%s is your new virtual img", DISKNAME);
}
else {
    fread(&superblock, sizeof(superblock), 1, check);
    fread(&root, sizeof(root), 1, check);
    printf("Using + %s\n", DISKNAME);
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

if (strcmp(argv[1], "mkdir") == 0) {
    createdir(argv[2]);
}

if (strcmp(argv[1], "getdir") == 0) {
    readFromDirectoryFile(argv[2], argv[3]);
}

if (strcmp(argv[1], "diradd") == 0 ) {
    addFileToDirectory(argv[2], argv[3]);
}

if (strcmp(argv[1], "dirlist") == 0) {
    listDirectoryContents(argv[2]);
}

fclose(check);
}


// if you notice the random print statements, you can uncomment for more file data, I was using to debug my createfile()
