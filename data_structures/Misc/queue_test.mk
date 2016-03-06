CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=queue.c queue_test.c list.c iterator.c logger.c argument_check.c alloc_check.c scoped_lock.c hazard.c thread_pool.c priority_queue.c events.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=queue_test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./misc/ ./memory/ ./data_structures/ ./data_structures/tests ./threading/ ./io/

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

