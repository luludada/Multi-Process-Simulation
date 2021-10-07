#include "simulation.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> 

#define process_state_running 1
int pid_Count = 0;
int sem_Count = 0;
int quantum = 5;
List* readyQueue_high;
List* readyQueue_norm;
List* readyQueue_low;
List* block_queue;
List* receive_waiting_queue;
List* message_list;
List* semaphore_queue;
PCB* init_process;
PCB* running_process;
int sendToID = 0;

bool Semaphore_comparator(void* item, void* comparisonArg) {
	Semaphore* myItem = (Semaphore*)item;
	int semID = myItem->semID;
	if (semID == *(int*)comparisonArg)
		return 1;
	return 0;
}

bool Message_comparator(void* item, void* comparisonArg) {
	Message* myItem = (Message*)item;
	int PID = myItem->send_PID;
	if (PID == *(int*)comparisonArg)
		return 1;
	return 0;
}

bool PID_comparator(void* item, void* comparisonArg) {
	PCB* myItem = (PCB*)item;
	int PID = myItem->PID;
	if (PID == *(int*)comparisonArg)
		return 1;
	return 0;
}


/*
Init the process and Create the ready queue
*/
void init(){
    init_process = (PCB *) malloc(sizeof(PCB));
    init_process->priority = high_priority;
    // init_process->process_state = process_state_running;
    init_process->PID = 0;
    pid_Count++;
    init_process->burstTime = (rand() % 8) + 1;
    init_process->semID = -1;
    init_process->semVal = 0;

    running_process = init_process;

    block_queue = List_create();
    receive_waiting_queue = List_create();
    message_list = List_create();
    semaphore_queue = List_create();
    readyQueue_high = List_create();
    readyQueue_norm = List_create();
    readyQueue_low = List_create();
}

void instruction(){
    printf("Please enter one of the following commands, one char only\n");
    printf("C: Create a process\n");
    printf("F: Fork\n");
    printf("K: Kill process\n");
    printf("E: Kill current process\n");
    printf("Q: Time quantum \n");
    printf("S: Send a message \n");
    printf("R: Receive a message\n");
    printf("Y: unblocks sender and delivers reply\n");
    printf("N: Initialize new semaphore\n");
    printf("P: Execute semaphore P Functionn");
    printf("V: Execute semaphore V Function\n");
    printf("I: Print complete process state info\n");
    printf("T: Display all process queues and contents\n");
    printf("\n");
}

void updateRunningP()
{
	if(List_count(readyQueue_high) > 0)
	{
		running_process = (PCB*)List_last(readyQueue_high);
	}  else if (List_count(readyQueue_norm) > 0)
	{
		running_process = (PCB*)List_last(readyQueue_norm);
	
	} else if(List_count(readyQueue_low) > 0)
		running_process = (PCB*)List_last(readyQueue_low);
	
}

int addReadyQueue(PCB* process) {
    int status;
	if (process->PID == 0){
        return -1;
    }	
	else if (process->priority == 0){
        status = List_prepend(readyQueue_high, process);
    }
	else if (process->priority == 1){
        status = List_prepend(readyQueue_norm, process);
    }
	else {
		status = List_prepend(readyQueue_low, process);
	}
    return status;
}


void Create(int priority){
    PCB* process = malloc(sizeof *process);
	process->PID = pid_Count;
    pid_Count++;
	process->priority = priority;
	process->semID = -1; 
    process->burstTime = (rand() % 10) + 1; 
    int status_create = addReadyQueue(process);

    if(status_create == LIST_SUCCESS){
        printf("Success: Create process with PID: %d!\n", process->PID);
    } else {
        printf("Fail: Create failed\n");
    }
}

PCB* Copy(PCB* proc) {
	PCB* Copy = malloc(sizeof* Copy);
	Copy->PID = proc->PID;
    Copy->priority = proc->priority;
    Copy->burstTime = proc->burstTime;
    strcpy(Copy->msg, proc->msg);
    Copy->semID = proc->semID;
	Copy->semVal = proc->semVal;
	return Copy;
}

