CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=DS_List.c MU_Event_Loop.c MU_Logger.c MU_Arg_Check.c MU_Event_Loop_Test.c MU_Events.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=MU_Event_Loop_Test
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./Misc_Utils/ ./Misc_Utils/Tests ./Data_Structures/

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

