CC=gcc
CFLAGS=-c -g
LIBS=-pthread
OBJS=Linked_List.o Linked_List_Test.o Misc_Utils.o
TARGET=Linked_List_Test
LLPATH=./Data_Structures/Linked_List/Code/
LLTPATH=./Data_Structures/Linked_List/Tests/
MUPATH=./Misc_Utils/Code/
DEPS=-I$(MUPATH) -I$(LLPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

Misc_Utils.o: $(MUPATH)Misc_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)Misc_Utils.c

Linked_List.o: $(LLPATH)Linked_List.c
	$(CC) $(CFLAGS) $(DEPS) $(LLPATH)Linked_List.c

Linked_List_Test.o: $(LLTPATH)Linked_List_Test.c
	$(CC) $(CFLAGS) $(DEPS) $(LLTPATH)Linked_List_Test.c

clean: 
	$(RM) $(TARGET) *.o *~
