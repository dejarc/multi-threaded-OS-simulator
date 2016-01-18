CC=gcc
CFLAGS=-I.
DEPS = eventQueue.h
OBJ = eventQueue.o finalTest.o
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
finalTest.exe: $(OBJ)
	gcc -pthread -o $@ $^ $(CFLAGS) 
