#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>


#include <drivers/amd_am79c973.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

GlobalDescriptorTable gdt;
TaskManager taskManager(&gdt);
InterruptManager interrupts(0x20, &gdt, &taskManager);
SyscallHandler syscalls(&interrupts, 0x80);

// #define GRAPHICSMODE




void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}


void printfInt(int num)
{
    char buffer[32]; // Assuming a maximum of 32 digits for an int
    int i = 0;
    if (num < 0) {
        printf("-");
        num = -num;
    }
    if (num == 0) {
        printf("0");
        return;
    }
    while (num != 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    buffer[i] = '\0'; // Null-terminate the string
    char reversed_buffer[32];
    int j = 0;
    // Reverse the buffer to get the correct order of digits
    for (int k = i - 1; k >= 0; --k) {
        reversed_buffer[j++] = buffer[k];
    }
    reversed_buffer[j] = '\0'; // Null-terminate the reversed string
    printf(reversed_buffer); // Print the reversed string
}




class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};

void longTask() {
    printf("Long task started...\n");
    int n = 100000;
    int result = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result = i * j;
        }
    }
    printf("Long task is finished.\n");
}

void linearSearch() {
    int arr[] = {1,2,3,4,5,6,7,8,9,10};
    int size = 10;
    int target = 6;
    int result;
    for (int i = 0; i < size; ++i) {
        if (arr[i] == target) {
            result = i;
            return;
        }
    }
    result = -1;
}

void binarySearch() {
    int arr[] = {1,2,3,4,5,6,7,8,9,10};
    int size = 10;
    int target = 6;

    int left = 0;
    int right = size - 1;

    int result;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid] == target) {
            result = mid; 
            return;
        }
        if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1; 
        }
    }
    result = -1;
}

void collatzSequence() {
    printf("Collatz started...\n");
    int num = 2;
    while (num < 100) {
        int n = num;
        while (n != 1) {
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
        }
        num++;
    }
    printf("Collatz is finished.\n");
}

void my_sleep2() {
    int n = 100000;
    int result = 0;
    for (int i = 0; i < n; ++i) 
        for (int j = 0; j < n; ++j) 
            result = i * j; 
}

void partA_process() {   
    printf("PART A is running\n"); 

    int pid, pid2, pid3;

    int returnedPid = fork(&pid); // first fork

    if (returnedPid == pid) {
        returnedPid = fork(&pid2); // second fork
        if (returnedPid == pid2) {
            returnedPid = fork(&pid3); // third fork
            if (returnedPid == pid3) {   // PARENT PROCESS
                printf("All child processes are forked successfully.\n");
                printf("\nProcess table after forked all childs:\n");
                taskManager.PrintProcessTable();
                waitpid(pid);
                waitpid(pid2);
                waitpid(pid3);
                printf("\nProcess table after all childs end:\n");
                taskManager.PrintProcessTable();
                printf("\nPART A is successfully completed.\n\n");
                exit();
            } 
            else { // FIRST CHILD
                collatzSequence();
                longTask();
                exit();
            }
        } 
        else { // SECOND CHILD
            collatzSequence();
            longTask();
            exit();
        } 
    } 
    else { // THIRD CHILD
        collatzSequence();
        longTask();
        exit();
    }
}


unsigned int seed = 0; // Seed value for the random number generator

int my_rand() {
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return (int)(seed >> 16);
}

void partB_first() {
    printf("PART B the first strategy is running.\n");

    // Array of function pointers to the available programs
    void (*programs[])() = {longTask, collatzSequence, binarySearch, linearSearch};
    int numPrograms = sizeof(programs) / sizeof(programs[0]);

    // Randomly choose one of the programs
    int chosenProgramIndex = my_rand() % numPrograms;
    void (*chosenProgram)() = programs[chosenProgramIndex];

    int pids[10];

    for (int i = 0; i < 10; ++i) {
        int pid;
        int returnedPid = fork(&pid);
        if (returnedPid == pid) {
            pids[i] = pid; // Store the PID of the child process
        } else {
            chosenProgram();
            exit(); 
        } 
    }

    printf("10 processes started\n");

    for (int i = 0; i < 10; ++i) {
        waitpid(pids[i]);
    }

    printf("\nPART B the first strategy is successfully finished.\n");
}


