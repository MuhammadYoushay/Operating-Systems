#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#pragma pack(1) 

#define MEMSIZE 500  //Total Memory Size
#define MAXFRAMES 5  //Maximum number of frames
#define MINSTACKFRAMESIZE 10 //Minimum size of a frame
#define MAXSTACKFRAMESIZE 80 //Maximum size of a frame

struct framestatus {
	int number;               
  char name[8];             
	int functionaddress;      
	int frameaddress;         
	char used;                 
};

struct freelist {
	int start;                
	int size;                      
	struct freelist * next;        
};

struct allocated { //This structure was provided to us by Sir Tariq Kamal to maintain Heap allocation
  char name[8];
  int start;
  int size;  
};


void createframe(char[], int);
void deleteframe();
void createint(char[], int);
void createdouble(char[], double);
void createchar(char[], char);
void createbuffer(char[], int);
void deletebuffer(char[]);
void showmemory();


char memory[MEMSIZE]; //This is the memory array of size 500
int offset = 10;  //used to calculate the offset of the frame
int top = -1;  //used to keep track of the top frame
int stackSize = 105; //Initial Stack Size - 105 bytes are used by the frame status list
int maxStackSize = 300; //Maximum Stack Size
int maxHeapSize = 300;  //Maximum Heap Size
int heapSize = 0; //Initial Heap Size
int topFrame = 395; //Top Frame Address


struct framestatus* frameStatusList = (struct framestatus*) &memory[395]; //This is the frame status list which is stored at the top of the stack
struct freelist* head = NULL;   //This is the head of the free list
struct allocated alloc[18]; //This is the allocated list which is used to maintain the heap allocation, maximum 18 buffers can be allocated
bool offsetFlag = false; //This flag is used to check if the offset has been calculated or not

void initialize() {
  head = (struct freelist*)malloc(sizeof(struct freelist)); //Allocating memory to the head of the free list
  head->start = 0; //Setting the start address of the free list
  head->size = maxHeapSize; //Setting the size of the free list
  head->next = NULL; //Setting the next pointer of the free list to NULL
}

void createframe(char functionName[], int functionAddress) {
  printf("Stack Size Before creating the Stack-Frame --> %d\n",stackSize);
  //Checking if the maximum number of frames have been reached or not - maximum 5 frames can be created
  if (top >= MAXFRAMES - 1) {
    printf("Cannot create another frame, maximum number of frames have been reached\n");
    return;
  }
  //Checking if the function already exists or not - comparing the function name with the names of the functions in the frame status list
  for (int i = 0; i <= top; i++) {
    if (strcmp(frameStatusList[i].name, functionName) == 0) {
      printf("Function already exists\n");
      return;
    }
  }
  //Creating a new frame
  top++;
  frameStatusList[top].number = top;
  strcpy(frameStatusList[top].name, functionName);
  frameStatusList[top].functionaddress = functionAddress;
  stackSize = stackSize + MINSTACKFRAMESIZE;
  frameStatusList[top].frameaddress = MEMSIZE - stackSize;
  frameStatusList[top].used = true;
  if (frameStatusList[top].frameaddress < maxStackSize) {
    printf("Stack overflow, cannot create frame\n");
    top--;
    stackSize -= MINSTACKFRAMESIZE;
    return;
  }
  else {
    printf("Frame created successfully\n");
    printf("Frame %d - Created frame %s at address %d\n", top, functionName, frameStatusList[top].frameaddress);
    printf("Stack Size After creating the Stack-Frame --> %d\n",stackSize);
  }
}

void deleteframe() {
  int frameSize = 0;
  //Checking if the stack is empty or not
  if (top == -1) {
    printf("Stack is empty, no frames to delete\n");
    return;
  }
  int frameAddress = frameStatusList[top].frameaddress;
  printf("Frame Address --> %d\n", frameAddress);
  //Deleting the frame
  if(top>0){
  frameSize = frameStatusList[top - 1] .frameaddress - frameAddress;
  }
  else{
    frameSize = 395 - frameAddress;
  }
  printf("Frame Size --> %d\n", frameSize);
  //Setting the memory of the frame to 0 
  memset(memory + frameAddress, 0, frameSize); 
  printf("Deleting frame %s\n", frameStatusList[top].name);
  stackSize = stackSize - frameSize;
  top--;
  if (top >= 0) {
    frameStatusList[top].frameaddress = MEMSIZE - stackSize;
  }
  topFrame = topFrame + frameSize;
  printf("Frame deleted successfully\n");
}