void Fork(){

    updateRunningP();
    
    if (running_process->PID != 0) {
		PCB* process = Copy(running_process);
		process->PID = pid_Count++;
		int status_fork = addReadyQueue(process);
		if(status_fork == LIST_SUCCESS){
            printf("Success: Create process with PID: %d!\n", process->PID);
        } else {
            printf("Fail: Create failed\n");
       	}
    } else {
		printf("Fail: Attempting fork init process\n");
    }
}



void printProcessMessage(PCB* proc) {
	if (proc->msg[0] != '\0') {
		printf("\nMessage Received: %s", proc->msg);
		memset(proc->msg, '\0', 40);
	} 
}

PCB* nextProcess() {
	PCB* nextProc;
    List_first(readyQueue_high);
    List_first(readyQueue_norm);
    List_first(readyQueue_low);
	if (List_count(readyQueue_high) != 0){
        nextProc = List_trim(readyQueue_high);
    }
	else if (List_count(readyQueue_norm) != 0){
        nextProc = List_trim(readyQueue_norm);
    }
	else if (List_count(readyQueue_low) != 0){
        nextProc = List_trim(readyQueue_low);
    }
	else {
        nextProc = init_process;
    }
		
	//printf("\nNew running process PID: %d\n", nextProc->PID);
	return nextProc;
}

void Exit() {
	//updateRunningP();
	if (running_process->PID != 0) {
		free(running_process);
		running_process = nextProcess();
		//printf("Success: Now new process running PID: %d\n", running_process->PID);
	} else if (List_count(readyQueue_low) == 0 && List_count(readyQueue_norm) == 0 && List_count(readyQueue_high) == 0) {
		printf("Fail: No process is available, end simulation\n");
		exit(0);
	} else {
		addReadyQueue(running_process);
		running_process = nextProcess();
	}
}


void Kill(int PID){
    
    if (PID == 0 && List_count(readyQueue_high) == 1 && List_count(readyQueue_norm) == 0 && List_count(readyQueue_low) == 0) { 
		printf("Killing 'init' process, end simulation\n");
		free(init_process);
		exit(0);
	} else if (running_process->PID == PID) {
		Exit();
	} else {
		PCB* proc;
        List_first(readyQueue_high);
        List_first(readyQueue_norm);
        List_first(readyQueue_low);
        List_first(block_queue);
		
        if (List_search(readyQueue_high, PID_comparator, &PID) != NULL) {
            proc = List_remove(readyQueue_high);
        } else if (List_search(readyQueue_norm, PID_comparator, &PID) != NULL) {
            proc = List_remove(readyQueue_norm);
        } else if (List_search(readyQueue_low, PID_comparator, &PID) != NULL){
            proc = List_remove(readyQueue_low);
        } else if ((List_search(readyQueue_low, PID_comparator, &PID)) != NULL)
            proc = List_remove(block_queue);
        else {
            proc = NULL;
        }

		if (proc == NULL){
            printf("Fail: Process PID: %d not found\n", PID);
        }	
		else {
			printf("Success: Process PID: %d killed\n", PID);
			free(proc);
		}
	}
}

void Quantum(){
    running_process->burstTime = running_process->burstTime - quantum;
    if(running_process->PID == 0 && List_count(readyQueue_high) == 1 && List_count(readyQueue_norm) == 0 && List_count(readyQueue_low) == 0){
        printf("Fail: Only Init process exits, and no other process in ready queue\n");
    } else if(running_process->burstTime  <= 0){
        Exit();
    } else {
        PCB* preeptive_process = Copy(running_process);
		addReadyQueue(preeptive_process);
		Exit();
    }
}

int searchProcess(int pid){
    if(pid == 0){
        return 1;
    }
    List_first(readyQueue_high);
    List_first(readyQueue_norm);
    List_first(readyQueue_low);
    List_first(block_queue);
    if (List_search(readyQueue_high, PID_comparator, &pid) != NULL) {
        return 1;
    } else if (List_search(readyQueue_norm, PID_comparator, &pid) != NULL) {
        return 1;
    } else if (List_search(readyQueue_low, PID_comparator, &pid) != NULL){
        return 1;
    } else if ((List_search(block_queue, PID_comparator, &pid)) != NULL)
        return 1;
    else {
        return 0;
    }
}

