CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=logger.c argument_check.c scoped_lock.c alloc_check.c map.c map_test.c string_buffer.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=map_test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./misc/ ./io/  ./data_structures/ ./data_structures/tests ./threading/ ./string/

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

