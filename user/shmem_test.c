#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"

int main(int argc, char *argv[])
{ 
    printf("Parent before malloc: %d\n", getprocsize());
    
    char* memory =(char*)malloc(12*sizeof(char)); 
    int parent_id = getpid();
    printf("Parent after malloc: %d\n", getprocsize());
    if(fork() == 0) {

        printf("Child memory before: %d\n", getprocsize());
        char* ptr;
        
        if((ptr = (char*) map_shared_pages(parent_id,getpid(), (uint64)memory, 12*sizeof(char))) < 0){
            printf("Error in map\n");
            exit(0);
        }
        printf("Child after map %d\n", getprocsize());
        //send the string 
        strcpy(ptr, "Hello daddy");
        // int child_id=getpid();
        //unmapping
        unmap_shared_pages((uint64)ptr,12*sizeof(char));
        printf("Child after unmap %d\n", getprocsize());

        malloc(40*PGSIZE);

        printf("Child after allocating memmory: %d\n", getprocsize()); 

    }
    else {
        
        wait(0);
        printf("Parent: %s\n", memory);
        
    }

    return 0;
}


