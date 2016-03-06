CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=logger.c argument_check.c alloc_check.c priority_queue_test.c priority_queue.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=priority_queue_test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./misc/ ./io/ ./data_structures/ ./data_structures/Tests

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

