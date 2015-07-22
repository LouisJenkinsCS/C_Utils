CC=gcc
CFLAGS=-c -g -D_GNU_SOURCE -Wall
LIBS=-pthread
OBJS=MU_Cond_Locks.o MU_Arg_Check.o DS_Hash_Map.o DS_Hash_Map_Test.o MU_Logger.o
TARGET=DS_Hash_Map_Test
DSPATH=./Data_Structures/
MUPATH=./Misc_Utils/
DEPS=-I$(MUPATH) -I$(DSPATH)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

MU_Logger.o: $(MUPATH)MU_Logger.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Logger.c

MU_Cond_Locks.o: $(MUPATH)MU_Cond_Locks.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Cond_Locks.c

MU_Arg_Check.o: $(MUPATH)MU_Arg_Check.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Arg_Check.c

DS_Hash_Map.o: $(DSPATH)DS_Hash_Map.c
	$(CC) $(CFLAGS) $(DEPS) $(DSPATH)DS_Hash_Map.c

DS_Hash_Map_Test.o: $(DSPATH)/Tests/DS_Hash_Map_Test.c
	$(CC) $(CFLAGS) $(DEPS) $(DSPATH)DS_Hash_Map_Test.c

clean: 
	$(RM) $(TARGET) *.o *~
