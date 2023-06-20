//Finished version & code beautification
#include<bits/stdc++.h> 
#include <mutex> 
#include <windows.h>
#define MAX 5  // Define the maximum number of processes
#define TIME 1 // Define the time slice size, unit: second (s)
#define MEMORYSIZE 1000
#define KERNELSIZE 100

using namespace std;

//----------------------------------------------------------------------PCB
//Process ID (pid):PCB
typedef struct PCB {
    int id;
    int arrivedTime;
    int requiredTime;
    int usedTime;
    char state[8];

    int size;
    int startAddress;
}PCB, *pPCB;

//Process organization: link mode, here uses a doubly linked list (leading node)
//Due to the relatively small scale, a single PCB chain is used here
typedef struct PCBNode {
    PCB pcb;
    PCBNode *before;
    PCBNode *next;
}PCBNode, *pPCBNode;

//Process Control: Primitives
void schedule(pPCBNode &p);     //Process Scheduling Primitives
void kill(pPCBNode &p);         //process termination primitive
void creatPCB();                //process creation primitive
void block(int id);             //process block primitive
void wakeup(int id);            //process wake up primitive

//Global variable forPCB
pPCBNode ReadyList = new PCBNode;   //Kernel data structure (PCB chain)
mutex mtx;                          //Mutex for kernel data structure (PCB chain)
int pcbID;                          //When creating a PCB, the id used to generate the pcb
clock_t beginTime;                  //When creating a PCB, it is used to generate the arrival time

//----------------------------------------------------------------------Mem
//Blank memory identifier: FMPT(FreeMemoryPartitionTable)
typedef struct FreeMemNode {
    int size;
    int startAddress;
}FreeMemNode, *pFreeMemNode;

//memory management
int allocate(int memoryNeeded);                             //memory allocation
void releaseAndMerge(int toReleasedSize, int startAddress);  //memory recovery

//Global variable forMem
vector<FreeMemNode> FMPT;           //free partition table

//----------------------------------------------------------------------OS
//Utility functions in the OS
void initPCB();                 // Initialize the PCB
void showPCB();                 // Print out the PCB linked list, visually display the status of each PCB 
void initMem();                 // Initialize memory 
void showMem();                 // Print out the free partition table 
void initOS();                  // Initialize the OS
void startOS();                 // start the OS 
pPCBNode checkReadyList();      // Check if there are unexecuted PCBs in the ReadyList


int main() {

	initOS();

	beginTime = clock();
	thread myThread ( startOS );
	myThread.detach();
	
	string s;
	while(1){
		printf("\n[HanShell]# ");
		
//	  	cin>>s;		
		getline(cin,s);
		istringstream record(s);

//	  	cout<<s;
		string out;
        record >> out;
        if(out == "creat") {
			creatPCB(); 
		}else if(out == "show"){
			showPCB();
			showMem();
		}else if(out == "block"){
			record >> out;
			int num = atoi(out.c_str());
			block(num);
		}else if(out == "wakeup"){
			record >> out;
			int num = atoi(out.c_str());
			wakeup(num);
		}
		
	}
	
	return 0;
}

// Initialize the PCB
void initPCB() {
    pPCBNode p = ReadyList;
    
    for (int i = 0; i < MAX; i++) {
    	pPCBNode temp = new PCBNode;
        //pointer field assignment
        temp->before = p;
        temp->next = NULL;
        p->next = temp;
        p = temp;
        //data field assignment
        // PCB pcb = temp->pcb;
        temp->pcb.id = pcbID++;
        temp->pcb.arrivedTime = 0;
        temp->pcb.requiredTime = rand() % 10 + 1;
        temp->pcb.usedTime = 0;
        strcpy(temp->pcb.state, "Ready");
        temp->pcb.size = rand() % 89 + 1;
        temp->pcb.startAddress = allocate(temp->pcb.size);
//        puts("success");
        //The before of the head node is used to record the tail node of the current linked list (to avoid creating a separate tail to mark the tail node)
        ReadyList->before = temp;
    }
    
}

// Print out the PCB linked list, visually display the status of each PCB
void showPCB() {
	pPCBNode p = ReadyList->next;
    printf("PID    |arrival time    |required time    |elapsed time    |state        |memory start address    |occupancy size\n");
    printf("----------------------------------------------------------------------------------\n"); 
	while(p) {
		PCB pcb = p->pcb;
		printf("%-6d|%-12d|%-12d|%-12d|%-12s|%-12d|%-12d\n", 
			pcb.id, pcb.arrivedTime, pcb.requiredTime, pcb.usedTime, pcb.state, pcb.startAddress, pcb.size );
		p = p->next;
	}
	puts("");
}



