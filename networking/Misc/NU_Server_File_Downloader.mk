CC=gcc
PRESENT_DIRECTORY = $(filter %/, $(wildcard ./*/))
CFLAGS=-g -D_GNU_SOURCE -Wall
LDFLAGS=-pthread
FLAGS=$(CFLAGS) $(LDFLAGS)
SOURCES=MU_Logger.c MU_Arg_Check.c NU_Server.c NU_Connection.c NU_Server_Test_File_Downloader.c TP_Pool.c MU_Events.c DS_PBQueue.c NU_HTTP.c DS_Hash_Map.c
OBJECTS=$(notdir $(SOURCES:.c=.o))
TARGET=NU_Server_File_Downloader
DEPS=$(addprefix -I, $(PRESENT_DIRECTORY))
VPATH=./Misc_Utils/ ./Net_Utils/ ./Net_Utils/Tests ./Thread_Pool/ ./Data_Structures/

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

