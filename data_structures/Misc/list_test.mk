CC=clang
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=list.c list_test.c logger.c scoped_lock.c alloc_check.c iterator.c string_buffer.c argument_check.c ref_count.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=list_test
VPATH=./misc/ ./data_structures/ ./data_structures/tests ./io/ ./threading/ ./string/ ./memory/

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

