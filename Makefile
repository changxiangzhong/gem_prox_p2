CC=gcc
CFLAGS=-I. -g -Wall 
DEPS=tlp224.h
OBJ=tlp224.o main.o serial.o
EXECUTABLE=lock

dist: $(OBJ)
	$(CC) -o $(EXECUTABLE) $(OBJ)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<  $(CFLAGS)
clean:
	rm -rf *.o  a.out $(EXECUTABLE)

val: dist
	valgrind ./$(EXECUTABLE)
