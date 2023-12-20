#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define FILENAME_MAXLEN 8  // including the NULL char
#define No_Of_Blocks 128 
#define Block_Size 1024
#define Max_Blocks 8
#define Total_Inodes 16
int myfs;

/* 
 * inode 
 */

typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [8];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;


/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;


int initialize_fileSystem() //This function basically returns fd of the file system like we studied in class.
{
    myfs = open("myfs", O_CREAT | O_RDWR, 0222); //Opening the file in read-write mode
    char buffer[Block_Size *No_Of_Blocks]; //Initializing it to null characters
    memset(buffer, '\0', sizeof(buffer));
    buffer[0] = 'S'; //Superblock
    buffer[1] = (char)1; 
    struct inode root;
    root.dir = 1;  
    strcpy(root.name, "/"); //root inode
    root.size = sizeof(struct dirent);  
    root.blockptrs[0] = 1;  
    root.used = 1;  
    root.rsvd = 0; 
    memcpy(buffer + No_Of_Blocks, &root, sizeof(struct inode)); //copying root inode to buffer
    struct dirent rootdirent;
    strcpy(rootdirent.name, "."); //Entry for root directory
    rootdirent.namelen = 1; 
    rootdirent.inode = 0;
    memcpy(buffer + Block_Size, &rootdirent, sizeof(struct dirent));
    if (write(myfs, buffer, sizeof(buffer)) == -1) { //Writing the buffer to the file
        perror("error: cannot write to ./myfs");
        close(myfs);
        return 1;
    }
    return myfs;
}

int GetParentInode(char *filename) {//This function is a helper fuunction that we'll use. I returns the inode of the parent directory of the file/directory specified in the absolute path.
	if (strcmp(filename, "/") == 0) {
    return -2; //If root directory, then return -2.
  };
	char currentDir[100];
	int parentInode = 0;
	struct inode Current_Inode;
	struct dirent DirEntry;
	while (sscanf(filename, "/%[^/]%s", currentDir, filename) == 2) { //Took help from chat gpt here to use sscanf. What it basically does is that it scans the string filename and stores the first part before the '/' in currentDir and the rest in filename. If it is not able to do so, then it returns 1.
		lseek(myfs, No_Of_Blocks + parentInode * sizeof(inode), SEEK_SET);
		read(myfs, (char*)&Current_Inode, sizeof(inode));
		int DirSize = Current_Inode.size;
		for (int i = 0; i < DirSize; i += sizeof(dirent)) {//Iterate through current parent's directory
			lseek(myfs, Block_Size * Current_Inode.blockptrs[0] + i, SEEK_SET); //lseek is used to go to the specified position in the file.
			read(myfs, (char*)&DirEntry, sizeof(dirent));
			if (strcmp(DirEntry.name, currentDir) == 0) { //Check for match
				lseek(myfs, No_Of_Blocks + DirEntry.inode * sizeof(inode), SEEK_SET); //Using lseek to got to the inode of the matched dir
				read(myfs, (char*)&Current_Inode, sizeof(inode));
				if (Current_Inode.dir == 1) { //Check if the matched dir ia dir or not
					parentInode = DirEntry.inode;
					goto DIR_MATCHED;
				}
			}
		}
		printf("error: the directory %s in the given path does not exist\n", currentDir);
		return -1;
	DIR_MATCHED:;
	}

	sscanf(filename, "/%s", filename);
	return parentInode;
}

int GetFreeInode() { //This is another helper function. It just checks that the inodes are free or not
	struct inode new_inode[Total_Inodes];
	lseek(myfs, No_Of_Blocks, SEEK_SET);
	read(myfs, (char*)new_inode, Total_Inodes * sizeof(inode));
	for (int i=0; i<Total_Inodes; i++) {
		if (new_inode[i].used == 0) return i;
	}
	printf("error: all inodes occupied\n");
	return -1;
}

int GetFreeBlocks(int *blocks, int n) {//This is another helper function. It just checks that the blocks are free or not
	int CurrentBlock=1;
	char full[No_Of_Blocks];
	lseek(myfs, 0, SEEK_SET);
	read(myfs, full, 128);
	for (int i=0; i<n; i++) {
		while(CurrentBlock <= No_Of_Blocks) {
			if (full[CurrentBlock] != (char)1) goto FOUND_BLOCK;
			CurrentBlock++;
		}
		printf("error: not enough space\n");
		return -1;
FOUND_BLOCK:
		blocks[i] = CurrentBlock;
		CurrentBlock++;
	}
	return 0; //If free blocks found, then return 0.
}

