CC = gcc

all: TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender

TCP: TCP_Receiver TCP_Sender

RUDP: RUDP_Receiver RUDP_Sender 

RUDP_Sender: RUDP_Sender.c RUDP_API.c RUDP.API.h 
	$(CC) RUDP_Sender.c RUDP_API.c -o RUDP_Sender 

RUDP_Receiver: RUDP_Receiver.c RUDP_API.c RUDP.API.h 
	$(CC) RUDP_Receiver.c RUDP_API.c -o RUDP_Receiver
	

TCP_Receiver: TCP_Receiver.o
	$(CC) -o TCP_Receiver TCP_Receiver.o

TCP_Sender : TCP_Sender.o
	$(CC) -o TCP_Sender TCP_Sender.o

TCP_Receiver.o : TCP_Receiver.c
	$(CC) -c TCP_Receiver.c

TCP_Sender.o : TCP_Sender.c
	$(CC) -c TCP_Sender.c

.PHONY: clean all

clean:
	rm -f *.o TCP_Receiver TCP_Sender random_data.txt RUDP_Receiver RUDP_Sender