void Send(int pid, char* msg){
    if(searchProcess(pid)){
    	updateRunningP();
        printf("running_process->PID: %d, search PID: %d\n", running_process->PID, pid );
        if(running_process->PID != pid){
            Message* newMsg = malloc(sizeof* newMsg);
            newMsg->from_PID = running_process->PID;
            newMsg->send_PID = pid;
            strcpy(newMsg->msg, msg);

            if (running_process->PID != 0) {
                PCB* blocked_process = Copy(running_process);
                sendToID = running_process->PID;
                List_prepend(block_queue, blocked_process);
                printf("Success: Process PID %d has been blocked until reply\n", running_process->PID);
            } else {
                printf("Cannot block init process");
                addReadyQueue(running_process);
            }

            PCB* unblock_process;
            if ((unblock_process = List_search(block_queue, PID_comparator, &pid)) != NULL) {
                printf("unblock the reciving process\n");
                strcpy(unblock_process->msg, msg);
               
                addReadyQueue(List_remove(block_queue));
            }

            List_prepend(message_list, newMsg);
            running_process = nextProcess();
            printProcessMessage(running_process);

        } else {
            printf("Fail: Cannot send message to current process, must send another process\n");
        }
    } else {
        printf("Fail: Cannot find PID\n");
    }
}

void Receive(){
    List_first(message_list);
    updateRunningP();
    Message* msg = List_search(message_list, Message_comparator, &running_process->PID);

    	if (msg == NULL) { 
        	printf("No messages for process PID: %d\n",running_process->PID);
		if (running_process->PID != 0) {
			PCB* blocked_process = Copy(running_process);
			List_prepend(block_queue, blocked_process);
            		printf("Success: Process PID %d has been blocked until one message arrives \n", running_process->PID);
		} else {
            		printf("Cannot block init process");
        	}
		running_process = nextProcess();
		printProcessMessage(running_process);
	} else {
		printf("Message from PID: %d to PID: %d\n", msg->from_PID, msg->send_PID);
		printf("Message: %s", msg->msg);
		free(List_remove(message_list));
	}
}

void Reply(int pid, char* msg){
    //Unblock any "sending" process and reply to it with msg
	PCB* toUnblock;
    List_first(block_queue);
	if (List_search(block_queue, PID_comparator, &pid) != NULL) {
		toUnblock = List_remove(block_queue);
		addReadyQueue(toUnblock);
		strcpy(toUnblock->msg, msg);
        	printf("Success: reply PID to %d\n", pid);
	} else {
        printf("Fail: reply PID to %d\n", pid);
    }
}

PCB* searchProcessByPID(int pid){
    if(pid == 0){
        return init_process;
    }
    List_first(readyQueue_high);
    List_first(readyQueue_norm);
    List_first(readyQueue_low);
    if (List_search(readyQueue_high, PID_comparator, &pid) != NULL) {
    	PCB* res = List_curr(readyQueue_high);
        return res;
    } else if (List_search(readyQueue_norm, PID_comparator, &pid) != NULL) {
        PCB* res = List_curr(readyQueue_norm);
        return res;
    } else if (List_search(readyQueue_low, PID_comparator, &pid) != NULL){
        PCB* res = List_curr(readyQueue_low);
        return res;
    } else if ((List_search(block_queue, PID_comparator, &pid)) != NULL) {
        PCB* res = List_curr(block_queue);
        return res;
     }
    else {
        return NULL;
    }
}

void NewSemaphore(int semID, int semVal) {
	Semaphore* newSema = malloc(sizeof *newSema);
	newSema->semID = semID;
	newSema->semVal = semVal;
	updateRunningP();
    	newSema->PID = running_process->PID;
    	running_process->semID = semID;
    	running_process->semVal = semVal;
	List_prepend(semaphore_queue, newSema);
    	printf("Success: create a new semaphore with semID %d and semVal %d on process %d\n", semID, semVal, running_process->PID);
}

