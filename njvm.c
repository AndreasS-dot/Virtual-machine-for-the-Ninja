#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bigint/build/include/support.h"
#include "bigint/build/include/bigint.h"
#include "bigint/build/include/instr.h"
#include "bigint/build/include/variables.h"

void initHeap(int heapSize);
void initStack(int stackSize);
void fatalError(char *msg);
void * heap_locate(int dataSize);
void *newPrimObject(int dataSize);
void * getPrimObjectDataPointer(void * primObject);
int pop(void); 
ObjRef * popObjRef(void); 
void push(int x);
void pushObjRef(ObjRef * value); 
ObjRef * newCompObject(int numObjRefs);
void flip_memory(void);
void initGC(void);
void copy_root_objects(void);
void scan_Root_Objects(void);
void garbage(void);
ObjRef  relocate(ObjRef orig);
ObjRef * copyObjToFreeMem(ObjRef * objRef);
void gcStats(void);
int showCode(signed int instruction,int step);
void showStack(void);
void set_breakpoint(void);
void debug(void) ;
void rflow(void);
void loadMemory(void);
void loadFile(char *codeFileName);


void fatalError(char *msg) {
  printf("Fatal error: %s\n", msg);
  exit(1);
}

void * heap_locate(int dataSize) {
    char *obj; 
    if(heap.end1 < heap.freeSize + dataSize) {
        garbage(); 
        if(heap.end1 < heap.freeSize + dataSize) {
            puts("heap overflow");
            puts("Ninja Virtual Machine stopped");
            exit(1);
        }
    }
    allocated_bytes = dataSize + allocated_bytes;
    allocated_objects = allocated_objects + sizeof(char);
    obj = heap.freeSize;
    heap.freeSize = heap.freeSize + dataSize;
    return obj;
} 

void * newPrimObject(int dataSize) {
    ObjRef  primObj;
    ObjRef primObj2;
    primObj = heap_locate(sizeof(int)+dataSize*sizeof(unsigned char));
    primObj2 = (ObjRef) primObj;
    primObj2->size = dataSize;
    return (ObjRef*) primObj2;
}

void * getPrimObjectDataPointer(void * primObject){ 
    ObjRef oo = ((ObjRef) primObject);
    return oo->data;
}

int pop(void){
    if (SC == 0) {
        puts("Stack underflow\n");
        puts("Ninja Virtual Machine stopped");
        exit(1);
    }
    else{
        SC = SC-1;
        return stack[SC].u.number; 
    }
}

ObjRef * popObjRef(void) {
    if(SC == 0) {
        puts("stack underflow");
        puts("Ninja Virtual Machine stopped");
        exit(1);
    }
    SC = SC-1;
    if(!stack[SC].isObjRef) {
        puts("popObjRef detected number on top of stack");
        puts("Ninja Virtual Machine stopped\n");
        exit(1);
    }
    return (ObjRef*) stack[SC].u.objRef;
    }


void push(int x){ 
    if(stackSize < SC) {
        printf("stack overflow");
        puts("Ninja Virtual Machine stopped");
        exit(1);
    }
    stack[SC].isObjRef = false; 
    stack[SC].u.number = x; 
    SC++;
    return;
}

void pushObjRef(ObjRef * value) { 
    if(stackSize < SC) {
        puts("Stack overflow\n");
        puts("Ninja Virtual Machine stopped\n");
        exit(1);
    }
    ObjRef or = ((ObjRef)value);
    stack[SC].isObjRef = true;
    stack[SC].u.objRef = or;
    SC++;
}


ObjRef * newCompObject(int numObjRefs) {
    ObjRef or;
    ObjRef or2;
    or = heap_locate(sizeof(int) + numObjRefs * sizeof(ObjRef));
    or2 = (ObjRef) or;
    or2->size = numObjRefs | MSB;
    int i;
    for (i = 0; i < numObjRefs; i++) {
        GET_REFS_PTR(or2)[i] = NULL;     
    }
    return (ObjRef*) or2;
}


