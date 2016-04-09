CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=heap.c heap_test.c scoped_lock.c logger.c ref_count.c alloc_check.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=heap_test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./misc/ ./data_structures/ ./data_structures/tests ./threading/ ./io/

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(DEPS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPS) -c $< -o $@

.PHONY: depend clean

depend: $(SOURCES)
	makedepend $(DEPS) $^

clean: 
	$(RM) $(TARGET) *.o *~

# DO NOT DELETE THIS LINE -- make depend depends on it.