int CD(char *dirname) { //Code for creating a directory
    int DirNode = GetParentInode(dirname); //Get the inode of the parent directory. We've used the helper function here that we created aboe
    if (DirNode == -1) return 1;
    if (DirNode == -2) { // If the user just gives '/' to be created/
        printf("Error -- Root Directory is already present\n");
        return 1;
    }
    int FreeBlock;
    int return_val = GetFreeBlocks(&FreeBlock, 1); //Here we are getting a free block using the helper we created above
    if (return_val == -1) return 1; //If no free block found, then return 1
    int Free_Inode = GetFreeInode(); //Here we are getting a free inode using the helper we created above
    if (Free_Inode == -1) return 1; //If no free inode found, then return 1

    struct inode directorynode; //Initializaing the inode of the directory to be created
    directorynode.dir = 1; 
    strcpy(directorynode.name, dirname);
    directorynode.size = 0;  
    directorynode.blockptrs[0] = FreeBlock;  
    directorynode.used = 1; 
    directorynode.rsvd = 0; 
    char one = (char)1; //Marking the block as taken
    lseek(myfs, FreeBlock, SEEK_SET);
    write(myfs, &one, 1);
    lseek(myfs, No_Of_Blocks + Free_Inode * sizeof(inode), SEEK_SET); //Writing the Directory Inode to the file system
    write(myfs, (char *) &directorynode, sizeof(inode));
    struct inode new_inode;
    struct dirent new_dirent;
    lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET); //Readind parent dirs inode
    read(myfs, (char *) &new_inode, sizeof(inode));
    int DirSize = new_inode.size;
    int Block= new_inode.blockptrs[0];
    for (int i = 0; i < DirSize; i += sizeof(dirent)) { //Checking if the dir already exists
        lseek(myfs, Block_Size * Block+ i, SEEK_SET);
        read(myfs, (char *) &new_dirent, sizeof(dirent));
        if (strcmp(new_dirent.name, dirname) == 0) {
            struct inode second_inode;
            lseek(myfs, No_Of_Blocks + new_dirent.inode * sizeof(inode), SEEK_SET);
            read(myfs, (char *) &second_inode, sizeof(inode));
            if (second_inode.dir == 1) {
                printf("Error -- Directory already exists\n");
                return 1;
            }
        }
    }
    lseek(myfs, Block_Size * Block+ DirSize, SEEK_SET); //Adding the new directory entry to the parent directory
    new_dirent.namelen = strlen(dirname);
    strcpy(new_dirent.name, dirname);
    new_dirent.inode = Free_Inode;
    write(myfs, (char *) &new_dirent, sizeof(dirent));
    new_inode.size += sizeof(dirent); //Updating the size of the parent directory, after we've added
    lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
    write(myfs, (char *) &new_inode, sizeof(inode));
    lseek(myfs, No_Of_Blocks + Free_Inode * sizeof(inode), SEEK_SET); //updating the '..' entry in the new directory
    read(myfs, (char *) &new_inode, sizeof(inode));
    lseek(myfs, Block_Size * new_inode.blockptrs[0] + new_inode.size, SEEK_SET);
    new_dirent.namelen = 2;  
    strcpy(new_dirent.name, "..");
    new_dirent.inode = DirNode;
    write(myfs, (char *) &new_dirent, sizeof(dirent));
    new_inode.size += sizeof(dirent);
    lseek(myfs, No_Of_Blocks + Free_Inode * sizeof(inode), SEEK_SET);
    write(myfs, (char *) &new_inode, sizeof(inode));
    lseek(myfs, Block_Size * new_inode.blockptrs[0] + new_inode.size, SEEK_SET); //updating the '.' entry in the new directory
    new_dirent.namelen = 1; 
    strcpy(new_dirent.name, ".");
    new_dirent.inode = Free_Inode;
    write(myfs, (char *) &new_dirent, sizeof(dirent));
    new_inode.size += sizeof(dirent);
    lseek(myfs, No_Of_Blocks + Free_Inode * sizeof(inode), SEEK_SET);
    write(myfs, (char *) &new_inode, sizeof(inode));
    return 0;
}