void initGC(void){
    heap.end1 = _heap+(heap.heapSize/2);
    heap.end2 = heap.end1 + (heap.heapSize/2); 
    heap.freeSize = _heap;
    heap.start1 = _heap; 
    heap.start2 = heap.end1;
}

void garbage(void) {
    char * tmp1;
    char * tmp2;
    int i = 0;
    ObjRef tmpObj;
    ObjRef tmpObj2;
    ObjRef **_p;
    ObjRef stackObj;
    unsigned int _tmp;

    heap.freeSize = heap.start2;
    tmp2 = heap.end1;
    tmp1 = heap.start1;
    heap.start1 = heap.start2;
    heap.start2 = tmp1;
    heap.end1 = heap.end2; 
    heap.end2 = tmp2;
    heap.scan = heap.freeSize; 
    heap.heapSize = heapSize;

    for(i = 0; i < *gVar; i++) {
        _p = staticData + i;
        tmpObj = relocate((ObjRef)staticData[i]);
        *_p = (ObjRef*) tmpObj;
    }
    i = SC;
    for(i = SC; 0 < i; i = i-1) {
        if(stack[i].isObjRef) {
            stackObj = relocate(stack[i].u.objRef);
            stack[i].u.objRef = stackObj;
        }
    }
    bip.op1 = relocate(bip.op1);
    bip.op2 = relocate(bip.op2);
    bip.res = relocate(bip.res);
    bip.rem = relocate(bip.rem);
    tmpObj = (ObjRef) returnRegister;
    tmpObj2 = relocate(tmpObj);
    returnRegister = (ObjRef *) tmpObj2;
    while (heap.scan != heap.freeSize) {
        _tmp = *(int*)heap.scan & 0x7fffffff;
        if(*(int*)heap.scan < 0) {
            for (i = 0; i < _tmp; i++)
                  GET_REFS_PTR((ObjRef) heap.scan)[i] = relocate(GET_REFS_PTR((ObjRef) heap.scan)[i]);
            heap.scan = heap.scan + _tmp*sizeof(ObjRef)+sizeof(int);
        }
        else{
            heap.scan = heap.scan+_tmp+sizeof(int);
        }
    }
    if(GC_PURGE)
        memset(heap.start2, 0,heap.heapSize/2);
    if(GC_STATS)
        gcStats();
    copied_objects = 0;
    allocated_objects = 0;
    copied_bytes = 0; 
    allocated_bytes = 0;
}

ObjRef  relocate(ObjRef orig){ 
    ObjRef copy;
    if(orig == NULL)
        copy = NULL;
    else if(IS_BROKENHEART(orig))
        copy = (ObjRef)(heap.start1 + (orig->size & 0xbfffffff));
    else{
        copy = (ObjRef)(copyObjToFreeMem((ObjRef*)orig));
        orig->size = (unsigned int)((char*)copy - heap.start1) ;
        orig->size = orig->size | (B_heart);
    }
    return copy; 
}

ObjRef * copyObjToFreeMem(ObjRef * objRef){
    int copySize;
    ObjRef tmpObj; 
    tmpObj = (ObjRef) objRef;

    if(IS_PRIMITIVE(tmpObj))
        copySize = sizeof(unsigned int)+(GET_SIZE(tmpObj));
    else
        copySize = sizeof(unsigned int)+sizeof(ObjRef)*GET_SIZE(tmpObj);
    if(heap.end1 < heap.freeSize + copySize) {
        puts("fatal GC error (no space to coby object");
        puts("Ninja Virtual Machine stopped\n");        
        exit(1);
    }
    memcpy(heap.freeSize,tmpObj,copySize);
    tmpObj = (ObjRef)heap.freeSize;
    copied_bytes = copySize + sizeof(int) + copied_bytes;
    copied_objects = copied_objects + sizeof(char);
    heap.freeSize += copySize;
    return (ObjRef*)tmpObj; 
}

