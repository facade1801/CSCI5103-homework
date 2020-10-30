// Using 1 Grace Day
// Ce Yao, yao00136@umn.edu
// Jia Zhang, zhan7164@umn.edu
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
    id = shmget(atoi(argv[1]), 0, 0); //get the shared memory id
    if (id == -1)
    {
        perror ("producer shmget failed");
        exit (1);
    }
    /* Now attach this segment into the address space. The 1023 is a
        flag consisting of all 1s, and the NULL pointer means we don't care
        where in the address space the segment is attached
    */
    ptr = shmat (id, (void *) NULL, 1023); //locate the shared memory
    if (ptr == (void *) -1)
    {
        perror ("producer shmat failed");
        exit (2);
    }
    long long size=ptr[2]; //get the buffer size from shared memory
    int red_total,blue_total,white_total; //record the number of product each producer produce
    red_total=0;
    blue_total=0;
    white_total=0;
    FILE *fp1 = fopen("producer_red.log", "w+");
    FILE *fp2 = fopen("producer_blue.log", "w+");
    FILE *fp3 = fopen("producer_white.log", "w+");
    if(strcmp(argv[2],"red")==0)//go into red producer process
    {
        while(red_total<2000) //means red producer still needs running
        {
            while(ptr[4]==0); //red semaphore
            ptr[4]--;
            while(ptr[1]==ptr[2]);//ptr[1] store the product number, equal means buffer is full
            while(ptr[3]==0); //semaphore
            ptr[3]=0; //set red semaphore = 0
            long long tail = ptr[8];//get the tail pointer position
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;//get the current time
            ptr[tail]=llTime; //store the current time into the tail position
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;//update the tail position
            ptr[0]++; //total number of product plus 1
            ptr[1]++; //number of product in the buffer plus 1
            red_total++;
            fprintf(fp1, "red %lld\n", llTime);//write information into file
            fflush(fp1);
            ptr[5]=1; //set blue semaphore = 1
            ptr[3]=1;//set the semaphore = 1
        }
        fclose(fp1);
        printf("%d\n",red_total);
    }
    else if(strcmp(argv[2],"blue")==0)//go into blue producer process
    {
        while(blue_total<2000)
        {
            while(ptr[5]==0);//blue semaphore
            ptr[5]--;
            while(ptr[1]==ptr[2]);
            while(ptr[3]==0);
            ptr[3]=0;
            long long tail = ptr[8];
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
            ptr[tail]=llTime;
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;
            ptr[0]++;
            ptr[1]++;
            blue_total++;
            fprintf(fp2, "blue %lld\n", llTime);
            fflush(fp2);
            ptr[6]=1;
            ptr[3]=1;
        }
        fclose(fp2);
        printf("%d\n",blue_total);
    }
    else//go into white producer process
    {
        while(white_total<2000)
        {
            while(ptr[6]==0);//white semaphore
            ptr[6]--;
            while(ptr[1]==ptr[2]);
            while(ptr[3]==0);
            ptr[3]=0;
            long long tail = ptr[8];
            struct timeval currentTime;
            gettimeofday(&currentTime, NULL);
            long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
            ptr[tail]=llTime;
            tail=(tail+1-10)%size+10;
            ptr[8]=tail;
            ptr[0]++;
            ptr[1]++;
            white_total++;
            fprintf(fp3, "white %lld\n", llTime);
            fflush(fp3);
            ptr[4]=1;
            ptr[3]=1;
        }
        fclose(fp3);
        printf("%d\n",white_total);
    }
    ptr[9]++; //add the number in the ptr[9], if it equals 3, this means
                //all the producer process are finished
    if(shmdt ( (void *)  ptr)==-1)
    perror("wrong shmdt");
    return 0;
}
