CC=gcc
CFLAGS=-c -g
LIBS=-pthread
OBJS=PBQueue.o PBQueue_Test.o Misc_Utils.o
TARGET=PBQueue_Test
PBQPATH=./Data_Structures/PBQueue/Code/
PBQTPATH=./Data_Structures/PBQueue/Test/
MUPATH=./Misc_Utils/Code/
DEPS=-I$(MUPATH) -I$(PBQPATH) -I$(PBQTPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

Misc_Utils.o: $(MUPATH)Misc_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)Misc_Utils.c

PBQueue.o: $(PBQPATH)PBQueue.c
	$(CC) $(CFLAGS) $(DEPS) $(PBQPATH)PBQueue.c

PBQueue_Test.o: $(PBQTPATH)PBQueue_Test.c
	$(CC) $(CFLAGS) $(DEPS) $(PBQTPATH)PBQueue_Test.c

clean: 
	$(RM) $(TARGET) *.o *~