void createint(char integerName[], int value) {
  //It will be created in the top most frame of the stack
  struct framestatus currentframe = frameStatusList[top];
  int currentframeaddress = currentframe.frameaddress;
  printf("Frame %d - Before creating integer %s, the frame is at address %d in the Stack\n", top, integerName, currentframeaddress);
  //Checking if the frame is full or not
  if (395 - currentframeaddress + 4> MAXSTACKFRAMESIZE) {
    printf("The frame is full, cannot create more data on it\n");
    return;
  }
    printf("Stack Size After creating the integer --> %d\n",stackSize);
    topFrame -= sizeof(int);
    char* valuePtr = (char*)&value; 
    //copying to the memory array
    memcpy(memory + topFrame, valuePtr, sizeof(int));
    printf("Created integer %s at address %d\n", integerName, topFrame);
    //Offset conditions - as initially we have to create a frame of 10 bytes, so when creating an int we need to use those 10 bytes first
    if(topFrame<currentframeaddress){
      int offset = currentframeaddress - topFrame;
      currentframeaddress = topFrame;
      if(offsetFlag == false){
        stackSize += offset;
        offsetFlag = true;
      }
      else{
        stackSize += sizeof(int);
      }
      printf("Stack Size Before creating the integer --> %d\n",stackSize);
    }
    printf("Frame %d - After creating integer %s, the frame is now at address %d in the Stack\n", top, integerName, currentframeaddress);
    frameStatusList[top] = currentframe;
    frameStatusList[top].frameaddress = currentframeaddress;
}

//create double func - same as create int - except the size changes to double
void createdouble(char doubleName[], double value) {
  struct framestatus currentframe = frameStatusList[top];
  int currentframeaddress = currentframe.frameaddress;
  printf("Frame %d - Before creating double %s, is at address %d in the Stack\n", top, doubleName, currentframeaddress);
  if (395 - currentframeaddress + 8> MAXSTACKFRAMESIZE) {
    printf("The frame is full, cannot create more data on it\n");
    return;
  }
    printf("Stack Size Before creating the double --> %d\n",stackSize);
    topFrame -= sizeof(double);


//This commented code is for typecasting double to string and then copying it to the memory array

      // char valuePtr[sizeof(double)];
      // sprintf(valuePtr, "%lf", value);
      char* valuePtr = (char*)&value;
      memcpy(memory + topFrame, valuePtr, sizeof(double));
    printf("Created double %s at address %d\n", doubleName, topFrame);
    if(topFrame<currentframeaddress){
      int offset = currentframeaddress - topFrame;
      currentframeaddress = topFrame;
      if(offsetFlag == false){
        stackSize += offset;
        offsetFlag = true;
      }
      else{
        stackSize += sizeof(double);
      }
      printf("Stack Size After creating the double --> %d\n",stackSize);
    }
    printf("Frame %d - After creating integer %s, is now at address %d in the Stack\n", top, doubleName, currentframeaddress);
    frameStatusList[top] = currentframe;
    frameStatusList[top].frameaddress = currentframeaddress;
}

//create char func - same as create int and create double - except the size changes to char
void createchar(char charName[], char value) {
    struct framestatus currentframe = frameStatusList[top];
    int currentframeaddress = currentframe.frameaddress;
    printf("Frame %d - Before creating char %s, the frame is at address %d in the Stack\n", top, charName, currentframeaddress);
    if (395 - currentframeaddress + 1 > MAXSTACKFRAMESIZE) {
        printf("The frame is full, cannot create more data on it\n");
        return;
    }
    printf("Stack Size Before creating the char --> %d\n", stackSize);
    topFrame -= sizeof(char);
    memcpy(memory + topFrame, &value, sizeof(char));
    printf("Created char %s at address %d\n", charName, topFrame);

    if (topFrame < currentframeaddress) {
        int offset = currentframeaddress - topFrame;
        currentframeaddress = topFrame;

        if (offsetFlag == false) {
            stackSize += offset;
            offsetFlag = true;
        }
        else {
            stackSize += sizeof(char);
        }

        printf("Stack Size After creating the char --> %d\n", stackSize);
    }

    printf("Frame %d - After creating char %s, the frame is now at address %d in the Stack\n", top, charName, currentframeaddress);
    frameStatusList[top] = currentframe;
    frameStatusList[top].frameaddress = currentframeaddress;
}

//function to create buffer on the heap
void createbuffer(char bname[], int size) {
  //Checking if the heap is full or not
  if (heapSize + size + 8 > maxHeapSize) {
    printf("The heap is full, cannot create more data\n");
    return;
  }
  //Maintaining the free list  - linke a linked list
  struct freelist* temp = head;
  struct freelist* prev = NULL;
  while (temp != NULL) {
    if (temp->size >= size+4)
      break;
    prev = temp;
    temp = temp->next;
  }
  //Checking if a large enough block is available or not in the free list
  if (temp == NULL) {
    printf("Could not find a large enough block\n");
    return;
  }
  //If enough memory in the free list is available then we create a buffer through the allocated list
  int index = 0;
  while (strlen(alloc[index].name) != 0)
    index++;
  strcpy(alloc[index].name, bname);
  alloc[index].start = temp->start;
  alloc[index].size = size+4;

  // int* localPointer = (int*)&memory[frameStatusList[top].frameaddress];
  //creating a local pointer to the bufffer address in the stack
  int localPointer = temp->start;
  createint(bname, localPointer);
  if (temp->size == size+4) {
    if (prev == NULL)
      head = temp->next;
    else
      prev->next = temp->next;
  } else {
    temp->start += (size+4);
    temp->size -= (size+4);
  }
  printf("Heap Size Before creating the buffer --> %d\n", heapSize);
  heapSize += size+4;
  int header = size;
  memcpy(memory + alloc[index].start, &header, 4);
  for (int i = 0; i < size; i++) {
    memory[alloc[index].start + 4 + i] = (char)rand();
  }
  printf("Allocated %d bytes for buffer %s in the Heap\n", size, bname);
  printf("Heap Size After creating the buffer --> %d\n", heapSize);
}