void SemaphoreP(int semID){
    	if (semID < 0 || semID > 4) {
		printf("Fail: Invalid semaphore ID\n");
		return;
	}

    	List_first(semaphore_queue);
    	Semaphore* sem = List_search(semaphore_queue, Semaphore_comparator, &semID);

    	if (sem == NULL) {
		printf("Fail: No semaphore semID %d found\n",semID);
	
	} else {
        	PCB* process = searchProcessByPID(sem->PID);
        	if(process != NULL){
            		sem->semVal--;
            		process->semVal = sem->semVal;
            
            		if (sem->semVal < 0) {
                		PCB* blocked_process = Copy(process);
                		List_prepend(block_queue, blocked_process);
                		printf("Blocked: process PID %d with semaphore semID %d blocked with semVal %d\n", process-> PID,process->semID, process->semVal);
                		process = nextProcess();
            		} else {
                		printf("Not Blocked: semaphore semID %d not blocked with semVal %d\n", process->semID, process->semVal);
            		}
        	} else {
            		printf("Fail: the process with semID %d not found", sem->semID);
        	}
		
	}

}

void SemaphoreV(int semID){
    	if (semID < 0 || semID > 4) {
		printf("Fail: Invalid semaphore ID\n");
		return;
    	}

    	List_first(semaphore_queue);
	Semaphore* sem = List_search(semaphore_queue, Semaphore_comparator, &semID);
	if (sem == NULL) {
		printf("\nNo semaphore found with semID %d\n",semID);
	} else {
		PCB* process = searchProcessByPID(sem->PID);
        
        	if(process != NULL){
            		PCB* toUnblock;
            		sem->semVal++;
            		if (sem->semVal >= 0) {
                		List_first(block_queue);
                		while ((toUnblock = List_search(block_queue, Semaphore_comparator, &semID)) != NULL) {
                    		printf("Ready: Unblocking process PID: %d\n", toUnblock->PID);
                    		toUnblock->semVal = sem->semVal;
                    		addReadyQueue(List_remove(block_queue));
                	}
            
            		} else {
                		printf("Not ready: process PID: %d \n", running_process->PID);
            
            		}
        
       	 	} else {
            		printf("Fail: the process with semID %d not found", sem->semID);
        	}
		
	}
}



void Procinfo(int pid){
    PCB* proc;
    if(pid == 0){
        proc = init_process;
        printf("\nPID: %d, Priority: %d, Brust Time Remaining: %d, semID: %d, semVal: %d\n",proc->PID, proc->priority, proc->burstTime, proc->semID, proc->semVal);
    } else if(pid == running_process->PID){
        proc = running_process;
        printf("\nPID: %d, Priority: %d, Brust Time Remaining: %d, semID: %d, semVal: %d\n",proc->PID, proc->priority, proc->burstTime, proc->semID, proc->semVal);
    } else {
        PCB* proc = searchProcessByPID(pid);
        if (proc == NULL) {
            printf("Fail: No process with ID: %d\n",pid);
        } else {
            printf("\nPID: %d, Priority: %d, Brust Time Remaining: %d, semID: %d, semVal: %d\n",proc->PID, proc->priority, proc->burstTime, proc->semID, proc->semVal);
        }
    }
}

void printQueue(List* queue){
    List_first(queue);
    PCB *iter = (PCB *)List_last(queue);
    while(iter){
        PCB * temp = (PCB *)(iter);
        Procinfo(temp->PID);
        iter = List_prev(queue);
    }
    //List_last(queue); //schedule the queue
}

void Totalinfo(){
    printf("\nPrinting current running process info\n");
    Procinfo(running_process->PID);
    printf("\nPrinting init process info\n");
    Procinfo(init_process->PID);
    printf("\nHigh priority ReadyQ :\n");
	printQueue(readyQueue_high);
	printf("\nNorm priority ReadyQ:\n");
	printQueue(readyQueue_norm);
	printf("\nLow priority ReadyQ:\n");
	printQueue(readyQueue_low);
	printf("\nBlocked priority ReadyQ:\n");
	printQueue(block_queue);
	printf("\n");
}


