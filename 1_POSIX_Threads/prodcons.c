#include<pthread.h>
#include<stdio.h>
#include<string.h>
// #include<unistd.h>

const int N = 5;
int Buffer[5];
int in = 0;
int out = 0;
int count = 0;

pthread_mutex_t  lock;
pthread_cond_t  SpaceAvailable, ItemAvailable;


void * producer (void *arg)
{
	int i;
    for ( i = 0; i< 1000; i++) {
        // Enter critical section
        pthread_mutex_lock ( &lock);
        // Make sure that buffer is NOT full
        while ( count == N )
            // Sleep using a cond variable
            while (  pthread_cond_wait( &SpaceAvailable, &lock) != 0 )
            	;
        // printf( "Producer %d \n", i);
        
        // count must be less than N
        // Put item in the buffer using "in"
        Buffer[in] = i;
        in = (in + 1) % N;
        
        // Increment the count of items in the buffer
        count++;
        printf("Deposited item = %d\n", i);
        
        pthread_mutex_unlock ( &lock);
        // Wakeup consumer, if waiting
        pthread_cond_signal( &ItemAvailable );
    }
    
    
    // Put -1 in the buffer to indicate finish to the consumer
    pthread_mutex_lock ( &lock);
    while ( count == N )
        while (  pthread_cond_wait( &SpaceAvailable, &lock) != 0 )
        	;
    Buffer[in] = -1;
    in = (in + 1) % N;
    count++;
    
    pthread_mutex_unlock ( &lock );
    // Wakeup consumer, if waiting
    pthread_cond_signal( &ItemAvailable );
}

void * consumer (void *arg)
{
	int i;
    i = 0;
    do {
        // Enter critical section
        pthread_mutex_lock ( &lock);
        // Make sure that buffer is NOT empty
        while ( count == 0 )
            // Sleep using a cond variable
            while (  pthread_cond_wait( &ItemAvailable, &lock) != 0 )
            	;
        
        // count must be > 0
        if  ( count > 0 ) {
            // Remove item form the buffer using "out"
            i = Buffer[out] ;
            out = (out + 1) % N;
            // Decrement the count of items in the buffer
            count--;
            printf( "Removed %d \n", i);
        }
        // exit critical seciton
        pthread_mutex_unlock ( &lock);
        // Wakeup prodcuer, if waiting
        pthread_cond_signal( &SpaceAvailable );
    } while ( i != -1 );
}

int main( int argc, char* argv[] )
{
    // thread variables
    pthread_t prod, cons;
    // attribute object
    pthread_attr_t attr;
    int n;
    
    pthread_mutex_init( &lock, NULL);
    pthread_cond_init( &SpaceAvailable, NULL);
    pthread_cond_init( &ItemAvailable, NULL);
    
    // Create producer thread
    if ( n = pthread_create(&prod, NULL, producer ,NULL)) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    
    // Create consumer thread
    if (  n = pthread_create(&cons, NULL, consumer, NULL) ) {
        fprintf(stderr,"pthread_create :%s\n",strerror(n));
        exit(1);
    }
    
    // Wait for the consumer thread to finish.
    if ( n = pthread_join(cons, NULL) ) {
        fprintf(stderr,"pthread_join:%s\n",strerror(n));
        exit(1);
    }
    
    printf("Finished execution \n" );
    return 0;
}