//Total memory 1000Mb, kernel area
void initMem(){

    FreeMemNode tmp = {.size = MEMORYSIZE - KERNELSIZE, .startAddress = KERNELSIZE };
    FMPT.push_back(tmp);
}

void showMem(){
	printf("id    |Memory start address    |free size <- free partition table\n");
	printf("-----------------------------\n"); 
	
    for(int i= 0 ;i< FMPT.size();i++ ){
        printf("%-6d|%-12d|%-12d\n", i,FMPT[i].startAddress, FMPT[i].startAddress);
    }
    puts("");
}

// Check if there are unexecuted PCBs in the ReadyList
// Tip: For functions with multiple exits, be very careful when controlling the mutex, and unlock before each exit
pPCBNode checkReadyList() {
    
	pPCBNode p = ReadyList->next;
	while(p) {
		if (strcmp(p->pcb.state, "Ready") == 0) {
			mtx.unlock();
			return p;
		}
		p = p->next;
	}
    
	return NULL;
}

void initOS(){
    //Initialize memory
    initMem();
    //Simulate a randomly distributed blank memory table
    vector<FreeMemNode> test;
    for( int i=0 ; i<15 ; i++){
        int memSize = rand()%100 +1;
        // cout<<memSize<<endl;
        int startAddress = allocate(memSize);
        FreeMemNode tmp = {.size = memSize, .startAddress = startAddress };
        test.push_back(tmp);
    }
    for( int i=1 ; i<14 ; i=i+2){
        releaseAndMerge(test[i].size,test[i].startAddress);
    }
    puts("\n**********initial memory allocation**********"); 
    showMem();
    
    //???PCB
    initPCB();
    puts("\n**********Initial Submitted Task List**********"); 
    showPCB();
}

void startOS() {
	pPCBNode p = NULL;
	while(1) {
        p = checkReadyList();
		if (p != NULL) {
			strcpy(p->pcb.state, "Running");
            int remainingTime = p->pcb.requiredTime - p->pcb.usedTime;
			if (remainingTime <= TIME) {
				Sleep(remainingTime * 1000);
				strcpy(p->pcb.state, "Dead");
				p->pcb.usedTime = p->pcb.requiredTime;
                //process termination primitive
				kill(p);
//TO DO Reclaiming the memory space occupied by the process

			} else {			
                Sleep(TIME * 1000);
				strcpy(p->pcb.state, "Ready");
				p->pcb.usedTime += TIME;
				if (p->next) {
                    //process switching primitives
					schedule(p);
				}
			}
		}
        

	}
}

void creatPCB(){
	
    mtx.lock();
	    pPCBNode temp = new PCBNode;
	    //pointer field assignment
	    temp->before = ReadyList->before;
	    temp->next = NULL;
	    ReadyList->before->next = temp;
	    ReadyList->before = temp;
	    //data field assignment
	    // PCB pcb = temp->pcb; Typical error: this is a deep copy, the parent cannot be modified
	    temp->pcb.id = pcbID;
        temp->pcb.arrivedTime = ((clock() - beginTime) / CLOCKS_PER_SEC );
	    temp->pcb.requiredTime = rand() % 10 + 1;
	    temp->pcb.usedTime = 0;
	    temp->pcb.size = rand() % 49 + 1;
		temp->pcb.startAddress = allocate(temp->pcb.size);
	    strcpy(temp->pcb.state, "Ready");
	    //The before of the head node is used to record the tail of the current linked list
	    pcbID++;
    mtx.unlock();
    
	printf("id    |Arrival time    |Required time    |Elapsed time    |State        |Memory start address    |Occupied size\n");
	printf("----------------------------------------------------------------------------------\n"); 
	PCB pcb = temp->pcb;
	printf("%-6d|%-12d|%-12d|%-12d|%-12s|%-12d|%-12d\n", 
		pcb.id, pcb.arrivedTime, pcb.requiredTime, pcb.usedTime, pcb.state, pcb.startAddress, pcb.size );
	puts("");
	
}

