#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG

int main(int argc, char *argv[])
{
    int shmem_id;       /* shared memory identifier */
    long long *shmem_ptr;     /* pointer to shared segment */
    key_t key;          /* A key to access shared memory segments */
    int size;           /* Memory size needed, in bytes */
    int flag;           /* Controls things like r/w permissions */
    key = 6;
    size = 20480;
    flag = 1023;
    shmem_id = shmget (key, size, flag);
    if (shmem_id == -1)
    {
        perror ("shmget failed");
        exit (1);
    }
    shmem_ptr = shmat (shmem_id, (void *) NULL, 1023);
    if (shmem_ptr == (void *) -1)
    {
        perror ("shmat failed");
        exit (2);
    }
    //the following I allocate 9 position of shared memory to store different information
    shmem_ptr[0]=0; //store the total number of products producer produces
    shmem_ptr[1]=0; //store the number of products in the buffer
    shmem_ptr[2]=(atoi(argv[1]));//store the buffer size N, which will get from user
    shmem_ptr[3]=1; //semaphore
    shmem_ptr[4]=1; //red product semaphore
    shmem_ptr[5]=0; //blue product semaphore
    shmem_ptr[6]=0; //white product semaphore
    shmem_ptr[7]=10; //store the position of head
    shmem_ptr[8]=10; //store the position of tail
    shmem_ptr[9]=0; //use this value to see if three consumer process are done

    int pid=fork();
    char keystr[10];
    sprintf (keystr, "%d", key);
    if(pid==0)//white producer process
    {
        if(execl("./producer", "producer",keystr,"white", NULL)==-1)
        {
            perror("execl failed for producer white");
        }
    }
    else
    {
        int pid1=fork();
        if(pid1==0) //blue producer process
        {
            if(execl("./producer", "producer",keystr,"blue", NULL)==-1)
            {
                perror("execl failed for producer blue");
            }
        }
        else
        {
            int pid2=fork();
            if(pid2==0) //red producer process
            {
                if(execl("./producer", "producer",keystr,"red", NULL)==-1)
                {
                    perror("execl failed for producer red");
                }
            }
            else //consumer process
            {
                if(execl("./consumer", "consumer",keystr,NULL)==-1)
                {
                    perror("execl failed for consumer");
                }
            }
        }
    }
    return 0;
}
