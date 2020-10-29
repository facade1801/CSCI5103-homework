#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define DEBUG

/* Simple example of shared memory usage */
/* Make sure your current working directory is the one these files are in */
/* Then execute `parent' from this directory */

int main(int argc, char *argv[])
{
    int shmem_id;       /* shared memory identifier */
    int *shmem_ptr;     /* pointer to shared segment */
    key_t key;          /* A key to access shared memory segments */
    int size;           /* Memory size needed, in bytes */
    int flag;           /* Controls things like r/w permissions */
    key = 4455;         /* Some arbitrary integer, which will also be
                            passed to the other processes which need to
                            share memory */
    size = 20480;        /* Assume we need 2Kb of memory, which means we
                            can store 512 integers or floats */
    flag = 1023;        /* 1023 = 111111111 in binary, i.e. all permissions
                            and modes are set. This may not be appropriate
                            for all programs! */
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
    shmem_ptr[0]=0; //set the total sum of products producer produce
    shmem_ptr[1]=0; //set the count of product
    shmem_ptr[2]=(atoi(argv[1]));//store the buffer size N into ptr[2]
    shmem_ptr[3]=1; //red product mutex
    shmem_ptr[4]=0; //blue product mutex
    shmem_ptr[5]=0; //white product mutex
    shmem_ptr[6]=0; //store the position of head
    shmem_ptr[7]=0; //store the position of tail

    int red_id,blue_id,white_id,consumer_id;
    int pid=fork();
    char keystr[10];
    sprintf (keystr, "%d", key);
    if(pid==0)//child process
    {
        white_id=pid;
        //printf("now is white producer\n");
        //printf("white keystr %s\n",keystr);
        if(execl("./producer", "producer",keystr,"white", NULL)==-1)
        {
            perror("execl failed for producer white");
        }
    }
    else
    {
        int pid1=fork();
        if(pid1==0)
        {
            blue_id=pid1;
            //printf("now is blue producer\n");
            //printf("blue keystr %s\n",keystr);
            if(execl("./producer", "producer",keystr,"blue", NULL)==-1)
            {
                perror("execl failed for producer blue");
            }
        }
        else
        {
            int pid2=fork();
            if(pid2==0)
            {
                red_id=pid2;
                //printf("now is red producer\n");
                //printf("red keystr %s\n",keystr);
                if(execl("./producer", "producer",keystr,"red", NULL)==-1)
                {
                    perror("execl failed for producer red");
                }
            }
            else
            {
                consumer_id=pid2;
                //printf("now is consumer\n");
                //printf("consumer keystr %s\n",keystr);
                if(execl("./consumer", "consumer",keystr,NULL)==-1)
                {
                    perror("execl failed for consumer");
                }
            }
        }
    }
    //while(shmem_ptr[0]!=100);
    //shmctl (shmem_id, IPC_RMID, NULL);
    return 0;
}
