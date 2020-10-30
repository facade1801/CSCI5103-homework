// Using 1 Grace Day
// Ce Yao, yao00136@umn.edu
// Jia Zhang, zhan7164@umn.edu
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#define DEBUG

/* consumer process executes this code */

int main(int argc, char *argv[])
{
    int id;         /* shared memory identifier */
    long long *ptr;       /* pointer to shared memory */
    id = shmget(atoi(argv[1]), 0, 0);  //get the shared memory id
    if (id == -1)
    {
        perror ("consumer shmget failed");
        exit (1);
    }

    /* Now attach this segment into the address space.  The 1023 is a
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
    FILE *fp1 = fopen("consumer_red.log", "w+");
    FILE *fp2 = fopen("consumer_blue.log", "w+");
    FILE *fp3 = fopen("consumer_white.log", "w+");
    int consume_total=0;
    int current_color=1; //1 for red, 2 for blue,3 for white
    while(consume_total<6000) //means consumer process still needs running
    {
        while(ptr[1]==0);//the buffer is empty
        while(ptr[3]==0);//producer process is running
        ptr[3]=0;
        ptr[1]--;
        consume_total++;
        long long head=ptr[7];//get the position of head pointer
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;//get current time
        if(current_color==1)//it's red product
        {
            fprintf(fp1, "red %lld %lld\n", ptr[head],llTime);
            fflush(fp1);
            current_color++;
        }
        else if(current_color==2)//it's blue product
        {
            fprintf(fp2, "blue %lld %lld\n", ptr[head],llTime);
            fflush(fp2);
            current_color++;
        }
        else //it's white product
        {
            fprintf(fp3, "white %lld %lld\n", ptr[head],llTime);
            fflush(fp3);
            current_color=1;
        }
        head=(head-10+1)%ptr[2]+10;//update head position
        ptr[7]=head;
        ptr[3]=1;
    }
    printf("%d\n",consume_total);
    while(ptr[9]!=3);//producer process is still running
    shmctl (id, IPC_RMID, NULL);
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    return 0;
}
