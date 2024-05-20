
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char*);
void printfInt(int);

void my_sleep() {
    int n = 40000;
    int result = 0;
    for (int i = 0; i < n; ++i) 
        for (int j = 0; j < n; ++j) 
            result = i * j; 
}

// default constructor
Task::Task()
{
    cpustate = nullptr;
    pid = 0;
    ppid = 0;
    waitpid = 0;
    waitnum = 0;
    priority = 1;
    state = ProcessState::READY;
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint()) {
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;
    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;
    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    cpustate->eflags = 0x202;

    pid = 0;
    ppid = 0;
    waitnum = 0;
    waitparent = false;
    state = ProcessState::READY;
}

Task::~Task() {}

TaskManager::TaskManager(GlobalDescriptorTable* gdt) {
    this->gdt = gdt;
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager() {}

void TaskManager::PrintProcessTable() {
    printf("\n");
    printf(" PID | PPID | WaitNum | WaitParent |   State   | Priority\n");
    printf("-----|------|---------|------------|-----------|---------\n");

    for (int i = 0; i < numTasks; ++i) {
        printf(" ");
        printfInt(tasks[i].pid);
        if(tasks[i].pid < 10) {
            printf("   | ");
        } else {
            printf("  | ");
        }
        printfInt(tasks[i].ppid);
        printf("    | ");
        printfInt(tasks[i].waitnum);
        printf("       | ");
        if (tasks[i].waitparent) {
            printf("true ");
        } 
        else {
            printf("false");
        }
        printf("      | ");
        if (tasks[i].state == ProcessState::RUNNING || i == currentTask) {
            printf("RUNNING   ");
        }
        else if (tasks[i].state == ProcessState::BLOCKED) {
            printf("BLOCKED   ");
        }
        else if (tasks[i].state == ProcessState::TERMINATED) {
            printf("TERMINATED");
        } else if (tasks[i].state == ProcessState::READY) {
            printf("READY     ");
        }
        printf("| ");
        printfInt(tasks[i].priority);
        printf("\n");
    }
    printf("\n");
}

bool TaskManager::InitTask(Task* task) {
    CopyTask(task, &tasks[numTasks]); 
    task->priority = 1;
    numTasks++;
    return true;
}

void TaskManager::CopyTask(Task *src, Task *dest) {
    dest->state = src->state;
    dest->pid = src->pid;
    dest->ppid = src->ppid;
    dest->waitpid = src->waitpid;
    dest->waitnum = src->waitnum;

    // Copy the stack
    for (int i = 0; i < sizeof(src->stack); ++i) {
        dest->stack[i] = src->stack[i];
    }

     // Calculate the offset of cpustate within the source stack
    common::uint32_t srcStackBase = (common::uint32_t)src->stack;
    common::uint32_t srcCpuStateOffset = (common::uint32_t)src->cpustate - srcStackBase;

    // Set the destination cpustate pointer to the corresponding position in the destination stack
    common::uint32_t destStackBase = (common::uint32_t)dest->stack;
    dest->cpustate = (CPUState*)(destStackBase + srcCpuStateOffset);
 
    // Copy the CPU state
    *(dest->cpustate) = *(src->cpustate);
}

common::uint32_t TaskManager::getpid() {
    return tasks[currentTask].pid;
}

common::uint32_t TaskManager::ForkTask(CPUState* cpustate) {

    // fork for part a, b1, b2
    // fork
    
    if (numTasks >= 256) 
        return -1; 
    
    tasks[numTasks].state = ProcessState::READY;
    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = getpid();

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate = (CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);

    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];

    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;

    return tasks[numTasks - 1].pid;
}

common::uint32_t TaskManager::ForkTaskThirdStrategy(CPUState* cpustate) {

    // fork for part b.3
    // fork_2

    if (numTasks >= 256) 
        return -1; 
    
    tasks[numTasks].state = (numTasks == 1) ? ProcessState::READY : ProcessState::BLOCKED;
    tasks[numTasks].priority = (numTasks == 1) ? 0 : 1;

    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = getpid();

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate = (CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);

    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];

    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;

    return tasks[numTasks - 1].pid;
}
common::uint32_t TaskManager::ForkTaskDynamic(CPUState* cpustate) {

    // fork for part b.4
    // forkdynamic

    if (numTasks >= 256) 
        return -1; 
    
    tasks[numTasks].priority = (numTasks == 1) ? 0 : 4;

    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = getpid();

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate = (CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);

    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];

    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;

    return tasks[numTasks - 1].pid;
}

bool TaskManager::ExitCurrentTask() {
     printf("Process with PID ");
    printfInt(currentTask);
    printf(" exited.\n");  
    tasks[currentTask].state = ProcessState::TERMINATED;

    // if it's parent waits its child to terminate adjust necessary parts in the parent
    if (tasks[currentTask].waitparent) {
        int ppid = tasks[currentTask].ppid;
        if (tasks[ppid].state == BLOCKED) {
            if (tasks[ppid].waitnum > 1) {
                tasks[ppid].waitnum--;
            } 
            else {
                tasks[ppid].waitnum--;
                tasks[ppid].state = ProcessState::READY;
            }
        }
    }
    return true;
}