int CR(char *filename, int size) { //Code for creating a file
  struct inode FileInode; // Initializing the inode of the file to be created
  int DirNode = GetParentInode(filename); //Get the inode of the parent directory. We've used the helper function here that we created aboe
  if (DirNode == -1) return 1; //If no parent directory found, then return 1
  if (DirNode == -2) { // If the user just gives '/' to be created
    printf("Error --  Invalid filename\n"); 
    return 1;
  }
  int blockcount = (size/Block_Size) + (size%Block_Size!=0); // Calculating the number of blocks required to store the file
  if (blockcount > Max_Blocks)
  {
    int mb = Max_Blocks;
    printf("Error -- Exceeding limit the file size limit\n", mb);
    return 1;
  }
  int return_value = GetFreeBlocks(FileInode.blockptrs, blockcount); //Here we are getting free blocks using the helper we created above
  if (return_value == -1) return 1; //If no free block found, then return 1
  int Free_Inode = GetFreeInode(); //Here we are getting a free inode using the helper we created above
  if (Free_Inode == -1) return 1; //If no free inode found, then return 1

  struct inode new_inode;  //Searching the file in the directory. Help taken here
  struct inode second_inode;
  struct dirent new_dirent;
  lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
  read(myfs, (char*)&new_inode, sizeof(inode));
  int DirSize = new_inode.size;
  int Block= new_inode.blockptrs[0];
  for(int i=0; i<DirSize; i+=sizeof(dirent)) {
    lseek(myfs, Block_Size * Block+ i, SEEK_SET);
    read(myfs, (char*)&new_dirent, sizeof(dirent));
    
    if (strcmp(new_dirent.name, filename) == 0) {
      lseek(myfs, No_Of_Blocks + new_dirent.inode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&second_inode, sizeof(inode));
      
      if (second_inode.dir == 0) {
        printf("error: the file already exists\n");
        return -1;
      }
    }
  }
  lseek(myfs, Block_Size * Block + DirSize, SEEK_SET); // Addin to the current directory - the new file weve created
  new_dirent.namelen = strlen(filename);
  strcpy(new_dirent.name, filename);
  new_dirent.inode = Free_Inode;
  write(myfs, (char*)&new_dirent, sizeof(dirent));
  new_inode.size += sizeof(dirent);
  lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
  write(myfs, (char*)&new_inode, sizeof(inode));
  FileInode.dir = 0; //not a dir
  strcpy(FileInode.name, filename);
  FileInode.size = size;
  FileInode.used = 1;
  FileInode.rsvd = 0;
  lseek(myfs, No_Of_Blocks + Free_Inode * sizeof(inode), SEEK_SET); //Adding the new file inode to the file system
  write(myfs, (char*)&FileInode, sizeof(inode));

  char one = (char)1;
  char blockdata[Block_Size];
  int buffsize = Block_Size;
  
  for (int i=0; i<blockcount; i++) { //Writing the file data to the blocks
    lseek(myfs, FileInode.blockptrs[i], SEEK_SET);
    write(myfs, &one, 1);

    if (size > Block_Size) size -= Block_Size;
    else buffsize = size;
    for (int k=0; k<buffsize; k++)
      blockdata[k] = (char) (97 + ((k + i * Block_Size) % 26));

    lseek(myfs, Block_Size * FileInode.blockptrs[i], SEEK_SET);
    write(myfs, blockdata, buffsize);
  }

  return 0;
}

