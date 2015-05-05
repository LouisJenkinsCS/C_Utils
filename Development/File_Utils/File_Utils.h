#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>
#include <stdlib.h>

/// Normal operations.
#define NONE 1 << 0
/// Reads from the current index in the line.
#define FROM 1 << 1
/// Used to append to end of the line or at the current index.
#define END 1 << 2

typedef struct File_Info File_Info;

/// Struct that contains the general information about the file to keep track.
struct File_Info{
	/// The file descriptor.
	FILE *file;
	/// The entire file as array of strings.
	char **file_contents;
	/// The line number.
	unsigned int line_number;
	/// The index in the current line.
	unsigned int current_char;
};

/// Opens the file, assigns it to a new struct along with it's contents and return the struct.
File_Info *File_Utils_Open_File(const char *filename, const char *mode);

/// Converts the FILE pointer to a file_info struct.
File_Info *File_Utils_To_File_Info(FILE *file);

/// Reads one char in the current line, advancing to the next.
char File_Utils_Read_Char(File_Info *info);

/// Reads the whole line, going to the next one.
char *File_Utils_Read_Line(File_Info *info, int parameter);

/// Reads the entire file, resetting 
char *File_Utils_Read_All(File_Info *info, int parameter);

/// Appends a character at the given line at the current index.
int File_Utils_Append_Char(File_Info *info, char character, int parameter);

/// Append a string to the current line at the current index.
int File_Utils_Append_String(File_Info *info, char *string, int parameter);

/// Append to the end of the document
int File_Utils_Append_To_End(File_Info *info, char *string);

/// Writes current contents to file. Note: Can overrite current file.
int File_Utils_Write_To_File(File_Info *info, FILE *file);

/// Deconstructor which will close the file and free any reserved memory
int File_Utils_Destroy(File_Info *info);

#endif /* end FILE_UTILS_H */
