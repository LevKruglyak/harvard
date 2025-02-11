CC=gcc -std=c99
CFLAGS = -ggdb3 -W -Wall -Wextra -O3
LDFLAGS =
LIBS = 

HASHMAP = robinhood_hash_table.o chained_hash_table.o pool_alloc.o utils.o string_hashtable.o

default: main test continuous_tester benchmark

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(HASHMAP) main.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

test: $(HASHMAP) test.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

continuous_tester: $(HASHMAP) continuous_tester.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

benchmark: $(HASHMAP) benchmark.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f main test benchmark *.o
