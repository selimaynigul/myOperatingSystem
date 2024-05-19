
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char*);
void printfInt(int);

Task::Task()
{
    cpustate = nullptr;
    pid = 0;
    ppid = 0;
    waitpid = 0;
    state = ProcessState::READY;
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{

    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;
    
    /*
    cpustate->gs = 0;
    cpustate->fs = 0;
    cpustate->es = 0;
    cpustate->ds = 0;
    */
    
    // cpustate->error = 0;    
   
    // cpustate->esp = ;
    cpustate->eip = (uint32_t)entrypoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    // cpustate->ss = ;
    cpustate->eflags = 0x202;

    pid = 0;
    ppid = 0;
    waitnum = 0;
    waitparent = false;
    state = ProcessState::READY;
}

Task::~Task()
{
}



TaskManager::TaskManager(GlobalDescriptorTable* gdt)
{
    this->gdt = gdt;
    numTasks = 0;
    currentTask = -1;
}


TaskManager::~TaskManager()
{
}

void TaskManager::PrintProcessTable() {
    printf("\n");
    printf(" PID | PPID | WaitNum | WaitParent | State\n");
    printf("-----|------|---------|------------|-------\n");

    for (int i = 0; i < numTasks; ++i) {
        printf(" ");
        printfInt(tasks[i].pid);
        printf("   | ");
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
        if (tasks[i].state == ProcessState::READY) {
            printf("READY");
        }
        else if (tasks[i].state == ProcessState::BLOCKED) {
            printf("BLOCKED");
        }
        else {
            printf("TERMINATED");
        }
        printf("\n");
    }
    printf("\n");
}

bool TaskManager::InitTask(Task* task)
{
    if(numTasks >= 256)
        return false;

    task->pid = numTasks;    
    CopyTask(task, &tasks[numTasks]); 
    numTasks++;

    return true;
}

void TaskManager::CopyTask(Task *src, Task *dest) {
    dest->state = src->state;
    dest->pid = src->pid;
    dest->ppid = src->ppid;
    dest->waitpid = src->waitpid;

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

bool TaskManager::ExitCurrentTask() {
   /*  printf("EXIT: ");
    printfInt(currentTask);
    printf("\n"); */
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

  /*   printf("***WAITPID: waiting: ");
    printfInt(pid);
    printf(", who is waiting: ");
    printfInt(tasks[currentTask].pid);
    printf("\n");  */

    // if the waited process already terminated return false without waiting
    if (tasks[pid].state == ProcessState::TERMINATED) {
       // printf("Already terminated\n");
        return false;
    }

    tasks[pid].waitparent = true;
    tasks[currentTask].state = ProcessState::BLOCKED;
    tasks[currentTask].waitnum++;

    return true;
}

CPUState* TaskManager::Schedule(CPUState* cpustate) {
    printf("MALTAKAN ");
    if (numTasks <= 0)
        return cpustate;

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    int nextTask = currentTask;
    do {
        nextTask = (nextTask + 1) % numTasks;
        if(tasks[nextTask].state == ProcessState::TERMINATED) {
        }
    } while (tasks[nextTask].state != ProcessState::READY);

    currentTask = nextTask;
 
    return tasks[currentTask].cpustate;
}
    
    