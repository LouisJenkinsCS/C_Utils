CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall -std=c11
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=DS_Stack.c DS_List.c DS_Helpers.c DS_Iterator.c MU_Logger.c MU_Arg_Check.c MU_Hazard_Pointers.c DS_Stack_Test.c TP_Pool.c DS_PBQueue.c MU_Events.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=DS_Stack_Test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./Misc_Utils/ ./Misc_Utils/ ./Data_Structures/ ./Data_Structures/Tests ./Thread_Pool/

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