int DL(char *filename) {
    int parentInode = GetParentInode(filename); //Get the inode of the parent directory. We've used the helper function here that we created aboe
    if (parentInode == -1 || parentInode == -2) { //If there is a directory in the path thats missing or root
        return 1;
    }
    struct inode parentInodeStruct;
    struct dirent DirEntry;
    lseek(myfs, No_Of_Blocks + parentInode * sizeof(inode), SEEK_SET);
    read(myfs, (char*)&parentInodeStruct, sizeof(inode));
    int DirSize = parentInodeStruct.size;
    int Block= parentInodeStruct.blockptrs[0];
    int found = 0;
    int i;
    for (i = 0; i < DirSize; i += sizeof(dirent)) { //Iterating to find the specified file
        lseek(myfs, Block_Size * Block+ i, SEEK_SET);
        read(myfs, (char*)&DirEntry, sizeof(dirent));
        if (strcmp(DirEntry.name, filename) == 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Error -- File does not exist\n");
        return 1;
    }
    struct inode fileInode; // When deleted marking the inode and blocks as unused
    lseek(myfs, No_Of_Blocks + DirEntry.inode * sizeof(inode), SEEK_SET);
    read(myfs, (char*)&fileInode, sizeof(inode));
    fileInode.used = 0;
    lseek(myfs, No_Of_Blocks + DirEntry.inode * sizeof(inode), SEEK_SET);
    write(myfs, (char*)&fileInode, sizeof(inode));

    char ZERO = (char)0;
    for (i = 0; i < Max_Blocks; i++) {
        if (fileInode.blockptrs[i] != 0) { 
            lseek(myfs, fileInode.blockptrs[i], SEEK_SET);
            write(myfs, &ZERO, 1);
        }
    }
    memset(&DirEntry, 0, sizeof(dirent));
    lseek(myfs, Block_Size * Block+ i, SEEK_SET);
    write(myfs, (char*)&DirEntry, sizeof(dirent)); //Removing dir entry. We have zeroed its content
    parentInodeStruct.size -= sizeof(dirent); //Re updating the size of parent directory
    lseek(myfs, No_Of_Blocks + parentInode * sizeof(inode), SEEK_SET);
    write(myfs, (char*)&parentInodeStruct, sizeof(inode));
    return 0;
}

int DeleteDirectoryHelper(int inodeIndex) { //This is a helper function that Ive created. Ill be using it for recursive deletion
    struct inode del_Inode;
    lseek(myfs, No_Of_Blocks + inodeIndex * sizeof(inode), SEEK_SET); //Reading the inode
    read(myfs, (char*)&del_Inode, sizeof(inode));
    for (int i = 0; i < Max_Blocks; i++) { //Performing recursive deletion here
        if (del_Inode.blockptrs[i] != 0) {
            struct dirent entry;
            for (int j = 0; j < Block_Size; j += sizeof(dirent)) {
                lseek(myfs, Block_Size * del_Inode.blockptrs[i] + j, SEEK_SET);
                read(myfs, (char*)&entry, sizeof(dirent));
                if (entry.inode != 0) {
                    if (entry.name[0] == '.') { //Skip if '.' encountered
                        continue;
                    }
                    struct inode entryInode;
                    lseek(myfs, No_Of_Blocks + entry.inode * sizeof(inode), SEEK_SET);
                    read(myfs, (char*)&entryInode, sizeof(inode));
                    if (entryInode.dir == 1) {
                        DeleteDirectoryHelper(entry.inode);
                    } else { //Deletion
                        entryInode.used = 0;
                        lseek(myfs, No_Of_Blocks + entry.inode * sizeof(inode), SEEK_SET);
                        write(myfs, (char*)&entryInode, sizeof(inode));
                    }
                    memset(&entry, 0, sizeof(dirent)); //removing the directory entry
                    lseek(myfs, Block_Size * del_Inode.blockptrs[i] + j, SEEK_SET);
                    write(myfs, (char*)&entry, sizeof(dirent));
                }
            }
            char ZERO = (char)0; //Freeing the blocks by Zeroing like we did for dl above
            lseek(myfs, del_Inode.blockptrs[i], SEEK_SET);
            write(myfs, &ZERO, 1);
        }
    }
    del_Inode.used = 0; //Makring the inodes free
    lseek(myfs, No_Of_Blocks + inodeIndex * sizeof(inode), SEEK_SET);
    write(myfs, (char*)&del_Inode, sizeof(inode));
    return 0;
}

int DD(char *dirname) {
    int result; // Will use this later in the function, just declaring it here
    int parentInode = GetParentInode(dirname); //Get the inode of the parent directory. We've used the helper function here that we created aboe
    if (parentInode == -1 || parentInode == -2) { //If there is a directory in the path thats missing or root
        return 1;
    }
    struct inode parentInodeStruct;
    struct dirent DirEntry;
    lseek(myfs, No_Of_Blocks + parentInode * sizeof(inode), SEEK_SET);
    read(myfs, (char*)&parentInodeStruct, sizeof(inode));
    int DirSize = parentInodeStruct.size;
    int Block= parentInodeStruct.blockptrs[0];
    int found = 0;
    int i;
    for (i = 0; i < DirSize; i += sizeof(dirent)) { //Finding the specified directory
        lseek(myfs, Block_Size * Block+ i, SEEK_SET);
        read(myfs, (char*)&DirEntry, sizeof(dirent));
        if (strcmp(DirEntry.name, dirname) == 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Error -- Directory does not exist\n");
        return 1;
    }

    result = DeleteDirectoryHelper(DirEntry.inode); // Using the helper function we created above to delete recursively
    if (result != 0) {
        printf("Error: Failed to delete directory.\n");
        return 1;
    }

    memset(&DirEntry, 0, sizeof(dirent)); // Removing the directory entry
    lseek(myfs, Block_Size * Block+ i, SEEK_SET);
    write(myfs, (char*)&DirEntry, sizeof(dirent));
    parentInodeStruct.size -= sizeof(dirent); //Updating the size of the parent directory
    lseek(myfs, No_Of_Blocks + parentInode * sizeof(inode), SEEK_SET);
    write(myfs, (char*)&parentInodeStruct, sizeof(inode));
    return 0;
}

int CP(char *srcname, char *dstname) { //Code for copying a file from src to dst
      int DirNode = GetParentInode(srcname); //Get the inode of the parent directory. We've used the helper function here that we created aboe
      if (DirNode == -1) return 1;//If no parent directory found, then return 1
      if (DirNode == -2) {// If the user just gives '/'
        printf("Error -- Cannot copy root\n");
        return 1;  
      }
      int src_block; //Working on the src
      struct inode src_del_Inode;
      lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&src_del_Inode, sizeof(inode));
      src_block = src_del_Inode.blockptrs[0];
      int src_size = src_del_Inode.size;

      int src_entry = -1;
      int src_Free_Inode;  
      struct dirent src_dirent;
      for(int i=0; i<src_size; i+=sizeof(dirent)) {
        lseek(myfs, Block_Size * src_block + i, SEEK_SET);
        read(myfs, (char*)&src_dirent, sizeof(dirent));
        if (strcmp(src_dirent.name, srcname) == 0 ) {
          src_entry = i;
          src_Free_Inode = src_dirent.inode;
          break;
        }
      }
      if (src_entry == -1) { //if srs not found
        printf("Error -- the source file does not exist\n");
        return 1;
      }
      struct inode src_inode; //If src is a directory
      lseek(myfs, No_Of_Blocks + src_Free_Inode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&src_inode, sizeof(inode));
      if (src_inode.dir == 1) {
        printf("Error -- cannot handle directories\n");
        return 1;
      }
      DirNode = GetParentInode(dstname); // Working on the dest now
      if (DirNode == -1) return 1; //Foloowing the same procedure as above for src
      if (DirNode == -2) {
        printf("Error -- Cannot copy to root\n");
        return 1;
      }
      int dest_block;
      struct inode dest_del_Inode;
      lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&dest_del_Inode, sizeof(inode));
      dest_block = dest_del_Inode.blockptrs[0];
      int dest_size = dest_del_Inode.size;

      int dest_entry = -1;
      int dest_Free_Inode;
      struct dirent dest_dirent;
      for(int i=0; i<dest_size; i+=sizeof(dirent)) {
        lseek(myfs, Block_Size * dest_block + i, SEEK_SET);
        read(myfs, (char*)&dest_dirent, sizeof(dirent));
        if (strcmp(dest_dirent.name, dstname) == 0 ) {
          dest_entry = i;
          dest_Free_Inode = dest_dirent.inode;
          break;
        }
      }
      
      if (dest_entry != -1) {  //if the dst is already thr,we'll delete it
        struct inode dest_del_Inode;
        lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
        read(myfs, (char*)&dest_del_Inode, sizeof(inode));
        int dest_DirSize = dest_del_Inode.size;
        
        struct dirent dest_new_dirent;
        int dest_Block= dest_del_Inode.blockptrs[0];
        lseek(myfs, Block_Size * dest_Block+ dest_DirSize - sizeof(dirent), SEEK_SET);
        read(myfs, (char*)&dest_new_dirent, sizeof(dirent));
        
        dest_del_Inode.size = dest_DirSize - sizeof(dirent);
        lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
        write(myfs, (char*)&dest_del_Inode, sizeof(inode));
        
      }
      
      struct inode new_del_Inode;  //new dst enteries in directory
      struct dirent new_dirent;
      lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&new_del_Inode, sizeof(inode));
      int new_DirSize = new_del_Inode.size;
      int new_Block= new_del_Inode.blockptrs[0];
      strcpy(new_dirent.name, dstname);
      new_dirent.namelen = strlen(dstname);
      new_dirent.inode = src_Free_Inode;
      lseek(myfs, Block_Size * new_Block+ new_DirSize, SEEK_SET);
      write(myfs, (char*)&new_dirent, sizeof(dirent));
      new_del_Inode.size = new_DirSize + sizeof(dirent);
      lseek(myfs, No_Of_Blocks + DirNode * sizeof(inode), SEEK_SET);
      write(myfs, (char*)&new_del_Inode, sizeof(inode));
      
      struct inode copy_inode; //creating copy
      lseek(myfs, No_Of_Blocks + src_Free_Inode * sizeof(inode), SEEK_SET);
      read(myfs, (char*)&copy_inode, sizeof(inode));
      
      int blockcount = (copy_inode.size/Block_Size) + (copy_inode.size%Block_Size != 0); //copying blocks data
      int blocks[blockcount];
      for (int i=0; i<blockcount; i++)
      blocks[i] = copy_inode.blockptrs[i];  
      strcpy(copy_inode.name, dstname);
      int new_inode = GetFreeInode();
      if (new_inode == -1) return 1;
      int return_value = GetFreeBlocks(copy_inode.blockptrs, blockcount);
      if (return_value == -1) return 1;
      lseek(myfs, No_Of_Blocks + new_inode * sizeof(inode), SEEK_SET);
      write(myfs, (char*)&copy_inode, sizeof(inode));
      char blockdata[Block_Size];
      for (int i=0; i<blockcount; i++) {
        lseek(myfs, Block_Size * blocks[i], SEEK_SET);
        read(myfs, blockdata, Block_Size);
        lseek(myfs, Block_Size * copy_inode.blockptrs[i], SEEK_SET);
        write(myfs, blockdata, Block_Size);
      }
    
      return 0;
    }

