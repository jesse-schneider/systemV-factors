SOBJECTS = server.o
COBJECTS = client.o
TOBJECTS = factors.o
OBJECTS = $(SOBJECTS) $(COBJECTS) $(TOBJECTS)
all: $(OBJECTS)
	$(CC) $(SOBJECTS) -lm -lpthread -o server
	$(CC) $(COBJECTS) -lpthread -o client
	$(CC) $(TOBJECTS) -lpthread -lm -o factors
server.o: server.c
	$(CC) -c server.c
client.o: client.c
	$(CC) -c client.c
factors.o: factors.c
	$(CC) -c factors.c 

fact:
	./factors 4294967285

shm:
	./client 4294967285
