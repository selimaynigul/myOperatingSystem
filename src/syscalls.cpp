
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
void printf(char*);

// System call enumeration
enum Syscalls {
    EXIT,
    GETPID,
    WAITPID,
    FORK,
    EXEC,
    PRINTF,
    ADDTASK
};

// Get process ID
int myos::getpid() {
    int pid = -1;
    asm("int $0x80" : "=c" (pid) : "a"(Syscalls::GETPID));
    return pid;
}

// Wait for a specific process ID
void myos::waitpid(common::uint8_t wPid) {
    asm("int $0x80" : : "a"(Syscalls::WAITPID), "b"(wPid));
}

void myos::sys_exit() {
    asm("int $0x80" : : "a"(Syscalls::EXIT));
}

void myos::sysprintf(char* str) {
    asm("int $0x80" : : "a"(Syscalls::PRINTF), "b"(str));
}

int myos::fork(int *pid) {
    asm("int $0x80" : "=c" (*pid): "a"(Syscalls::FORK));
    return *pid;
}

int myos::exec(void entrypoint()) {
    int result;
    asm("int $0x80" : "=c"(result) : "a"(Syscalls::EXEC), "b"((uint32_t)entrypoint));
    return result;
}

int myos::addTask(void entrypoint()) {
    int result;
    asm("int $0x80" : "=c"(result) : "a"(Syscalls::ADDTASK), "b"((uint32_t)entrypoint));
    return result;
}

SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;

    switch(cpu->eax)
    {
        case Syscalls::PRINTF:
            printf((char*)cpu->ebx);
            break;

        case Syscalls::EXEC: 
          //  esp = InterruptHandler::sys_exec(cpu->ebx);
            break;    

        case Syscalls::FORK: 
            cpu->ecx = InterruptHandler::sys_fork(cpu);
            return InterruptHandler::HandleInterrupt(esp);
            break;    

        case Syscalls::EXIT: 
          //  if(InterruptHandler::sys_exit()) 
           //     return InterruptHandler::HandleInterrupt(esp);
            break;    

        case Syscalls::GETPID: 
         //   cpu->ecx = InterruptHandler::sys_getpid();
            break; 

        case Syscalls::WAITPID: 
            //if(InterruptHandler::sys_waitpid()) 
              //      return InterruptHandler::HandleInterrupt(esp);
            break;    

        case Syscalls::ADDTASK: 
            //cpu->ecx = InterruptHandler::sys_addTask(cpu->ebx);
            break;  

        default:
            break;
    }
    return esp;
}