//function to delete buffer from the heap
void deletebuffer(char bname[]) {
  int index = 0;
  //Checking if the buffer exists or not
  while (strlen(alloc[index].name) != 0) {
    if (strcmp(alloc[index].name, bname) == 0)
      break;
    index++;
  }
  //If the buffer does not exist then we return
  if (strlen(alloc[index].name) == 0) {
    printf("Invalid buffer name\n");
    return;
  }
  int size = alloc[index].size;
  int startAddress = alloc[index].start;
  //Setting the memory of the buffer to 0
  struct freelist* temp = (struct freelist*)malloc(sizeof(struct freelist));
  temp->start = startAddress;
  temp->size = size;
  temp->next = head;
  head = temp;
  for (int i = 0; i < size; i++) {
    memory[startAddress + i] = 0;
  }
  //Deleting the buffer from the allocated list
  strcpy(alloc[index].name, "");
  alloc[index].start = 0;
  alloc[index].size = 0;
  heapSize -= size;
  printf("Deleted buffer %s\n", bname);
}

void showmemory() {
    //Double d taken to print double values in the memory
    double d;
    char command[2];
    int count = 0;
    printf("\n");
    printf("Memory Printing format -->\n");
    printf("Enter I to print values in the form of Integers (The character and doubles will also be represented in Integers):\nEnter C to print values in the form of Chars (The Integers and Doubles will also be represented in Integers):\n Enter H to print values in the form of Hexadecimal (The Integers, Doubles Chars all also be represented in Hexadecimal):\nEnter D to print values in the form of Doubles (The Integers and Chars will also be represented in Doubles)");
    printf("\nInput -->");
    scanf("%s", command);
    if (strcmp(command, "I") == 0 || strcmp(command, "D") == 0 || strcmp(command, "C") == 0 || strcmp(command, "H") == 0) {
        printf("Stack Frame List:\n");
        for (int i = 0; i <= top; i++) {
            printf("Frame %d: Name - %s, Function Address - %d, Frame Address - %d\n",
                   frameStatusList[i].number,
                   frameStatusList[i].name,
                   frameStatusList[i].functionaddress,
                   frameStatusList[i].frameaddress);
        }
        printf("\nMemory Contents:\n");
        for (int i = 395; i >= 0; i--) {
            if (strcmp(command, "I") == 0) {
                printf("%d --> %d\n", i, memory[i]);
            }  else if (strcmp(command, "C") == 0) {
                printf("%d --> %c\n", i, memory[i]);
            }
            else if (strcmp(command, "H") == 0) {
                printf("%d --> %x\n", i, memory[i]);
            }
            else if (strcmp(command, "D") == 0) {
              memcpy(&d, &memory[i - sizeof(double)], sizeof(double));
              printf("%d --> %lf\n", i, d);
            }  
        }
    } else {
        printf("Invalid command\n");
    }
    printf("\nHeap Details:\n");
    printf("----------------\n");
      struct freelist* temp = head;
  int heapAddress = 0;

  while (temp != NULL) {
    printf("Free Block at address %d, size %d\n", heapAddress + temp->start, temp->size);
    temp = temp->next;
  }
  for (int i = 0; i < 10; i++) {
    if (strlen(alloc[i].name) != 0) {
      printf("Allocated Buffer %s at address %d, size %d\n", alloc[i].name, alloc[i].start, alloc[i].size);
    }
  }
}


int main() {
  initialize();
  memset(memory, 0 , MEMSIZE);
  while (1) {
    char command[10], name[10];
    int address, size, value;
    double dvalue;
    char cvalue;

    printf("\nEnter command: ");
    scanf("%s", command);
    if (strcmp(command, "CF") == 0) {
      scanf("%s %d", name, &address);
      createframe(name, address);
    } else if (strcmp(command, "CI") == 0) {
      scanf("%s %d", name, &value);
      createint(name, value);
    } else if (strcmp(command, "CD") == 0) {
      scanf("%s %lf", name, &dvalue);
      createdouble(name, dvalue);
    } else if (strcmp(command, "CH") == 0) {
      scanf("%s %d", name, &size);
      createbuffer(name, size);
    } else if (strcmp(command, "CC") == 0) {
      scanf("%s %c", name, &cvalue);
      createchar(name, cvalue);
    } 
    else if (strcmp(command, "DH") == 0) {
      scanf("%s", name);
      deletebuffer(name);
    } else if (strcmp(command, "SM") == 0) {
      showmemory();
    }else if (strcmp(command, "DF") == 0) {
  deleteframe();
}
    else {
      printf("Invalid command\n");
    }
  }
  free(head); 
  return 0;
}