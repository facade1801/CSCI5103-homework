#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>

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

    id = shmget(atoi(argv[1]), 0, 0);
    if (id == -1)
    {
        perror ("consumer shmget failed");
        exit (1);
    }
    #ifdef DEBUG
    //printf ("consumer Got shmem id = %d\n", id);
    #endif

    /* Now attach this segment into the address space. Again, the 1023 is a
        flag consisting of all 1s, and the NULL pointer means we don't care
        where in the address space the segment is attached
    */
    ptr = shmat (id, (void *) NULL,1023);
    if (ptr == (void *) -1)
    {
        perror ("consumer shmat failed");
        exit (2);
    }
    #ifdef DEBUG
    //printf ("consumer Got ptr = %p\n", ptr);
    #endif
    int consume_total=0;
    while(consume_total<20)
    {
        while(ptr[1]==0);
        while(ptr[3]==0);
        consume_total++;
        //printf("%d\n",consume_total);
        ptr[3]--;
        ptr[1]--;
        int head=ptr[7];
        int color=ptr[head];
        if(color==0)
        printf("remove item red\n");
        else if(color==1)
        printf("remove item blue\n");
        else
        printf("remove item white\n");
        head=(head-10+1)%ptr[2]+10;
        ptr[7]=head;
        ptr[3]++;
    }
    //printf("Con\n");
    //printf("%d\n",ptr[0],ptr[1],ptr)
    //ptr[0]=100;
    printf("consumer exit\n");
    ptr[9]++;
    if(shmdt ( (void *)  ptr)==-1)
    perror("wrong shmdt consumer");
    //shmctl (id, IPC_RMID, NULL);
    return 0;


    /* Now check the 50th integer in the shared memory space. The parent
        had placed the value 676 there. So this statement should print
        out the same value. But what about "race conditions"? What happens
        if the child process executes this before the parent has had a chance
        to store the value 676? Can this happen? If so, how can you prevent
        it from happening? (Synchronization)
    */
    //printf ("child sees %d\n", ptr[50]);

    /* Done with the program, so detach the shared segment and terminate */
    /*
    shmdt ( (void *) ptr);
    */

    /* The following is one way of destroying the shared memory segment
        and returning it to the system. I can do this safely here, because
        I know the parent program won't be using the shared memory any more.
        In general, you can only do this safely when ALL processes that used
        the memory are known to have detached the segment using shmdt(). Look
        up the shmctl man page for details.
    */

    //shmctl (id, IPC_RMID, NULL);

    /* If you don't destroy the segment using shmctl, it will remain allocated
        even after all your processes terminate! You can check the status of
        your shared memory segments using the 'ipcs' command. From the Unix
        command line, type: ipcs -m
        To destroy any segments belonging to you, use the command:
        ipcrm -m <shm_ident>
    */
}
