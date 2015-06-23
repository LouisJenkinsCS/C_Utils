CC=gcc
CFLAGS=-c -g
LIBS=-pthread
OBJS=String_Utils.o String_Utils_Tests.o Misc_Utils.o
TARGET=String_Utils_Test
SUPATH=./String_Utils/Code/
SUTPATH = ./String_Utils/Tests/
MUPATH=./Misc_Utils/Code/
DEPS=-I$(SUPATH) -I$(SUTPATH) -I$(MUPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

Misc_Utils.o: $(MUPATH)Misc_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)Misc_Utils.c

String_Utils.o: $(SUPATH)String_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(SUPATH)String_Utils.c

String_Utils_Tests.o: $(SUTPATH)String_Utils_Tests.c
	$(CC) $(CFLAGS) $(DEPS) $(SUTPATH)String_Utils_Tests.c

clean: 
	$(RM) $(TARGET) *.o *~
