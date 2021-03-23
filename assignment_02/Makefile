CC = gcc
CFLAGS = -lpthread

RESULT = ssu_shell
TTOP = ttop
PPS = pps

.PHONY: all
all : $(RESULT) $(TTOP) $(PPS)

$(RESULT) : ssu_shell.o
	$(CC) -c ssu_shell.c -o ssu_shell.o
	$(CC) -o $(RESULT) ssu_shell.o

$(TTOP) : ttop.o getdata.o
	$(CC) -o $(TTOP) ttop.o getdata.o $(CFLAGS)

$(PPS) : pps.o getdata.o
	$(CC) -o $(PPS) pps.o getdata.o

getdata.o : getdata.c
	$(CC) -c getdata.c -o getdata.o

ttop.o: ttop.c
	$(CC) -c ttop.c -o ttop.o

pps.o: pps.c
	$(CC) -c pps.c -o pps.o

.PHONY: clean
clean:
	rm -rf *.o
