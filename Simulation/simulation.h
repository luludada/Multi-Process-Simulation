#include "list.h"

#define high_priority 0
#define norm_priority 1
#define low_priority 2
#define max_message_size 40
#define command_size 4
#define parameter_size 4
#define INVALID_PID -1

typedef struct message{
    int from_PID;
    int send_PID;
    char msg[max_message_size];
} Message;

typedef struct process_control_block{
    int PID;
    int priority; // high: 0, norm: 1, low: 2
    char msg[max_message_size];
    int burstTime;
    int semID;
    int semVal;
    List* PCB_queue;
} PCB;

typedef struct Semaphore{
    int PID;
    int semID;
    int semVal;
} Semaphore;
