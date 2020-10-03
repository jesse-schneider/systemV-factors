SOBJECTS = server.o
COBJECTS = client.o
OBJECTS = $(SOBJECTS) $(COBJECTS)
all: $(OBJECTS)
	$(CC) $(SOBJECTS) -lm -lpthread -o server
	$(CC) $(COBJECTS) -lpthread -o client
server.o: server.c
	$(CC) -c server.c
client.o: client.c
	$(CC) -c client.c

