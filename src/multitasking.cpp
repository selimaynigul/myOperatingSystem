
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

bool TaskManager::InitTask(Task &task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}


common::uint32_t TaskManager::getpid() {
    return tasks[currentTask].pid;
}

 common::uint32_t TaskManager::ForkTask2(CPUState* cpustate)
{
    if(numTasks >= 256)
        return 0;
 
    tasks[numTasks].state=ProcessState::READY;
    tasks[numTasks].ppid=tasks[currentTask].pid;
   // tasks[numTasks].pid=Task::pIdCounter++;
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++)
    {
        tasks[numTasks].stack[i]=tasks[currentTask].stack[i];
    }
    //Stackten yer alında cpustate'in konumu değişiyor bu nedenle şuanki taskın offsetini hesaplayıp yeni oluşan process'in cpu statenin konumunu ona göre düzenliyorum. Bu işlemi yapmazsam process düzgün şekilde devam etmiyor.
    common::uint32_t currentTaskOffset=(((common::uint32_t)cpustate - (common::uint32_t) tasks[currentTask].stack));
    tasks[numTasks].cpustate=(CPUState*)(((common::uint32_t) tasks[numTasks].stack) + currentTaskOffset);
 
    //Burada ECX' yeni taskın process id'sini atıyorum. Syscall'a return edebilmek için.
    tasks[numTasks].cpustate->ecx = 0;
    numTasks++;
    return tasks[numTasks-1].pid;
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
    CPUState* cpustate = (CPUState*)esp;
    common::uint32_t pid = cpustate->ebx;

    if(tasks[currentTask].pid == pid || pid == 0) {
        return false;
    }

    int index = getIndex(pid);
    if (index == -1) {
        return false;
    }

    if(numTasks <= index || tasks[index].state == ProcessState::TERMINATED) {
        return false;
    }

    tasks[currentTask].cpustate = cpustate;
    tasks[currentTask].waitpid = pid;
    tasks[currentTask].state = ProcessState::BLOCKED;
    return true;
}


CPUState* TaskManager::Schedule(CPUState* cpustate)
{

    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;


    return tasks[currentTask].cpustate;
}



    