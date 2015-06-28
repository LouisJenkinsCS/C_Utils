CC=gcc
CFLAGS=-c -g
LIBS=-pthread
OBJS=PBQueue.o Misc_Utils.o Thread_Pool.o Thread_Pool_Test.o
TARGET=Thread_Pool_Test
PBQPATH=./Data_Structures/PBQueue/Code/
TPPATH=./Thread_Pool/Code/
TPTPATH=./Thread_Pool/Test/
MUPATH=./Misc_Utils/Code/
DEPS=-I$(PBQPATH) -I$(MUPATH) -I$(TPPATH) -I$(TPTPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS)

PBQueue.o: $(PBQPATH)PBQueue.c
	$(CC) $(CFLAGS) $(DEPS) $(PBQPATH)PBQueue.c

Misc_Utils.o: $(MUPATH)Misc_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)Misc_Utils.c

Thread_Pool.o: $(TPPATH)Thread_Pool.c
	$(CC) $(CFLAGS) $(DEPS) $(TPPATH)Thread_Pool.c

Thread_Pool_Test.o: $(TPTPATH)Thread_Pool_Test.c
	$(CC) $(CFLAGS) $(DEPS) $(TPTPATH)Thread_Pool_Test.c

clean: 
	$(RM) $(TARGET) *.o *~
