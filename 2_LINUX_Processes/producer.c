#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define DEBUG

/* producer process executes this code */

int main(int argc, char *argv[])
{
    int id;         /* shared memory identifier */
    long long *ptr;       /* pointer to shared memory */
    id = shmget(atoi(argv[1]), 0, 0);
    if (id == -1)
    {
        perror ("producer shmget failed");
        exit (1);
    }
    /* Now attach this segment into the address space. The 1023 is a
        flag consisting of all 1s, and the NULL pointer means we don't care
        where in the address space the segment is attached
    */
    ptr = shmat (id, (void *) NULL, 1023);
    if (ptr == (void *) -1)
    {
        perror ("producer shmat failed");
        exit (2);
    }
    long long size=ptr[2];
    int red_total,blue_total,white_total;
    red_total=0;
    blue_total=0;
    white_total=0;
    FILE *fp1 = fopen("producer_red.log", "w+");
    FILE *fp2 = fopen("producer_blue.log", "w+");
    FILE *fp3 = fopen("producer_white.log", "w+");
    if(strcmp(argv[2],"red")==0)
    {
        while(red_total<7)
        {
            while(ptr[4]==0); //red mutex
            ptr[4]--;
            while(ptr[1]==ptr[2]);
            while(ptr[3]==0);
            ptr[3]--;
            long long tail = ptr[8];
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
            ptr[tail]=llTime;
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;
            ptr[0]++;
            ptr[1]++;
            ptr[5]=1;
            red_total++;
            fprintf(fp1, "red %lld\n", llTime);
            fflush(fp1);
            ptr[3]++;
        }
        fclose(fp1);
    }
    else if(strcmp(argv[2],"blue")==0)
    {
        while(blue_total<7)
        {
            while(ptr[5]==0);//blue mutex
            ptr[5]--;
            while(ptr[1]==ptr[2]);
            while(ptr[3]==0);
            ptr[3]--;
            long long tail = ptr[8];
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
            ptr[tail]=llTime;
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;
            ptr[0]++;
            ptr[1]++;
            ptr[6]=1;
            blue_total++;
            fprintf(fp2, "blue %lld\n", llTime);
            fflush(fp2);
            ptr[3]++;
        }
        fclose(fp2);
    }
    else
    {
        while(white_total<6)
        {
            while(ptr[6]==0);//white mutex
            ptr[6]--;
            while(ptr[1]==ptr[2]);
            while(ptr[3]==0);
            ptr[3]--;
            long long tail = ptr[8];
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
            ptr[tail]=llTime;
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;
            ptr[0]++;
            ptr[1]++;
            if(ptr[0]<20)
            ptr[4]=1;
            white_total++;
            fprintf(fp3, "white %lld\n", llTime);
            fflush(fp3);
            ptr[3]++;
        }
        fclose(fp3);
    }
    ptr[9]++;
    if(shmdt ( (void *)  ptr)==-1)
    perror("wrong shmdt");
    return 0;
}