int MV(char *srcname, char *dstname) {
  int src_DirNode = GetParentInode(srcname); //Get the inode of the parent directory. We've used the helper function here that we created aboe
  if (src_DirNode == -1) return 1; //If no parent directory found, then return 1
  if (src_DirNode == -2) { // If the user just gives '/' 
    printf("Error -- Root directory is invalid srcname\n"); 
    return 1;
  }

  int src_block;
  struct inode src_inode;
  lseek(myfs, No_Of_Blocks + src_DirNode * sizeof(inode), SEEK_SET); //Reading the inode of the source file parent directory
  read(myfs, (char*)&src_inode, sizeof(inode));
  src_block = src_inode.blockptrs[0];
  int src_size = src_inode.size;

  int src_entry = -1;
  int src_Free_Inode;
  struct dirent src_dirent;
  for(int i=0; i<src_size; i+=sizeof(dirent)) { //finding the source file
    lseek(myfs, Block_Size * src_block + i, SEEK_SET);
    read(myfs, (char*)&src_dirent, sizeof(dirent));
    if (strcmp(src_dirent.name, srcname) == 0 ) {
      src_entry = i;
      src_Free_Inode = src_dirent.inode;
      break;
    }
  }
  if (src_entry == -1) {
    printf("Error -- File does not exist\n");
    return 1;
  }

  struct inode src_fileinode;
  lseek(myfs, No_Of_Blocks + src_Free_Inode * sizeof(inode), SEEK_SET);
  read(myfs, (char*)&src_fileinode, sizeof(inode));
  if (src_fileinode.dir == 1) {
    printf("Error -- Youv'e entered a directory at the source\n"); 
    return 1;
  }
  
  int dest_DirNode = GetParentInode(dstname); //Doing the same with dest what we did with src
  if (dest_DirNode == -1) return 1;
  if (dest_DirNode == -2) {
    printf("Error -- Root directory is invalid dst name\n");
    return 1;
  }

  int dest_block;
  struct inode dest_inode;
  lseek(myfs, No_Of_Blocks + dest_DirNode * sizeof(inode), SEEK_SET);
  read(myfs, (char*)&dest_inode, sizeof(inode));
  dest_block = dest_inode.blockptrs[0];
  int dest_size = dest_inode.size;

  int dest_entry = -1;
  struct dirent dest_dirent;
  for(int i=0; i<dest_size; i+=sizeof(dirent)) { //finding the dst file
    lseek(myfs, Block_Size * dest_block + i, SEEK_SET);
    read(myfs, (char*)&dest_dirent, sizeof(dirent));
    if (strcmp(dest_dirent.name, dstname) == 0 ) {
      dest_entry = i;
      break;
    }
  }
  
  if (dest_entry != -1) {  //deleting if already exists
    dest_inode.size -= sizeof(dirent);  
    lseek(myfs, Block_Size * dest_block + dest_entry, SEEK_SET);
    write(myfs, (char*)&dest_dirent, sizeof(dirent)); 
    lseek(myfs, No_Of_Blocks + dest_DirNode * sizeof(inode), SEEK_SET);
    write(myfs, (char*)&dest_inode, sizeof(inode));
  }
  dest_dirent.namelen = strlen(dstname); //Adding new entry
  strcpy(dest_dirent.name, dstname);
  dest_dirent.inode = src_Free_Inode;
  
  dest_inode.size += sizeof(dirent);
  lseek(myfs, Block_Size * dest_block + dest_size, SEEK_SET);
  write(myfs, (char*)&dest_dirent, sizeof(dirent));
  lseek(myfs, No_Of_Blocks + dest_DirNode * sizeof(inode), SEEK_SET);
  write(myfs, (char*)&dest_inode, sizeof(inode));

  src_inode.size -= sizeof(dirent); //deleteing the src entry
  lseek(myfs, Block_Size * src_block + src_entry, SEEK_SET);
  write(myfs, (char*)&src_dirent, sizeof(dirent));
  lseek(myfs, No_Of_Blocks + src_DirNode * sizeof(inode), SEEK_SET);
  write(myfs, (char*)&src_inode, sizeof(inode));
  return 0;
}

