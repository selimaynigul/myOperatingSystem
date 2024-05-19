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


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;



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
//printf("LongEntered \n");
    int n = 100000;
    int result = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result = i * j;
          
        }
    }
    printf("LongExitted \n");
    //while(true);
}


void collatzTask() {
    int k = 0;
    for (int i = 1; i <= 100; ++i) {
       // printf("Output: ");
       // printfInt(i);
         k++;
        while (i != 1) {
          //  printfInt(i);
          //  printf(",");
    
            if (i % 2 == 0) {
               i /= 2;
            } else {
               i = 3 *i + 1;
            }
        } 
      //  printf("1");
    }
  //  while(true);
}

void childProcess() {
    printf("child\n");
    //collatzTask();
    longTask();
    exit();
}
/*
 void initProcess()
{      
    int pid = 0;
    int pid2;
    int pid3;
    
    int returnedPid = fork(&pid);
    printf("FORKED\n");
   
    if (returnedPid == pid) {
        printf("child icindeyim\n");
        longTask();
    } 
    else {
        printf("parent icindeyim pid: ");
        printfInt(pid);
        printf("\n");
        returnedPid = fork(&pid2);
        printf("FORKED in 2. parent\n");
        if (pid2 == returnedPid) {
            printf("ikinci child icindeyim\n");
            longTask();
        } 
        else {
            printf("ikinci parent icindeyim pid: ");
            printfInt(pid2);
            printf("\n");
            returnedPid = fork(&pid3);
            printf("FORKED in 3. parent\n");
            if (pid3 == returnedPid) {
                printf("ucuncu child icindeyim\n");
                longTask(); 
            } 
            else {
                printf("ucuncu parent icindeyim pid: ");
                printfInt(pid3);
                printf("\n");
                waitpid(pid3);
            }
        } 
    //while(true);
    }

    printf(" CIKTIM ");
}
  */

/*  void initProcess() {      

    int pid;
    int pid2;
    int pid3;

    int returnedPid = fork(&pid);

    if (returnedPid == pid) {
        childProcess();
    } 
    else {
        returnedPid = fork(&pid2);
        if (returnedPid == pid2) {
            childProcess();
        } 
        else {
            returnedPid = fork(&pid3);
            if (returnedPid == pid3) {
                childProcess();
            } 
            else {
                printf("parent ");
                printfInt(pid3);
                printf("\n");
            }
        } 
    }
    while(true);
    printf("exitting... ");
}  *//* 
 
 void initProcess() {      
    int pid1, pid2, pid3;

    int returnedPid1 = fork(&pid1);
    printf("forked\n");
    if (returnedPid1 == pid1) {
        printf("Forked first child with PID ");
        printfInt(pid1);
        printf("\n");
        waitpid(pid1);
    } 
    else {
        printf(" first child ");
        longTask();
        exit();
    }

    printf(" HEREEEE ");

    int returnedPid2 = fork(&pid2);
    if (returnedPid2 == pid2) {
        printf("Forked second child with PID ");
        printfInt(pid2);
        printf("\n");
        waitpid(pid2);
    } 
    else {
        printf(" second child ");
        longTask();
        exit();   
    }

    int returnedPid3 = fork(&pid3);
    if (returnedPid3 == pid3) {
        printf("Forked third child with PID ");
        printfInt(pid3);
        printf("\n");
        waitpid(pid3);
    } 
    else {
        printf(" third child ");
        longTask();
        exit();  
    }
    while(true);
    printf("All children exited\n");
}  */


 void initProcess() {      

    int pid;
    int pid2;
    int pid3;

    int returnedPid = fork(&pid);
    printf("forked\n");

    if (returnedPid == pid) {
        returnedPid = fork(&pid2);
        if (returnedPid == pid2) {
            returnedPid = fork(&pid3);
            if (returnedPid == pid3) {
                printf("parent ");
                printfInt(pid3);
                printf("\n");
                waitpid(pid);
                waitpid(pid2);
                waitpid(pid3);
                printf("finish\n");
            } 
            else {
                childProcess();
            }
        } 
        else {
            childProcess();
        } 
    } 
    else {
        childProcess();
    }
    while(true);
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
    printf("Hello World!\n");
    GlobalDescriptorTable gdt;
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    void* allocated = memoryManager.malloc(1024);

    TaskManager taskManager(&gdt);
    Task initTask(&gdt, initProcess);
    taskManager.InitTask(&initTask);
    
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);
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