void gcStats(void) {
    puts("Garbage Collector:");
    printf("    %u objects (%u bytes) allocated since last collection\n",allocated_objects, allocated_bytes);
    printf("    %u objects (%u bytes) copied during this collection\n", copied_objects,copied_bytes);
    printf("    %u of %u bytes free after this collection\n",(heap.heapSize /2) - ((int)heap.freeSize - (int) heap.start1),heap.heapSize /2);   
}

int showCode(signed int instruction,int step){ 
    printf("%04d:\t",counter_showCode);
    int value;
    counter_showCode++;
    unsigned int operation = instruction >> 0x18;
        if ((instruction & 0x800000) == 0)
            value = instruction & 0xffffff;
        else 
            value = instruction | 0xff000000;
        
        switch(operation){
            case(0):
                printf("halt\n");
                break;
            case(1):
                printf("pushc    %d\n", value);
                break;
            case(2):
                printf("add\n");
                break;
            case(3):
                printf("sub\n");
                break;
            case(4):
                printf("mul\n");
                break;
            case(5):
                printf("div\n");
                break;
            case(6):
                printf("mod\n");
                break;
            case(7):
                printf("rdint\n");
                break;
            case(8):
                printf("wrint\n");
                break;
            case(9):
                printf("rdchr\n");
                break;
            case(10):
                printf("wrchr\n");
                break;
            case(11):
                printf("pushg   %d\n", value);
                break;
            case(12):
                printf("popg    %d\n", value);
                break;
            case(13):
                printf("asf     %d\n", value);
                break;
            case(14):
                printf("rsf\n");
                break;
            case(15):
                printf("pushl   %d\n", value);
                break;
            case(16):
                printf("popl    %d\n", value);
                break;
            case(17):
                printf("eq\n");
                break;
            case(18): {
            puts("heap overflow");
                printf("ne\n");
                break;
            case(19):
                printf("lt\n");
                break;
            case(20):
                printf("le\n");
                break;
            case(21):
                printf("gt\n");
                break;
            case(22):
                printf("ge\n");
                break;
            case(23):
                printf("jmp     %d\n", value);
                break;
            case(24):
                printf("brf     %d\n", value);
                break;
            case(25):
                printf("brt     %d\n", value);
                break;
            case(26):
                printf("call    %d\n", value);
                break;
            case(27):
                printf("ret\n");
                break;
            case(28):
                printf("drop    %d\n", value);
                break;
            case(29):
                printf("pushr\n");
                break;
            case(30):
                printf("popr\n");
                break;
            case(31):
                printf("dup\n");
                break;
            case (32):
                printf("new\n");
                break;
            case (34):
                printf("putf\n");
                break;
            case (33):
                printf("getf\n");
                break;
            case (35):
                printf("newa\n");
                break;
            case (36):
                printf("getfa\n");
                break;
            case (37):
                printf("putfa\n");
                break;
            case (38):
                printf("getsz\n");
                break;
            case (39):
                printf("pushn\n");
                break;
            case (40):
                printf("refeq\n");
                break;
            case (41):
                printf("refne\n");
                break;
            default:
                printf("illegal instruction at address %d",instruction);
                break;
        }
        return 0;
    }

void showStack(void) {
  int i;
  i = SC;
  if (SC == FP)
        printf("sp, fp  --->\t%04d:\t(xxxxxx) xxxxxx\n",SC);
  else 
        printf("sp      --->\t%04d:\t(xxxxxx) xxxxxx\n",SC);
  while (0 < i) {
    i = i - 1;
    if (i == FP)
        printf("fp      --->\t");
    else 
        printf("\t\t");
    if (!stack[i].isObjRef) 
        printf("%04d:\t(number) %d\n",i,stack[i].u.number);
    else {
        if (stack[i].u.objRef == 0)
            printf("%04d:\t(objref) nil\n",i);
        else 
            printf("%04d:\t(objref) %p \n",i,(ObjRef *)stack[i].u.objRef);
    }
  }
  puts("\t\t--- bottom of stack ---");
  return;
}

void set_breakpoint(void) {
    char *user_input;
    long user_input_num;
    int brkpt;
    char *endptr;
    char line [200];

    printf("DEBUG [breakpoint]: ");
    if (breakpoint == -1) 
        puts("cleared");
    else 
        printf("set at %d\n",breakpoint);
  printf("DEBUG [breakpoint]: address to set, -1 to clear, ");
  puts("<ret> for no change?");
  user_input = fgets(line,200,stdin);
  if (user_input == (char *)0x0) 
        endptr = line;
  else {
    user_input_num = strtol(line,&endptr,10);
    brkpt = (int)user_input_num;
  }
  if ((endptr != line) && (*endptr == '\n')) {
        breakpoint = brkpt;
        printf("DEBUG [breakpoint]: now ");
        if (breakpoint == -1) 
            puts("cleared");
        else 
            printf("set at %d\n",breakpoint); 
  }
  return;
}

void debug(void) {                 
    char *input;                             
    DEBUGGING = true; 
    while(DEBUGGING) {
        if(!d) {
            showCode(code[PC], step2);
            step2++;
            d = true;
        }
        printf("DEBUG: inspect, list, step, RUN, quit?\n"); 
        input = malloc(sizeof(char) * 20);                   
        fgets(input, 20, stdin);
        switch(input[0]) {
            case 'i': 
                printf("DEBUG [inspect] stack, data, object?\n");
                fgets(input, 20, stdin);
                switch(input[0]) {
                    case's': 
                        showStack();
                        break;
                    case 'a':
                        break; 
                    case 'd':
                        break; //todo
                    case 'o':
                        break; //todo
                    default:
                        break;
                }
                break;
            case 's':
                while(true) {
                showCode(code[PC],step2);
                step2++;
                rflow();  
                }
                break;
            case 'l':
                for (int i = 0; i < instr[0]; i ++)
                showCode(code[i],i);
                puts("\t--- end of code ---\n");
                debug();    
                break;
            case 'b': 
                set_breakpoint();
                break;
            case 'q':
                DEBUGGING = false;
                break;
            case 'r': 
                rflow();
                break;
        }
    }
}

void rflow(){
    char userInputC = 0;
    ObjRef * ref1; 
    ObjRef * ref2;
    ObjRef deref1;
    ObjRef deref2; 
    unsigned int instruction;
    int tmp;
    int value;
    while(true) {
        if(!RUN){
            return;
        }
        int _PC = PC+1;
        instruction = code[PC]; 

        unsigned int operation = instruction >> 0x18;
        if ((instruction & 0x800000) == 0) {
            value = instruction & 0xffffff;
            }
        else {
        value = instruction | 0xff000000;
        }
    switch(operation){
        case (halt): 
            RUN = false;
            PC = _PC;
            break;
        case (pushc): 
            PC = _PC;
            bigFromInt(value);
            pushObjRef(bip.res);
            break;
        case (add):
            PC = _PC;
            bip.op2 = popObjRef();            
            bip.op1 = popObjRef(); 
            bigAdd(); 
            pushObjRef(bip.res);
            break;
        case (sub):  
            PC = _PC;          
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigSub();
            pushObjRef(bip.res);        
            break;
        case (mul):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigMul();
            pushObjRef(bip.res);    
            break;
        case (div):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigDiv();
            pushObjRef(bip.res);
            break;
        case (mod):         
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            bigDiv();
            pushObjRef(bip.rem);          
            break;
        case (rdint):
            PC = _PC;
            bigRead(stdin);
            pushObjRef(bip.res);
            break;
        case (wrint):
            PC = _PC;
            bip.op1 = popObjRef();
            bigPrint(stdout);
            break;
        case (rdchr):
            PC = _PC;
            scanf("%c",&userInputC);
            bigFromInt((int) userInputC); 
            pushObjRef(bip.res);
            break;
        case (wrchr):
            PC = _PC;
            bip.op1 = popObjRef();
            userInputC = bigToInt();
            printf("%c", userInputC);
            break;
        case (pushg):
            PC = _PC;
            pushObjRef(staticData[value]);
            break;
        case (popg):
            PC = _PC;
            ref1 = popObjRef(); 
            staticData[value] = ref1;
            break;
        case (asf):
            PC = _PC;
            push(FP);
            FP = SC; 
            for(int n = 0; n < value; n++) 
                pushObjRef((ObjRef *) 0x0);
            break;
        case (rsf):
            SC = FP;
            PC = _PC;
            FP = pop();
            break;
        case (pushl):
            PC = _PC;
            tmp = value + FP; 
            if (stack[tmp].isObjRef == 0) {
                printf("PUSHL detected number in local or parameter variable\n");
                puts("Ninja Virtual Machine stopped");
                exit(1);          
            }
               pushObjRef((ObjRef*)(stack[tmp].u.objRef));
            break;
        case (popl):
            PC = _PC;
            ref1 = popObjRef();
            stack[(value+FP)].isObjRef = true;
            stack[(value +FP)].u.objRef = (ObjRef)ref1;
            break;    
        case (eq):                                      
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() == 0)
                bigFromInt(1);
            else 
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case (ne):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() == 0) 
                bigFromInt(0);
             else 
                bigFromInt(1);
            pushObjRef(bip.res);
            break;
        case (lt):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() < 0)
                bigFromInt(1);
            else 
               bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case (le):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() < 1) 
                bigFromInt(1);
            else 
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case (gt):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() < 1)
                bigFromInt(0);
            else 
                bigFromInt(1);
            pushObjRef(bip.res);
            break;
        case (ge):
            PC = _PC;
            bip.op2 = popObjRef();
            bip.op1 = popObjRef();
            if(bigCmp() < 0)
               bigFromInt(0);
            else 
                bigFromInt(1);
            pushObjRef(bip.res);
            break;
        case (jmp):
            PC = value;
            break;
        case (brt):
            PC = _PC;
            bip.op1 = popObjRef();
            tmp = bigSgn();
            if (tmp == 0) 
                PC = value;
            break;
        case (brf):
            PC = _PC;
            bip.op1 = popObjRef(); 
            _PC = bigSgn();
            if(_PC != 0)
                PC = value;
            break;
        case (call):
            PC = _PC;
            push(_PC);
            PC = value;
            break;
        case (ret): 
            PC = _PC;
            PC = pop();
            break;
        case (drop): 
            SC = SC - value; 
            PC = _PC;
            if (SC < 0) {
                printf("stack underflow");
                puts("Ninja Virtual Machine stopped");
                exit(1);
            }
            break;
        case (pushr):
            PC = _PC;
            pushObjRef(returnRegister);
            break;
        case (popr): 
            PC = _PC;
            returnRegister = popObjRef();
            break;
        case (dup):
            PC = _PC;
            ref1 = (ObjRef*)stack[SC-1].u.objRef;
            pushObjRef(ref1);
            break;
        case(new):
            PC = _PC;
            pushObjRef(newCompObject(value));
            break;
        case(getf):
            PC = _PC;
            ref1 = popObjRef();
            if(ref1 == (ObjRef *) 0x0) {
                printf("nil reference exception\n"); 
                puts("Ninja Virtual Machine stopped");
                exit(1);
            }
            deref1 = (ObjRef) ref1;
            pushObjRef((ObjRef *)(GET_REFS_PTR(deref1)[value]));
            break;
        case(putf):
            PC = _PC;
            ref2 = popObjRef();
            ref1 = popObjRef();
            if(ref1 == (ObjRef *) 0x0) {
                printf("nil reference exception\n");
                puts("Ninja Virtual Machine stopped");
                exit(1);
            }
            GET_REFS_PTR((ObjRef)ref1)[value] = (ObjRef)ref2;
            break;
        case(newa):
            PC = _PC;
            bip.op1 = popObjRef();
            tmp = bigToInt();
            if (tmp < 0) {
                puts("index out of bounds exception");
                puts("Ninja Virtual Machine stopped");
                exit(1);
            }
            ref1 = newCompObject(tmp);
            pushObjRef(ref1);
            break;
        case(getfa):
            PC = _PC;
            bip.op1 = popObjRef();
            tmp = bigToInt();            
            ref1 = popObjRef();
            if (ref1 == (ObjRef *)0x0) {
                printf("nil reference exception\n");
                puts("Ninja Virtual Machine stopped");
                exit(1);                
            }
            deref1 = (ObjRef) ref1;
            pushObjRef((ObjRef *)(GET_REFS_PTR(deref1)[tmp]));
            break;
        case(putfa): 
            PC = _PC;
            ref2 = popObjRef(); 
            bip.op1 = popObjRef();
            tmp = bigToInt(); 
            ref1 = popObjRef(); 
            deref2 = (ObjRef) ref2; 
            deref1 = (ObjRef) ref1; 
            GET_REFS_PTR(deref1)[tmp] = deref2;
            break;
        case(getsz):
            PC = _PC;
            ref1 = popObjRef();
            deref1 = (ObjRef)ref1;
            if(IS_PRIMITIVE(deref1)) {
                tmp = -1;
                bigFromInt(tmp);
                pushObjRef(bip.res);
            }
            else {
                tmp = GET_ELEMENT_COUNT(deref1); 
                bigFromInt(tmp); 
                pushObjRef(bip.res);
            }
            break;
        case(pushn):
            PC = _PC;
            pushObjRef((ObjRef *) 0x0);
            break;
        case(refeq):
            PC = _PC;
            ref2 = popObjRef();
            ref1 = popObjRef();
            if (ref1 == ref2)
                bigFromInt(1);
            else 
                bigFromInt(0);
            pushObjRef(bip.res);
            break;
        case(refne):
            PC = _PC;
            ref2 = popObjRef();
            ref1 = popObjRef();
            if (ref1 == ref2)
                bigFromInt(0);
            else 
                bigFromInt(1);
            pushObjRef(bip.res);
            break;
        default:
            printf("Error! Operation is incompartible or not defined!\n");
            puts("Ninja Virtual Machine stopped");
            exit(1);
        }
        if(DEBUGGING) {
            debug();
        }
    }
}


