//Implement wakeup and timer
#include<bits/stdc++.h> 
#include <mutex> 
#include <windows.h>
#define MAX 5  // Define the maximum number of processes
#define TIME 6 // Define the time slice size, unit: second (s)

using namespace std;

//process id (pid):PCB
typedef struct PCB {
    int id;
    int arrivedTime;
    int requiredTime;
    int usedTime;
    char state[8];
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
void creatPCB();                //process creation primitives
void block(int id);             //process blocking primitives
void wakeup(int id);            //Process wakeup primitive

//Utility functions in the OS
void initPCB();                 // Initialize the PCB
void showPCB();                 // Print out the PCB linked list, visually display the status of each PCB 
pPCBNode checkReadyList();      // Check if there are unexecuted PCBs in the ReadyList
void startOS();                 // start the OS 

//global variable
mutex mtx;                          //Mutex for kernel data structure (PCB chain)
pPCBNode ReadyList = new PCBNode;   //Kernel data structure (PCB chain)
int pcbID;                          //When creating a PCB, the id used to generate the pcb
clock_t beginTime                   //When creating a PCB, it is used to generate the arrival time

int main() {
	
	beginTime = clock();

	initPCB();
	thread myThread ( startOS );
	myThread.detach();
	
	string s;
	while(1){
		printf("\n[myshell]# ");
		
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
		}else if(out == "block"){
			record >> out;
			int num = atoi(out.c_str());
			block(num);
		}
		
	}
	
	return 0;
}


// Print out the PCB linked list, visually display the status of each PCB
void showPCB() {
	pPCBNode p = ReadyList->next;
    printf("id    |Time arrival    |Time required    |Elapsed time   |State\n");
    printf("---------------------------------------------------\n"); 
	while(p) {
		PCB pcb = p->pcb;
		printf("%-6d|%-12d|%-12d|%-12d|%-12s\n", 
			pcb.id, pcb.arrivedTime, pcb.requiredTime, pcb.usedTime, pcb.state);
		p = p->next;
	}
	puts("");
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
        //The before of the head node is used to record the tail node of the current linked list (to avoid creating a separate tail to mark the tail node) 
        ReadyList->before = temp;
    }
    printf("ReadyList after initialization:\n");
    showPCB();
    printf("\n");
}

//Check if there are unexecuted PCBs in the ReadyList
//Tip: For functions with multiple exits, be very careful when controlling the mutex, and unlock before each exit
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
	    strcpy(temp->pcb.state, "Ready");
	    //The before of the head node is used to record the tail of the current linked list
	    pcbID++;
    mtx.unlock();
    
	printf("id    |Time arrival   |Time required    |Elapsed time    |State\n");
    printf("---------------------------------------------------\n"); 
	PCB pcb = temp->pcb;
	printf("%-6d|%-12d|%-12d|%-12d|%-12s\n", 
		pcb.id, pcb.arrivedTime, pcb.requiredTime, pcb.usedTime, pcb.state);
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
	            printf("id=%d wakeup\n", id);
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
