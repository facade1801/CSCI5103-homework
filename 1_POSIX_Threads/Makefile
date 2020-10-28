CFLAGS	= -Xa -v -D_REENTRANT
LIBS	= -lpthread

all	: prodcons

clean	:
	rm -f prodcons *.o *% a.out core

hello	:
	$(CC) $(CFLAGS) -o prodcons prodcons.c $(LIBS)