void loadFile(char *codeFileName)
{
  int version;
  int i;
  FILE *codeFile;
  char buffer [4]; 
  codeFile = (FILE *)fopen(codeFileName,"r");
  if (codeFile == (FILE *)0x0) 
    printf("cannot open code file \'%s\'",codeFileName); //ich übergebe NULL
  if (fread(buffer,1,4,(FILE *)codeFile) != 4)              //fread(4, 1, 4, codeFile)
    printf("cannot read code file \'%s\'",codeFileName);
  if ((((buffer[0] != 'N') || (buffer[1] != 'J')) || (buffer[2] != 'B')) || (buffer[3] != 'F')) 
    printf("file \'%s\' is not a Ninja binary",codeFileName);
  if (fread(&version,4,1,(FILE *)codeFile) != 1) 
    printf("cannot read code file \'%s\'",codeFileName);
  if (version != 8)
    printf("file \'%s\' has wrong version number",codeFileName);
  if (fread(instr,4,1,(FILE *)codeFile) != 1) 
    printf("cannot read code file \'%s\'",codeFileName);
  if (fread(gVar,4,1,(FILE *)codeFile) != 1) 
    printf("cannot read code file \'%s\'",codeFileName);
  code = (int*)malloc(instr[0]*sizeof(int));                //code = (int*)malloc(instr[0]*sizeof(int))
  if (code == (int *)0x0) 
    printf("cannot allocate program memory");
  if (fread(code,4,instr[0],(FILE *)codeFile) != instr[0]) 
    printf("cannot read code file \'%s\'",codeFileName);
  staticData = (ObjRef **)malloc((long)*gVar << 3);         //staticData = (ObjRef **) malloc((long)*gVar << 3);
  if (staticData == (ObjRef **)0x0) 
    printf("cannot allocate data memory");
  for (i = 0; i < *gVar; i++) 
    staticData[i] = (ObjRef *)0x0;
  fclose((FILE *)codeFile);
  return;
}

