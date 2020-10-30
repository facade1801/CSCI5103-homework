all : main consumer producer

main : main.c
	gcc -o main main.c

consumer : consumer.c
	gcc -o consumer consumer.c

producer : producer.c
	gcc -o producer producer.c

clean	:
	rm -f main producer consumer *.o *% a.out core