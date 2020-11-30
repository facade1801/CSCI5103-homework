CLFAGS = -Xa -v -D_REENTRANT
LIBS	= -lpthread

all	: prodcons

prodcons: prodcons.c
	gcc -pthread -o prodcons prodcons.c

clean	:
	rm -f prodcons *.o *% a.out core

hello	:
	$(CC) $(CFLAGS) -o prodcons prodcons.c $(LIBS)