bool TaskManager::WaitTask(common::uint32_t esp) {
    CPUState* cpustate = (CPUState*)esp;
    common::uint32_t pid = cpustate->ebx;

    printf("PID ");
    printfInt(tasks[currentTask].pid);
    printf(" is waiting for PID ");
    printfInt(pid);
    printf("...\n");    

    // if the waited process already terminated return false without waiting
    if (tasks[pid].state == ProcessState::TERMINATED || pid == tasks[currentTask].pid) {
        tasks[currentTask].cpustate->ecx = 0;
        return false;
    }

    tasks[pid].waitparent = true;
    tasks[currentTask].state = ProcessState::BLOCKED;
    tasks[currentTask].waitnum++;
    tasks[currentTask].cpustate->ecx = 1;

    return true;
}

//schedule for part a, b.1, b.2
CPUState* TaskManager::ScheduleRobinRound(CPUState* cpustate, int interruptCount) {
    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    int nextTask = currentTask;

    
    do {
        nextTask = (nextTask + 1) % numTasks;
    } while (tasks[nextTask].state != ProcessState::READY);

  /*   if (currentTask >= 0 && tasks[currentTask].state == ProcessState::RUNNING) {
        tasks[currentTask].state = ProcessState::READY;
    }
 */
    currentTask = nextTask;
   /*   if (currentTask >= 0 && tasks[currentTask].state == ProcessState::READY) {
        tasks[currentTask].state = ProcessState::RUNNING;
    }
 */
    // print process table for given interval of interrupts
    if (interruptCount > 10 && interruptCount < 20) {
        PrintProcessTable();
        my_sleep();
    }

    if (interruptCount == 20) {
        printf("\nScheduling continues without printing process tables...\n");
    }

    return tasks[currentTask].cpustate;
}

CPUState* TaskManager::SchedulePreemptive(CPUState* cpustate, int interruptCount) {

    // schedule for part b.3
    // schedule priority

    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

     if (interruptCount == 5) {
        for (int i = 0; i < numTasks; i++) {
            if (i > 1 && tasks[i].state == ProcessState::BLOCKED) {
                tasks[i].state = ProcessState::READY;
            }
        }
    }   

  /*   if (tasks[currentTask].state = ProcessState::RUNNING) {
        tasks[currentTask].state = ProcessState::READY;
    }
 */
    if (interruptCount < 15) {
        printf("current task: ");
        printfInt(tasks[currentTask].pid);
        printf(", count: ");
        printfInt(interruptCount);
        printf("\n");
    }

    int highestPriorityTask = -1;
    for (int i = 0; i < numTasks; ++i) {
        if (tasks[i].state == ProcessState::READY || tasks[i].state == ProcessState::RUNNING) {
            if (highestPriorityTask == -1 || tasks[i].priority > tasks[highestPriorityTask].priority) {
                highestPriorityTask = i;
            }
        }
    }

    if (highestPriorityTask == -1) {
        // No READY task found, return the current CPU state
        return cpustate;
    }

     // save current cputstate of the current task
    if (currentTask >= 0 && tasks[currentTask].state == ProcessState::RUNNING) {
        tasks[currentTask].cpustate = cpustate;
        tasks[currentTask].state = ProcessState::READY;
    }

    currentTask = highestPriorityTask;
    tasks[currentTask].state = ProcessState::RUNNING;
    return tasks[currentTask].cpustate;
}

CPUState* TaskManager::ScheduleDynamic(CPUState* cpustate, int interruptCount) {

    // schedule for part b.4
    // schedule dynamic

    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    if (interruptCount % 5 == 0) {
        if (tasks[1].state != ProcessState::TERMINATED) {
            tasks[1].priority++;
        }
    }

    // save current cputstate of the current task
    if (currentTask >= 0 && tasks[currentTask].state == ProcessState::RUNNING) {
        tasks[currentTask].cpustate = cpustate;
        tasks[currentTask].state = ProcessState::READY;
    }
 
    int highestPriorityTask = -1;
    for (int i = 0; i < numTasks; ++i) {
        if (tasks[i].state == ProcessState::READY) {
            if (highestPriorityTask == -1 || tasks[i].priority > tasks[highestPriorityTask].priority) {
                highestPriorityTask = i;
            }
        }
    }

    if (highestPriorityTask == -1) {
        // No READY task found, return the current CPU state
        return cpustate;
    }

    currentTask = highestPriorityTask;
    tasks[currentTask].state = ProcessState::RUNNING;

    if (interruptCount % 5 == 0 && interruptCount < 100) {
        PrintProcessTable();
        my_sleep();
    }

    return tasks[currentTask].cpustate;
}
    