void partB_second() {
    printf("PART B the second strategy is running.\n");
 
    void (*programs[])() = {longTask, collatzSequence, binarySearch, linearSearch};
    int numPrograms = sizeof(programs) / sizeof(programs[0]);
    int pids[6];

    // Choose 2 programs randomly
    int chosen_programs[2];
    for (int i = 0; i < 2; ++i) {
        int chosenProgramIndex = my_rand() % numPrograms;
        chosen_programs[i] = chosenProgramIndex; // Store the index of the chosen program
    }

    // Load each chosen program 3 times
    int index = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            int pid;
            int returnedPid = fork(&pid);
            if (returnedPid == pid) {
                pids[index++] = pid;
            }
            else {
                programs[chosen_programs[j]]();
                exit();
            }
        }
    }

    // Wait for all child processes to terminate
     for (int i = 0; i < 6; ++i) {
        waitpid(pids[i]);
    }

    printf("\nPART B the second strategy is successfully finished.\n");
}


void partB_third() {
    printf("PART B the preemptive strategy is running\n");   

    int pid;
    int pid2;
    int pid3;
    int pid4;

    int returnedPid = fork(&pid);

    if (returnedPid == pid) {
       // taskManager.PrintProcessTable();
    }
    else {
        collatzSequence();
        exit();
    }

    returnedPid = fork(&pid2);
    if (returnedPid == pid2) {
    }
    else {
        longTask();
        exit();
    }

    returnedPid = fork(&pid3);
    if (returnedPid == pid3) {
    }
    else {
        binarySearch();
        exit();
    }

    returnedPid = fork(&pid4);
    if (returnedPid == pid4) {
    }
    else {
        linearSearch();
        exit();
    }

    waitpid(pid);
    waitpid(pid2);
    waitpid(pid3);
    waitpid(pid4);

    printf("PART B dynamic priority strategy is successfully finished.\n");
}

 void initProcess() { 
    
    switch (interrupts.strategy) {
        case 0:
            partA_process();
            break;
        
        case 1:
            partB_first();
            break;

        case 2:
            partB_second();
            break;
        
        case 3:
            partB_third();
            break;
        
        case 4:
            partB_third();
            break;
        
        default:
            break;
    }     
 
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World!\n\n");
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    void* allocated = memoryManager.malloc(1024);

    Task initTask(&gdt, initProcess);
    taskManager.InitTask(&initTask);
    
 
    /*
    printf("Initializing Hardware, Stage 1\n");
    
    #ifdef GRAPHICSMODE
        Desktop desktop(320,200, 0x00,0x00,0xA8);
    #endif
    
    DriverManager drvManager;
    
        #ifdef GRAPHICSMODE
            KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif
        drvManager.AddDriver(&keyboard);
        
    
        #ifdef GRAPHICSMODE
            MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        drvManager.AddDriver(&mouse);
        
        PeripheralComponentInterconnectController PCIController;
        PCIController.SelectDrivers(&drvManager, &interrupts);

        #ifdef GRAPHICSMODE
            VideoGraphicsArray vga;
        #endif
        
    printf("Initializing Hardware, Stage 2\n");
        drvManager.ActivateAll();
        
    printf("Initializing Hardware, Stage 3\n");

    #ifdef GRAPHICSMODE
        vga.SetMode(320,200,8);
        Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
        desktop.AddChild(&win2);
    #endif
*/

    /*
    printf("\nS-ATA primary master: ");
    AdvancedTechnologyAttachment ata0m(true, 0x1F0);
    ata0m.Identify();
    
    printf("\nS-ATA primary slave: ");
    AdvancedTechnologyAttachment ata0s(false, 0x1F0);
    ata0s.Identify();
    ata0s.Write28(0, (uint8_t*)"http://www.AlgorithMan.de", 25);
    ata0s.Flush();
    ata0s.Read28(0, 25);
    
    printf("\nS-ATA secondary master: ");
    AdvancedTechnologyAttachment ata1m(true, 0x170);
    ata1m.Identify();
    
    printf("\nS-ATA secondary slave: ");
    AdvancedTechnologyAttachment ata1s(false, 0x170);
    ata1s.Identify();
    // third: 0x1E8
    // fourth: 0x168
    */
    
    
   // amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);
   // eth0->Send((uint8_t*)"Hello Network", 13);
        

    interrupts.Activate();


    while(1)
    {

        /*
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
        */
    }
}