void LL() {
  struct inode inode;
  for(int i = 0; i < Total_Inodes; i++) { //Iterating through all the inodes
    lseek(myfs, No_Of_Blocks + i * sizeof(inode), SEEK_SET); 
    read(myfs, (char*)&inode, sizeof(inode));
    
    if(inode.used == 1) { //if in use -- print
      printf("%s ", inode.name);
      if(inode.dir == 1) { //if dir then print that it is a dir
        printf("(this is a dir)\n"); 
      } else {
        printf("%d\n", inode.size); //size of the file
      }
    }
  }
}


int main(int argc, char* argv[]) {
    char command[100];
    int result;
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    FILE* inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("Error: Could not open the input file.\n");
        return 1;
    }
    int myfs = initialize_fileSystem();
    while (fgets(command, sizeof(command), inputFile) != NULL) {
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "exit") == 0) {
            close(myfs);
            break;
        } else if (strncmp(command, "CD ", 3) == 0) {
            char* dirname = command + 3;
            result = CD(dirname);
            if (result != 0) {
                printf("Error: Failed to change directory.\n");
            } else {
                printf("Done\n");
            }
        } else if (strncmp(command, "CP ", 3) == 0) {
            char srcname[100];
            char dstname[100];
            sscanf(command + 3, "%s %s", srcname, dstname);
            result = CP(srcname, dstname);
            if (result != 0) {
                printf("Error: Failed to copy file.\n");
            } else {
                printf("Done\n");
            }
        } else if (strncmp(command, "DD ", 3) == 0) {
            char dirname[100];
            sscanf(command + 3, "%s", dirname);
            result = DD(dirname);
            if (result != 0) {
                printf("Error: Failed to delete directory.\n");
            } else {
                printf("Done\n");
            }
        } else if (strncmp(command, "CR ", 3) == 0) {
            char filename[100];
            int size;
            sscanf(command + 3, "%s %d", filename, &size);
            result = CR(filename, size);
            if (result != 0) {
                printf("Error: Failed to create file.\n");
            } else {
                printf("Done\n");
            }
        } else if (strncmp(command, "DL ", 3) == 0) {
            char filename[100];
            sscanf(command + 3, "%s", filename);
            result = DL(filename);
            if (result != 0) {
                printf("Error: Failed to delete file.\n");
            } else {
                printf("Done\n");
            }
        } else if (strncmp(command, "MV ", 3) == 0) {
            char srcname[100];
            char dstname[100];
            sscanf(command + 3, "%s %s", srcname, dstname);
            result = MV(srcname, dstname);
            if (result != 0) {
                printf("Error: Failed to move file.\n");
            } else {
                printf("Done\n");
            }
        } else if (strcmp(command, "LL") == 0) {
            LL();
            printf("Done\n");
        } else {
            printf("Error: Invalid command: %s\n", command);
        }
    }
    fclose(inputFile);
    return 0;
}