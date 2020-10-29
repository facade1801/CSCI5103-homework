#include<pthread.h>
#include<stdio.h>
#include<string.h>
// #include<unistd.h>
#include <sys/time.h>


const int N = 5;
int Buffer[5];
int in = 0;
int out = 0;
int count = 0;
int currentColor = 0; // first red.
int howManyColor = 2;

pthread_mutex_t  lock;
pthread_cond_t  SpaceAvailable, ItemAvailable;

// color: red(0), blue(1), white(2)
void * producer (int _color)
{
	int color = _color;
	printf("Producer color is %d\n", color);
	char* colorName;
	colorName = (color == 0) ? "red" : (color == 1) ? "blue" : "white";
	printf("color is %s\n", colorName);

	int i;
	for ( i = 0; i < 20; i ++) {
		// Enter critical section
		pthread_mutex_lock ( &lock );
		// Make sure that buffer is NOT full
		while ( count == N || currentColor != color) {
			// Sleep using a cond variable
			while (  pthread_cond_wait( &SpaceAvailable, &lock) != 0 ) {
				// printf("%s(%i) is waiting\n", colorName, color);
				// sleep(1);
				;
			}
				// printf("%s is waiting, count = %i, currentColor should be %i\n", colorName, count, currentColor);
		}
		printf("Producer >>> %s GOT the key\n", colorName);
		// count must be less than N. Otherwise throw the lock
		if (count < N) {
			// Put item in the buffer
			Buffer[in] = i;
			in = (in + 1) % N;

			// Increment the count of items in the buffer
			count ++;
			currentColor = (currentColor + 1) % 3;
			printf("Deposited %dth item (color:%s(%i)), next producer is %i\n", i, colorName, color, currentColor);
		}
		
		printf("Producer >>> %s RETURN the key\n", colorName);
		pthread_mutex_unlock ( &lock );
		// Wakeup consumer, if waiting
		pthread_cond_signal ( &ItemAvailable );
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
		while ( count == 0 ) {
			// Sleep using a cond variable
			while (  pthread_cond_wait( &ItemAvailable, &lock) != 0 ) {
				// printf("Consumer is waiting\n");
				// sleep(1);
				;
			}
				// printf("Consumer is waiting\n");
		}
		printf("consumer >>> GOT\n");
		
		// count must be > 0
		if  ( count > 0 ) {
			// Remove item form the buffer using "out"
			i = Buffer[out] ;
			out = (out + 1) % N;
			// Decrement the count of items in the buffer
			count--;
			// printf( "Removed %d \n", i);
		}
		// exit critical seciton
		printf("consumer >>> RETURN\n");
		pthread_mutex_unlock ( &lock);
		// Wakeup producer, if waiting
		pthread_cond_signal( &SpaceAvailable );
	} while ( i != -1 );
}

int main( int argc, char* argv[] )
{
	// how to use time of day
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	printf("Start running time (s + ms):  %ld\n", currentTime.tv_sec*1000000 + currentTime.tv_usec);
	printf("Start running time (seconds): %ld\n", currentTime.tv_sec);
	printf("Start running time (microseconds):      %ld\n", currentTime.tv_usec);
	
	// thread variables
	pthread_t prod1, prod2, cons;
	// attribute object
	pthread_attr_t attr;
	int n;

	pthread_mutex_init( &lock, NULL);
	pthread_cond_init( &SpaceAvailable, NULL);
	pthread_cond_init( &ItemAvailable, NULL);

	// Create 3 producer threads
	if ( n = pthread_create(&prod1, NULL, producer, 0) ) {
		fprintf(stderr,"pthread_create :%s\n",strerror(n));
		exit(1);
	}
	if ( n = pthread_create(&prod2, NULL, producer, 1) ) {
		fprintf(stderr,"pthread_create :%s\n",strerror(n));
		exit(1);
	}
	if ( n = pthread_create(&prod1, NULL, producer, 2) ) {
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