void startSimulation(){

    char cmd[command_size];
	char param[parameter_size];
	char msg[max_message_size];

    while(true){
        printf("Please enter a command : ");
        fgets(cmd, command_size, stdin);

        if (strlen(cmd) == 2) {
            if (cmd[0] == 'c' || cmd[0] == 'C') {
                printf("Create - Please enter the priority of the new process: 0 (high), 1 (norm), 2 (low)\n");
                fgets(param, parameter_size, stdin);
                if ((strlen(param) == 2) & (param[0] == '0' || param[0] == '1' || param[0] == '2')) {
                    int priority = param[0] - '0';
                    Create(priority);
                } else {
                    printf("Invalid priority\n");
                }
            }
            else if(cmd[0] == 'f' || cmd[0] == 'F'){
            	updateRunningP();
                printf("Forking current running process PID: %d\n", running_process->PID);
	    		Fork();
            }
            else if(cmd[0] == 'k' || cmd[0] == 'K'){
                printf("Kill - enter the process PID: \n");
	    		fgets(param, parameter_size, stdin);
	    		int PID = atoi(param);
                printf("Kill process PID: %d\n", PID);
	    		Kill(PID);
            }
            else if(cmd[0] == 'e' || cmd[0] == 'E'){
            	updateRunningP();
                printf("Exit - kill the currently running process PID: %d\n", running_process->PID);
	    	Exit();
            }
            else if(cmd[0] == 'q' || cmd[0] == 'Q'){
                printf("Quantum - time quantum of running process expires\n");
	    		Quantum();
            }
            else if(cmd[0] == 's' || cmd[0] == 'S'){
                printf("Send - Please enter the PID of the process to send: \n");
	    		fgets(param, parameter_size, stdin);
	    		int pid = atoi(param);
	    		printf("Send - Please enter your message(40 char max): \n");
	    		fgets(msg, max_message_size, stdin);
	    		Send(pid,msg);
	    		memset(msg,'\0', max_message_size);
            }
            else if(cmd[0] == 'r' || cmd[0] == 'R'){
                printf("Receive - block until one arrives\n");
	    		Receive();
            }
            else if(cmd[0] == 'y' || cmd[0] == 'Y'){
                printf("Reply - Please enter the PID of the process to reply (unblock senders and delivers): \n");
	    		fgets(param, parameter_size, stdin);
	    		int pid = atoi(param);
	    		printf("Reply - Please enter your message(40 char max): \n");
	    		fgets(msg, max_message_size, stdin);
	    		Reply(pid, msg);
	    		memset(msg,'\0', max_message_size);
            }
            else if(cmd[0] == 'n' || cmd[0] == 'N'){
                if (sem_Count >= 5){
                    printf("Fail: Can only have 5 semaphores on system\n");
                } else {
                    printf("New Semaphore - Please enter Semaphore Value for Semaphore ID %d:\n", sem_Count);
                    fgets(param, parameter_size, stdin);
                    int semVal = atoi(param);
                    NewSemaphore(sem_Count, semVal);
                    sem_Count++;
                }
            }
            else if(cmd[0] == 'p' || cmd[0] == 'P'){
                printf("Semaphore P: Please enter a semaphore semID (0 - 4): \n");
	    		fgets(param, parameter_size, stdin);
	    		int semID = atoi(param);
	    		SemaphoreP(semID);
            }
            else if(cmd[0] == 'v' || cmd[0] == 'V'){
                printf("Semaphore V: Please enter a semaphore semID (0 - 4): \n");
	    		fgets(param, parameter_size, stdin);
	    		int semID = atoi(param);
	    		SemaphoreV(semID);
            }
            else if(cmd[0] == 'i' || cmd[0] == 'I'){
                printf("Process Info: - Please enter the PID of the process: \n");
	    		fgets(param, parameter_size, stdin);
	    		int pid = atoi(param);
	    		Procinfo(pid);
            }
            else if(cmd[0] == 't' || cmd[0] == 'T'){
                printf("TotalInfo - Print all process info\n");
	    		Totalinfo();
            }
            else {
                printf("Invalid command\n");
            }
        } else {
            printf("Invalid command\n");
        }

        memset(cmd,'\0', command_size);
        memset(param,'\0', parameter_size);
        //memset(msg,'\0', max_message_size);
    } 

}



int main() {
    srand(time(0));
    init();
    instruction();
    startSimulation();
    free(init_process);
    return 0;
}