void loadMemory(void) {
    stack = (StackSlot *)malloc((long)stackSize << 4);
    if (stack == (StackSlot *)0x0) {
        puts("cannot allocate stack memory");
        printf("Ninja Virtual Machine stopped\n");
        exit(1);
    }
    _heap = (char *)malloc((long)heapSize);
    heap.heapSize = heapSize;
    if (_heap == (char *)0x0) {
        puts("cannot allocate heap memory");
        printf("Ninja Virtual Machine stopped\n");
        exit(1);
    }
}

int main(int argc , char *argv []) {
    char *endptr;
    long n; 
    int _argc = 1;
    char * FileName;
    FileName = (char *) 0x0;
    do{
        if(argc <= _argc) {
            if (FileName == (char *) 0x0) {
                puts("no code file specified");
            }
            loadFile(FileName); //char *
            loadMemory(); //
            if(DEBUGGING) {
                debug();
            }
            puts("Ninja Virtual Machine started");
            initGC(); 
            rflow(); 
            garbage(); 
            printf("Ninja Virtual Machine stopped\n");
            return 0; 
        }
        if(*argv[_argc] == '-') {
            if(strcmp(argv[_argc], "--stack") == 0) {
                if(argc - 1 == _argc) {
                    puts("stackSize is missing"); 
                    printf("Ninja Virtual Machine stopped\n");
                    exit(1);                    
                }
                _argc++;
                n = strtol(argv[_argc], &endptr, 0);
                if ((*endptr != '\0') || (n<1)) {
                    puts("Illegal stack size"); 
                    printf("Ninja Virtual Machine stopped\n");
                    exit(1);                    
                }
                stackSize = (int) ((long) (n<<10) >> 4); 
            }
            else {
                if(strcmp(argv[_argc], "--heap") == 0) {
                    if(argc - 1 == _argc) {
                        puts("heap size is missing");
                        printf("Ninja Virtual Machine stopped\n");
                        exit(1);                        
                    }
                    _argc++; 
                    n = strtol(argv[_argc], &endptr, 0);
                    if ((*endptr != '\0') || (n<1)) {
                        puts("Illegal heap size"); 
                        printf("Ninja Virtual Machine stopped\n");
                        exit(1);                        
                    }
                    heapSize = (int) n << 10; 
                }
                else {
                    if(strcmp(argv[_argc], "--gcstats") == 0) {
                        GC_STATS = true;
                    }
                    else {
                        if(strcmp(argv[_argc], "--gcpurge") == 0) {
                            GC_PURGE = true;
                        }
                        else {
                            if(strcmp(argv[_argc], "--debug") == 0) {
                                DEBUGGING = true;
                            }
                            else {
                                if(strcmp(argv[_argc], "--help") == 0) {
                                    printf("Usage: %s [options] <code file>\n",*argv);
                                    puts("Options:");
                                    puts("  --stack<n>       set stack size to n KBytes (default: n = 64");
                                    puts("  --heap<n>        set heap size to n KBytes (default: n = 8192");
                                    puts("  --gcstats        show garbage collection statistics");
                                    puts("  --gcpurge        purge old objects after collection");
                                    puts("  --debug          start virtual machine in debug mode");
                                    puts("  --version        show version and exit");
                                    puts("  --help           show this help and exit");
                                    exit(0);                                    
                                }
                                if(strcmp(argv[_argc], "--version") == 0) {
                                    printf("Ninja Virtual Machine version %d (compiled %s, %s)\n",8,"Jan 07 2022","20:28:00\n");
                                    exit(0);
                                }
                                printf("unknown option \'%s\', try \'%s --help\'",argv[_argc],*argv);
                                exit(1);
                            }
                        }
                    }
                }
            }
        }
        else {
            FileName = argv[_argc];
        }
        _argc++;
    } while(true);
}
                