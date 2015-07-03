CC=gcc
CFLAGS=-c -g -D_GNU_SOURCE
LIBS=-pthread
OBJS=NU_Client.o NU_Helper.o NU_Client_Test_File_Downloader.o MU_Logger.o
TARGET=NU_Client_Test_File_Downloader
NUPATH=./Net_Utils/
MUPATH=./Misc_Utils/
DEPS=-I$(MUPATH) -I$(NUPATH)


$(TARGET): $(OBJS)
	$(CC) $(LIBS) $(OBJS) -o $(TARGET)

MU_Logger.o: $(MUPATH)MU_Logger.c
	$(CC) $(CFLAGS) $(DEPS) $(MUPATH)MU_Logger.c

NU_Helper.o: $(NUPATH)NU_Helper.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Helper.c

NU_Client.o: $(NUPATH)NU_Client.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)NU_Client.c

NU_Client_Test_File_Downloader.o: $(NUPATH)/Tests/NU_Client_Test_File_Downloader.c
	$(CC) $(CFLAGS) $(DEPS) $(NUPATH)/Tests/NU_Client_Test_File_Downloader.c

clean: 
	$(RM) $(TARGET) *.o *~
