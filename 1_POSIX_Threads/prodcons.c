#include<pthread.h>
#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<stdlib.h>

char ** buffer = NULL;
int bufferSize;
int in = 0;
int out = 0;
int count = 0;
int currentColor = 0; // first red.

pthread_mutex_t  lock;
pthread_cond_t  SpaceAvailable, ItemAvailable;

// color: red(0), blue(1), white(2)

void returnTime(char* strTime) {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
    sprintf(strTime, "%lld", llTime);
}

void * producer (int _color)
{
    int color = _color;
    char colorName[6];
    char logName[30];
    switch (color) {
        case 0: strcpy(colorName, "red"); strcpy(logName, "producer_red.log"); break;
        case 1: strcpy(colorName, "blue"); strcpy(logName, "producer_blue.log"); break;
        case 2: strcpy(colorName, "white"); strcpy(logName, "producer_white.log"); break;
        default: break;
    }
    printf("%s%s", colorName,logName);
    FILE *fp = fopen(logName, "w+");
    
    int i;
    for ( i = 0; i < 2000; i ++) {
        // Enter critical section
        pthread_mutex_lock ( &lock );
        // Make sure that buffer is NOT full
        while ( count == bufferSize || currentColor != color) {
            // Sleep using a cond variable
            while (  pthread_cond_wait( &SpaceAvailable, &lock) != 0 ) ;
            if (count == bufferSize || currentColor != color) pthread_cond_signal( &SpaceAvailable ); else break;
        }
        if (count < bufferSize) {
            // Put item in the buffer
            char curTime[40];
            returnTime(curTime);
            strcpy(buffer[in], colorName);
            strcat(buffer[in], " ");
            strcat(buffer[in], curTime);

            fprintf(fp, "%s\n", buffer[in]);
            in = (in + 1) % bufferSize;
            
            // Increment the count of items in the buffer
            count ++;
            currentColor = (color + 1) % 3;
//            printf("Deposited %dth item (color:%s(%i)), next producer is %i, after deposit count is %i\n", i, colorName, color, currentColor, count);
        }
        
        pthread_mutex_unlock ( &lock );
        // Wakeup consumer, if waiting
        pthread_cond_signal ( &ItemAvailable );
    }
    
    // PRODUCER_WHITE will put -1 in the buffer to indicate finish to the consumer
    if (color == 2) {
        pthread_mutex_lock (&lock);
        while ( count == bufferSize )
            while (  pthread_cond_wait( &SpaceAvailable, &lock) != 0 )
                ;
        strcpy(buffer[in], "-1");
        in = (in + 1) % bufferSize;
        count++;
        
        pthread_mutex_unlock ( &lock );
        // Wakeup consumer, if waiting
        pthread_cond_signal( &ItemAvailable );
    }
    fclose(fp);
    return NULL;
}

void * consumer (void *arg)
{
    char i[40];
    FILE *fp0 = fopen("consumer_red.log", "w+");
    FILE *fp1 = fopen("consumer_blue.log", "w+");
    FILE *fp2 = fopen("consumer_white.log", "w+");

    do {
        // Enter critical section
        pthread_mutex_lock ( &lock);
        // Make sure that buffer is NOT empty
        while ( count == 0 ) {
            // Sleep using a cond variable
            while (  pthread_cond_wait( &ItemAvailable, &lock) != 0 ) ;
        }
        if  ( count > 0 ) {
            // Remove item from the buffer using "out"

            strcpy(i, buffer[out]);
            strcat(i, " ");
            char curTime[40];
            returnTime(curTime);
            strcat(i, curTime);
            out = (out + 1) % bufferSize;
            // Decrement the count of items in the buffer
            count--;
            switch (i[0]) {
                case 'r': fprintf(fp0, "%s\n", i); break;
                case 'b': fprintf(fp1, "%s\n", i); break;
                case 'w': fprintf(fp2, "%s\n", i); break;
                default:  printf("wrong"); break;
            }
        }
        // exit critical section
        pthread_mutex_unlock (&lock);
        // Wakeup producer, if waiting
        pthread_cond_signal( &SpaceAvailable );
    } while (i[0] != '-');
    
    fclose(fp0);
    fclose(fp1);
    fclose(fp2);
    return NULL;

}

int main( int argc, char* argv[] )
{
    // Get buffer size
    if (argc == 2) {
        bufferSize = atoi(argv[1]);
        printf("bufferSize = %i\n", bufferSize);
    }
    else {
        printf("Please enter exactly 1 argument");
        return 0;
    }
    
    // String arrays to store the buffer; 40 chars are enough for <COLOR TIME1 TIME2>
    char list[bufferSize][40];
    // Make the global var 'buffer' point at this list
    buffer = (char **)malloc(sizeof(char*) *bufferSize);
    for (int i = 0; i < bufferSize; i ++) { buffer[i] = list[i]; }

    // how to use time of day
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    printf("Start running time:  %ld\n", currentTime.tv_sec*1000000 + currentTime.tv_usec);
    
    // thread variables
    pthread_t prod1, prod2, prod3, cons;
    pthread_mutex_init( &lock, NULL);
    pthread_cond_init( &SpaceAvailable, NULL);
    pthread_cond_init( &ItemAvailable, NULL);
    
    // Create 3 producer threads
    int n;
    if (( n = pthread_create(&prod1, NULL, producer, 0) )) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    if (( n = pthread_create(&prod2, NULL, producer, 1) )) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    if (( n = pthread_create(&prod3, NULL, producer, 2) )) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    // Create consumer thread
    if (( n = pthread_create(&cons, NULL, consumer, NULL) )) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    // Wait for the consumer thread to finish.
    if (( n = pthread_join(cons, NULL) )) {
        fprintf(stderr,"pthread_join:%s\n",strerror(n));
        exit(1);
    }
    
    free(buffer);
    printf("Finished execution \n" );
    return 0;
}
