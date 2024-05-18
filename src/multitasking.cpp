
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

bool TaskManager::InitTask(Task* task)
{
    if(numTasks >= 256)
        return false;

   // tasks[numTasks++] = *task;

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
    printf("EXIT: ");
    printfInt(currentTask);
    printf("\n");
    tasks[currentTask].state = ProcessState::TERMINATED;
    return true;
}

int TaskManager::getIndex(common::uint32_t pid) {
    for (int i = 0; i < numTasks; i++) {
        if (tasks[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

bool TaskManager::WaitTask(common::uint32_t esp) {
     printf("***WAITPID: ");
    CPUState* cpustate = (CPUState*)esp;
    common::uint32_t pid = cpustate->ebx;

    printfInt(pid);
    printf(", ");
    printfInt(tasks[currentTask].pid);
        printf("\n");
    if(tasks[currentTask].pid == pid || pid == 0) {
        return false;
    }

    return true;
 printf("burda2\n");
    int index = getIndex(pid);
    if (index == -1) {
        return false;
    }
 printf("burda3\n");
    if(numTasks <= index || tasks[index].state == ProcessState::TERMINATED) {
        return false;
    }
 printf("burda4\n");
    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].waitpid = pid;
    tasks[currentTask].state = ProcessState::BLOCKED;
    printf("burda5\n");
    return true;
}


 /* CPUState* TaskManager::Schedule(CPUState* cpustate) {

    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;


    return tasks[currentTask].cpustate;
}
 
 */

CPUState* TaskManager::Schedule(CPUState* cpustate) {
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
           /*  printf(" CU: ");
            printfInt(nextTask);
            printf(" "); */

    currentTask = nextTask;
   /*  printf("NUMTASKS: ");
    printfInt(numTasks);
    printf("\n");

    printf("CURRENT STATE: ");
    printfInt(tasks[currentTask].state);
    printf("\n");

    printf("CURRENT PID: ");
    printfInt(tasks[currentTask].pid);
    printf("\n");  */

    return tasks[currentTask].cpustate;
}
    
    