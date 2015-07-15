CC=gcc
CFLAGS=-c -g -D_GNU_SOURCE -Wall
LIBS=-pthread
OBJS=NU_Connection.o NU_Server.o NU_Helper.o NU_Server_Test_File_Downloader.o MU_Logger.o MU_Cond_Locks.o
TARGET=NU_Server_Test_File_Downloader
NUPATH=./Net_Utils/
MUPATH=./Misc_Utils/
DEPS=-I$(MUPATH) -I$(NUPATH)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

MU_Logger.o: $(MUPATH)MU_Logger.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Logger.c

NU_Connection.o: $(NUPATH)NU_Connection.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Connection.c

NU_Helper.o: $(NUPATH)NU_Helper.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Helper.c
	
MU_Cond_Locks.o: $(MUPATH)MU_Cond_Locks.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Cond_Locks.c
	
NU_Server.o: $(NUPATH)NU_Server.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Server.c

NU_Server_Test_File_Downloader.o: $(NUPATH)/Tests/NU_Server_Test_File_Downloader.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)/Tests/NU_Server_Test_File_Downloader.c

clean: 
	$(RM) $(TARGET) *.o *~
