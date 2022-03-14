/*
 * support.h -- object representation and support functions
 */


#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/* support functions */

void fatalError(char *msg);   /* print a message and exit */
void * newPrimObject(int dataSize); /* create a new primitive object */
void * getPrimObjectDataPointer(void * primObject);

#endif /* _SUPPORT_H_ */
#define B_heart (1<<(8*sizeof(unsigned int)-2))
#define MSB (1 << (8 * sizeof ( unsigned int) - 1)) //= 0x80000000
#define IS_PRIMITIVE(objRef) (((objRef)->size & MSB) == 0)
#define GET_ELEMENT_COUNT(objRef) ((objRef)->size & ~MSB) //GET_SIZE
#define GET_REFS_PTR(objRef) ((ObjRef *)(objRef)->data)
#define IS_BROKENHEART(objRef)  (((objRef)->size&B_heart)!=0)
#define FORWARD_OFFSET(objRef)  ((objRef)->size&~((MSB) | (B_heart)))
#define GET_SIZE(objRef)    ((objRef)->size& ~((MSB) | (B_heart)))


typedef struct {
    unsigned int size; // # byte of payload
    unsigned char data[1]; //payload data, size as needed!
} *ObjRef;                     

typedef struct {
    bool isObjRef;
    union {
        ObjRef objRef; 
        int number;
    } u;
} StackSlot;

typedef struct Heap{
    char *start1;
    char *start2;
    int heapSize;
    char *freeSize;
    char * end1;
    char * end2;
    char * scan;
    char * heap2;
}Heap;


struct Heap heap;
StackSlot *stack;
ObjRef ** staticData;
ObjRef * returnRegister;


long  stackSize = 4096;
long  heapSize = 8388608;
unsigned int breakpoint;
unsigned int instr[1]; 
unsigned int SC = 0; 
unsigned int IR; 
unsigned int PC = 0; 
unsigned int FP = 0;
unsigned int step2 = 0;
unsigned int codeSize = 0;
unsigned int gVar[1];
unsigned int *instruction;
int counter_showCode = 0;
int *code;
int allocated_bytes;
int allocated_objects;
int copied_objects;
int copied_bytes;
bool DEBUGGING = false;
bool RUN = true; 
bool GC_STATS = false;
bool GC_PURGE = false;
bool d = false;
char * _heap;
 















