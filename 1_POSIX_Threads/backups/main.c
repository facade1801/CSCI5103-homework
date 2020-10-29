#include <string.h>
#include <stdio.h>
#include<sys/time.h>

char ** buffer = NULL;
int bufferSize;

void returnTime(char* strTime) {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    long long llTime = currentTime.tv_sec*1000000 + currentTime.tv_usec;
    sprintf(strTime, "%lld", llTime);
}

int main( int argc, char* argv[]) {
	char time[40];
	returnTime(time);
	printf("returned time is %s\n", time);

}