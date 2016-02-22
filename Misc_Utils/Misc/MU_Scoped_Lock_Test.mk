CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=MU_Logger.c MU_Arg_Check.c MU_Scoped_Lock_Test.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=MU_Scoped_Lock_Test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./Misc_Utils/ ./Misc_Utils/Tests

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