void schedule(pPCBNode &p){
	mtx.lock();
	    p->before->next = p->next;
	    p->next->before = p->before;
	    p->before = ReadyList->before;
	    ReadyList->before->next = p;
	    ReadyList->before = p;
	    p->next = NULL;
	mtx.unlock();    
}

void kill(pPCBNode &p){
	mtx.lock();
		releaseAndMerge(p->pcb.size,p->pcb.startAddress);
		if (p->next) {
		    p->before->next = p->next;
		    p->next->before = p->before;
		    delete p;
		}else{
			p->before->next = p->next;
			ReadyList->before = ReadyList;
			delete p;
		}
	mtx.unlock();  

}

//Tip: For functions with multiple exits, be very careful when controlling the mutex, and unlock before each exit
void block(int id){
	mtx.lock();
	    pPCBNode p = ReadyList->next;
	    while(p) {
			if(p->pcb.id == id){
	            strcpy(p->pcb.state, "Blocked");
	            printf("id=%d blocked\n", id);
                mtx.unlock();
	            return;
	        }
			else{
	            p = p->next;
	        }
		}
	    //No process specified to block exists
		puts("error");
	mtx.unlock();   
}


void wakeup(int id){
	mtx.lock();   
	    pPCBNode p = ReadyList->next;
	    while(p) {
			if(p->pcb.id == id){
	            strcpy(p->pcb.state, "Ready");
	            printf("id=%d wake up\n", id);
                mtx.unlock();
	            return;
	        }
			else{
	            p = p->next;
	        }
		}
	    //No process specified to wake up exists
		puts("error");
	mtx.unlock();   
}

int allocate(int memoryNeeded){
	int tmp = -1;
    for(int i= 0 ;i< FMPT.size();i++ ){
        if(FMPT[i].size >= memoryNeeded){
            tmp = FMPT[i].startAddress;
            FMPT[i].size -= memoryNeeded;
            FMPT[i].startAddress += memoryNeeded;
            break;
        }
    }
    // cout<<"in the allocate func  "<<tmp<<"   "<< memoryNeeded <<endl;
	if(tmp==-1) puts("Allocation Failure");
    return tmp;
}

void releaseAndMerge(int toReleasedSize, int startAddress){
	
//	cout<<"in the Merge func  "<<startAddress<<"   "<< toReleasedSize <<endl;
	
    int endAddress = startAddress + toReleasedSize ; 
    
    if( FMPT[0].startAddress  >= startAddress ){
        if(FMPT[0].startAddress  == endAddress){
            FMPT[0].size += toReleasedSize;
            FMPT[0].startAddress -= toReleasedSize;
        }else{
            FreeMemNode tmp = {.size = toReleasedSize, .startAddress = startAddress };
            FMPT.insert(FMPT.begin(),tmp);
        }
    }else if(FMPT[FMPT.size()-1].startAddress < startAddress){
        if(FMPT[FMPT.size()-1].startAddress + FMPT[FMPT.size()-1].size  == startAddress){
            FMPT[FMPT.size()-1].size += toReleasedSize;
        }else{
            FreeMemNode tmp = {.size = toReleasedSize, .startAddress = startAddress };
            FMPT.push_back(tmp);
        }
    }else{
		for( int i= 0; i< FMPT.size()-1;i++ ){
		    	if(FMPT[i].startAddress < startAddress && FMPT[i+1].startAddress > startAddress){
		            if(FMPT[i].startAddress + FMPT[i].size  == startAddress ){
		                //There are adjacent free partitions before & after the reclaimed area
		                if(FMPT[i+1].startAddress == endAddress){
		                    FMPT[i].size = FMPT[i].size + toReleasedSize + FMPT[i+1].size;
		                    FMPT.erase(FMPT.begin() + i+1);
		                }//There are adjacent free partitions before the reclaim area only
		                else{
		                    FMPT[i].size += toReleasedSize;
		                    // puts("success");
		                }
		            }//There are adjacent free partitions only after the reclaim area
		            else if(FMPT[i].startAddress  == endAddress){
		                FMPT[i].size += toReleasedSize;
		                FMPT[i].startAddress -= toReleasedSize;
		            }//There are no adjacent free partitions before & after the reclaimed area
		            else{
		                FreeMemNode tmp = {.size = toReleasedSize, .startAddress = startAddress };
		                FMPT.insert(FMPT.begin()+i+1,tmp);
		            }
		
		        }
		    }	
	}
}
