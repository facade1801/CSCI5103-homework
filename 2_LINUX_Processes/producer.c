#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define DEBUG

/* Child process executes this code */

int main(int argc, char *argv[])
{
    int id;         /* shared memory identifier */
    int *ptr;       /* pointer to shared memory */
    /* A key for the shared memory segment has been passed to this program
        as its first parameter. We use it to get the segment id of the
        segment that the parent process created. The size parameter is set
        to zero, indicating that the segment is
        not being created here, it already exists
    */
    //printf("%s\n",argv[1]);
    id = shmget(atoi(argv[1]), 0, 0);
    if (id == -1)
    {
        perror ("producer shmget failed");
        exit (1);
    }
    #ifdef DEBUG
    //printf ("producer Got shmem id = %d\n", id);
    #endif

    /* Now attach this segment into the address space. Again, the 1023 is a
        flag consisting of all 1s, and the NULL pointer means we don't care
        where in the address space the segment is attached
    */
    ptr = shmat (id, (void *) NULL, 1023);
    if (ptr == (void *) -1)
    {
        perror ("producer shmat failed");
        exit (2);
    }
    #ifdef DEBUG
    //printf ("producer Got ptr = %p\n", ptr);
    #endif
    int size=ptr[2];
    if(strcmp(argv[2],"red")==0)
    {
        //printf("Pro Red\n");
        while(ptr[0]<20)
        {
            while(ptr[4]==0); //red mutex
            ptr[4]--;
            while(ptr[1]==ptr[2]);
            if(ptr[0]<20)
            {
                while(ptr[3]==0);
                ptr[3]--;
                int tail = ptr[8];
                ptr[tail]=0;
                tail=(tail+1-9)%size+9;
                ptr[8]=tail;
                ptr[0]++;
                ptr[1]++;
                ptr[5]=1;
                printf("now produce red product\n");
                ptr[3]++;
            }
        }
    }
    else if(strcmp(argv[2],"blue")==0)
    {
        //printf("Pro Blue\n");
        while(ptr[0]<20)
        {
            while(ptr[5]==0);//blue mutex
            ptr[5]--;
            while(ptr[1]==ptr[2]);
            if(ptr[0]<20)
            {
                while(ptr[3]==0);
                ptr[3]--;
                int tail = ptr[8];
                ptr[tail]=1;
                tail=(tail+1-9)%size+9;
                ptr[8]=tail;
                ptr[0]++;
                ptr[1]++;
                ptr[6]=1;
                printf("now produce blue product\n");
                ptr[3]++;
            }
        }
    }
    else if(strcmp(argv[2],"white")==0)
    {
        //printf("Pro white\n");
        while(ptr[0]<20)
        {
            while(ptr[6]==0);//white mutex
            ptr[6]--;
            while(ptr[1]==ptr[2]);
            if(ptr[0]<20)
            {
                while(ptr[3]==0);
                ptr[3]--;
                int tail = ptr[8];
                ptr[tail]=2;
                tail=(tail+1-9)%size+9;
                ptr[8]=tail;
                ptr[0]++;
                ptr[1]++;
                ptr[4]=1;
                printf("now produce white product\n");
                ptr[3]++;
            }
        }
    }
    shmdt ( (void *)  ptr);
    return 0;
}
