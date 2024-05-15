
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char*);
void printfInt(int);

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{

    pcb->cpustate = (CPUState*)(pcb->stack + 4096 - sizeof(CPUState));
    
    pcb->cpustate -> eax = 0;
    pcb->cpustate -> ebx = 0;
    pcb->cpustate -> ecx = 0;
    pcb->cpustate -> edx = 0;

    pcb->cpustate -> esi = 0;
    pcb->cpustate -> edi = 0;
    pcb->cpustate -> ebp = 0;
    
    /*
    pcb->cpustate -> gs = 0;
    pcb->cpustate -> fs = 0;
    pcb->cpustate -> es = 0;
    pcb->cpustate -> ds = 0;
    */
    
    // pcb->cpustate -> error = 0;    
   
    // pcb->cpustate -> esp = ;
    pcb->cpustate -> eip = (uint32_t)entrypoint;
    pcb->cpustate -> cs = gdt->CodeSegmentSelector();
    // pcb->cpustate -> ss = ;
    pcb->cpustate -> eflags = 0x202;

    pcb->pid = 0;
    pcb->ppid = 0;
    pcb->state = ProcessState::READY;
}

Task::~Task()
{
}

        
TaskManager::TaskManager(GlobalDescriptorTable *gdt)
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
    tasks[numTasks++] = task;

    return true;
}

common::uint32_t TaskManager::getpid() {
    return tasks[currentTask]->pcb->pid;
}

 common::uint32_t TaskManager::ForkTask2(CPUState* cpustate)
{
    if(numTasks >= 256)
        return 0;
 
    tasks[numTasks]->pcb->state=ProcessState::READY;
    tasks[numTasks]->pcb->ppid=tasks[currentTask]->pcb->pid;
   // tasks[numTasks]->pid=Task::pIdCounter++;
    for (int i = 0; i < sizeof(tasks[currentTask]->pcb->stack); i++)
    {
        tasks[numTasks]->pcb->stack[i]=tasks[currentTask]->pcb->stack[i];
    }
    //Stackten yer alında cpustate'in konumu değişiyor bu nedenle şuanki taskın offsetini hesaplayıp yeni oluşan process'in cpu statenin konumunu ona göre düzenliyorum-> Bu işlemi yapmazsam process düzgün şekilde devam etmiyor->
    common::uint32_t currentTaskOffset=(((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask]->pcb->stack));
    tasks[numTasks]->pcb->cpustate=(CPUState*)(((common::uint32_t) tasks[numTasks]->pcb->stack) + currentTaskOffset);
 
    //Burada ECX' yeni taskın process id'sini atıyorum-> Syscall'a return edebilmek için->
    tasks[numTasks]->pcb->cpustate->ecx = 0;
    numTasks++;
    return tasks[numTasks-1]->pcb->pid;
} 


 common::uint32_t TaskManager::ForkTask(CPUState* cpustate) {

    if (numTasks >= 256) 
        return -1; 
    
    printf(" PARENT: ");
    printfInt(tasks[numTasks - 1]->pcb->pid);
    printf(" CHILD: ");
    printfInt(tasks[numTasks]->pcb->pid);
    
    tasks[numTasks] = new Task(gdt, (void(*)()) cpustate->eip);
    if (tasks[numTasks] == nullptr) {
        printf("Error: Failed to allocate new Task.\n");
        return -1;
    }

    tasks[numTasks]->pcb->state = ProcessState::READY;
    tasks[numTasks]->pcb->pid = numTasks;

    printf(" ************PARENT: ");
    printfInt(tasks[numTasks - 1]->pcb->pid);
    printf(" CHILD: ");
    printfInt(tasks[numTasks]->pcb->pid);

    tasks[numTasks]->pcb->ppid = getpid();

    common::uint32_t currentTaskOffset = (((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask]->pcb->stack));
    tasks[numTasks]->pcb->cpustate = (CPUState*)(((common::uint32_t) tasks[numTasks]->pcb->stack) + currentTaskOffset);
    
    for (int i = 0; i < sizeof(tasks[currentTask]->pcb->stack); i++)
        tasks[numTasks]->pcb->stack[i] = tasks[currentTask]->pcb->stack[i];
    
    tasks[numTasks]->pcb->cpustate->ecx = 0;
    numTasks++;

    tasks[1]->pcb->pid = 31;
    printf(" NEW: ");
    printfInt(tasks[0]->pcb->pid);


    return tasks[numTasks - 1]->pcb->pid;
}

bool TaskManager::ExitCurrentTask() {
    tasks[currentTask]->pcb->state = ProcessState::TERMINATED;
    return true;
}

int TaskManager::getIndex(common::uint32_t pid) {
    for (int i = 0; i < numTasks; i++) {
        if (tasks[i]->pcb->pid == pid) {
            return i;
        }
    }
    return -1;
}

bool TaskManager::WaitTask(common::uint32_t esp) {
    CPUState* cpustate = (CPUState*)esp;
    common::uint32_t pid = cpustate->ebx;

    if(tasks[currentTask]->pcb->pid == pid || pid == 0) {
        return false;
    }

    int index = getIndex(pid);
    if (index == -1) {
        return false;
    }

    if(numTasks <= index || tasks[index]->pcb->state == ProcessState::TERMINATED) {
        return false;
    }

    tasks[currentTask]->pcb->cpustate = cpustate;
    tasks[currentTask]->pcb->waitpid = pid;
    tasks[currentTask]->pcb->state = ProcessState::BLOCKED;
    return true;
}


CPUState* TaskManager::Schedule(CPUState* cpustate)
{
/* 
    printf(" CUR: ");
    printfInt(currentTask);
    printf(" pid: ");
    printfInt(tasks[currentTask]->pcb->pid);
 */
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask]->pcb->cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
/* 
         printf("     *CUR: ");
    printfInt(currentTask);
    printf(" *pid: ");
    printfInt(tasks[currentTask]->pcb->pid);
    
 */

    return tasks[currentTask]->pcb->cpustate;
}



    