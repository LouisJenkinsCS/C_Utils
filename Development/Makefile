CC=gcc
CFLAGS=-c -g -D_GNU_SOURCE
LIBS=-pthread
OBJS=NU_Server.o NU_Helper.o  NU_Server_Test_Client_Chat.o Misc_Utils.o
TARGET=NU_Server_Test
NUPATH=./Net_Utils/
MUPATH=./Misc_Utils/Code/
DEPS=-I$(MUPATH) -I$(NUPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

Misc_Utils.o: $(MUPATH)Misc_Utils.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)Misc_Utils.c

NU_Helper.o: $(NUPATH)NU_Helper.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Helper.c

NU_Server.o: $(NUPATH)NU_Server.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Server.c

NU_Server_Test_Client_Chat.o: $(NUPATH)NU_Server_Test_Client_Chat.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Server_Test_Client_Chat.c

clean: 
	$(RM) $(TARGET) *.o *~
