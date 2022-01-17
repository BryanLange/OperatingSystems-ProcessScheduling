#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------- 
   About: This program implements preemptive priority-based process scheduling
   		  with round-robin scheduling for processes with the same priority
   Compile: $ gcc scheduling.c -o scheduling
   Execute: $ ./scheduling scheduling_data.txt
----------------------------------------------------------------------------*/


#define Q 10  // time quantum used by the scheduler
#define N 96  // length of processing time period


// process structure
struct Node {
	char *id;
	int priority; 
	int burst;
	int arrival;
	int timeLeft;
	int turnaround;
	int wait;
	int quantum;
	struct Node *next;		 // used in ready queue
	struct Node *tableNext;  // used in master process data list
};


struct Node *table;   // head of master process data list 
struct Node *head;	  // head of ready queue list
struct Node *running; // process in the running state



// creates and returns a new process with given parameters:
// 	(process name, priority, burst length, arrival time)
struct Node* newNode(char *i, int p, int b, int a) 
{
	struct Node *temp = (struct Node*)malloc(sizeof(struct Node));

	temp->id = (char*)malloc(strlen(i)*sizeof(char*)); 
	strcpy(temp->id, i);
	temp->priority = p;
	temp->burst = b;
	temp->arrival = a;
	temp->timeLeft = b;
	temp->turnaround = 0;
	temp->wait = 0;
	temp->quantum = 0;
	temp->next = NULL;
	temp->tableNext = NULL;

	return temp;
} // end newNode()



// read and store data from input file
// terminate program if file not found
// created processes stored in master process data list
void readFile(char *filename) 
{ 
	FILE *file = fopen(filename, "r"); // "r" is read only
	if(!file) {
		printf("File not found: %s\n", filename);
		exit(2);
	}

	const int lineLength = 100;
	char line[lineLength];
	char *process;
	int priority, burst, arrival;
	struct Node *tempTail = NULL;

	// check if file has header:
	//		Process	Priority	Burst		Arrival
	//		-------	--------	-----		-------
	// absorb if it does, else close/reopen
	fgets(line, lineLength, file);
	process = strtok(line, "\t");
	if(process[1] == 'r') {
		fgets(line, lineLength, file);
	} 
	else {
		fclose(file);
		file = fopen(filename, "r");
	}

	while(fgets(line, lineLength, file)) {		

		process = strtok(line, "\t");
		priority = atoi(strtok(0, "\t"));		
		burst =  atoi(strtok(0, "\t"));	
		arrival =  atoi(strtok(0, "\t"));

		struct Node *newProc = newNode(process, priority, burst, arrival);

		if(table == NULL) {
			table = newProc;
			tempTail = table;
		}
		else {
			tempTail->tableNext = newProc;
			tempTail = newProc;
		}
	}

	fclose(file);
} // end readFile()



// adds given node to ready queue
// enforces ready queue is kept as a priority queue
// resets the node's quantum if >= Q
// if node was not preempted(quantum == 0), it will be placed at the end of
//		all other nodes with equal priority, if they exist, ensuring round
//		robin scheduling for equal priority processes
// if node was preempted before its quantum was finished(quantum != 0), it is 
// 		placed ahead of all other nodes with equal priority, if they exist, so
//		it can finish its remaining time quantum ensuring true round robin
//		scheduling for equal priority processes
void enqueue(struct Node *node) 
{
	if(node == NULL) {
		return;
	}
	else if(node->quantum >= Q) {
		node->quantum = 0;
	}


	if(head == NULL) {
		head = node;
	} 
	else if(node->priority > head->priority) {
		node->next = head;
		head = node;
	}
	else if(node->priority == head->priority) {
		if(node->quantum != 0) {
			node->next = head;
			head = node;
		} 
		else {
			node->next = head->next;
			head->next = node;
		}
	}
	else {
		struct Node* nav = head;

		while(nav->next != NULL && nav->next->priority <= node->priority &&
				node->quantum == 0) {
			nav = nav->next;
		}

		node->next = nav->next;
		nav->next = node;		
	}	
} // end enqueue();



// removes head of ready queue and places it in the running state
// head will always be highest priority process
void dequeue() 
{
	running = head;
	if(head != NULL) {
		head = head->next;
	}	
} // end dequeue()



// checks the state of the running process
// if finished, replace from ready queue
// if not finished but has exhausted its time quantum, put it back into the
//		ready queue to assume its round robin place, then replace from ready
//		queue
void quantumCheck() 
{
	if(running == NULL) {
		return;
	}
	else if(running->timeLeft == 0) {
		running->next = NULL;
		dequeue();
	}
	else if(running->quantum == Q) {
		enqueue(running);
		dequeue();
	}
} // end quantumCheck()



// increment or decrement running process and ready queue's processes
//		variables(quantum, timeLeft, wait, turnaround)
void stepProcesses() 
{
	if(running != NULL) {
		running->quantum++;
		running->timeLeft--;
		running->turnaround++;		
	}
	
	struct Node *nav = head;
	while(nav != NULL) {
		nav->wait++;
		nav->turnaround++;
		nav = nav->next;
	}
} // end incTurnAndWait()



// console print process information
void output()
{
	printf("output:\n");
	struct Node *nav = table;
	while(nav != NULL) {
		printf("\t%s,  turnaround time: %2d,  wait time: %2d\n",
				nav->id, nav->turnaround, nav->wait);
		nav = nav->tableNext;
	}
} // end output()



// frees malloc memory
void flush() 
{
	struct Node* temp;
	while(table != NULL) {
		temp = table;
		table = table->tableNext;
		free(temp->id);
		free(temp);
	}
} // end flush()



int main(int argc, char *argv[]) 
{
	table = NULL;
	head = NULL;
	running = NULL;


	// terminate program if invalid command line arguments	
	if(argc != 2) {
		printf("Invalid number of arguments.\n");
		exit(1);
	} 

	// read data from input file
	readFile(argv[1]);


	// from 0 to N, handle scheduling of processes
	for(int t=0; t<N; t++) {
		
		struct Node *nav = table;
		int flag = 0;

		// check arrival time of all processes
		while(nav != NULL) {

			// if new process is arriving, check whether it needs to preempt
			//		the running process, put into ready queue if not
			if(nav->arrival == t) {

				flag = 1; // change flag to idicate arrival of process
				

				// if no process is running, put into running state
				if(running == NULL) {				
					running = nav;
				}


				// else if priority is higher, check if running process is
				//		finished, context switch if not
				else if(nav->priority > running->priority) {
					if(running->quantum == 0 || running->quantum == 10) {		
						
						if(running->timeLeft <= 0) {
							running = nav;
						}
						else {
							enqueue(running);
							running = nav;
						}
					}
					else {
						printf("context switch|  t:%2d,  P_n:%s,  P_r:%s\n",
								t, nav->id, running->id);

						enqueue(running);
						running = nav;
					}	
				}


				// else put into ready queue if priority is the same or lower
				else {
					enqueue(nav);
				}
			}

			nav = nav->tableNext;

		} // end WHILE

		// if no new process arrived, check running process state
		if(flag == 0) {
			quantumCheck();
		}
			
		// increment or decrement active process variables
		stepProcesses();

	} // end FOR


	// output final process data
	output();
	

	// free allocated memmory
	flush();


	return 0;
} // end main()