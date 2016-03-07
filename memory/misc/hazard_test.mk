CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=list.c logger.c iterator.c alloc_check.c scoped_lock.c argument_check.c hazard.c hazard_test.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=hazard_test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./memory/ ./io/ ./misc/ ./memory/tests ./data_structures/ ./threading/

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

