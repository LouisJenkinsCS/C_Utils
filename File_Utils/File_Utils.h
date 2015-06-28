#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>
#include <stdlib.h>

/*
 * @Author: Louis Jenkins
 * @Brief: File Utilities for the C Programming Language
 * 
 * File Utilities for the C Programming Language, or as it's going to be commonly
 * referred to as, File_Utils, will essentially be my attempt at implementing a
 * somewhat lightweight and robust library for file reading, writing and manipulations.
 * 
 * File_Utils begins by opening the file pointed to by a file pointer or a filename
 * passed, in read only mode. This is because it never actually writes directly to the
 * file unless specified, as it reads the entire file, line by line to a buffer.
 * The buffer, or rather the array of buffers, stores every line, meaning it must
 * contain a newline character to properly be considered a line, from the file to
 * be retrieved and operated on later. This sacrifices memory size for safety, as
 * any file that can be read by the user will be able to be "modified".
 * 
 * What File_Utils does with it's copy of the original is that it allows you to
 * modify the copy without impacting the original file. And due to the structure
 * of everything, it allows you modify a line easily, append new information, 
 * even deleting entire lines from the copy of the file. Another thing that it does
 * is that it allows the user to search the entire file for a substring. It even
 * allows line-searching, which is notably important for parsing information.
 * 
 * When finished with File_Utils, you can flush the copy's contents on either the
 * same file, or to another file, as a safe copy. Maybe in the future, File_Utils
 * will allow you to view the file via a GUI text-viewer, but for now, it can print
 * every line to the terminal to validate that everything went according to plan.
 */

/// Normal operations.
#define NONE 1 << 0
/// Sets the current character to the beginning for this operation.
#define BEGINNING 1 << 1
/// Sets the current character to the end for this operation.
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

/**
 * Opens the file to be stored in File_Info, and initializes then returns the File_Info instance.
 * @param filename Name of the file
 * @param mode Mode to be opened in (I.E Read/Write)
 * @return Newly instantiated File_Info struct.
 */
File_Info *File_Utils_Open_File(const char *filename, const char *mode);

/**
 * Takes an already initialized file descriptor and uses it to initialize a File_Info struct.
 * @param file File already opened and to be read from.
 * @return Newly instantiated File_Info struct.
 */
File_Info *File_Utils_To_File_Info(FILE *file);

/**
 * Reads the next character on the current line. Doing so will also forward the
 * current character by one.
 * @param info The File_Info instance to be read.
 * @return The character.
 */
const char File_Utils_Read_Char(File_Info *info);

/**
 * Reads the line that it is currently on. If FROM parameter is passed, it reads
 * from the current character, else it will just return the whole line. 
 * @param info Instance of File_Info to read from.
 * @param parameter NONE | FROM
 * @return String representing the line to be read.
 */
const char *File_Utils_Read_Line(File_Info *info, int parameter);

/**
 * Reads up to the next delimiter specified on the current line. 
 * This function is good for parsing information from files, I.E Comma Separated 
 * files like Excel Spreadsheets or general text files with fields separated by
 * delimiters. FROM parameter reads from the current character, and will prevent repeatedly
 * reading the same portion.
 * @param info Instance of File_Info.
 * @param delimiter Delimiter to read up to.
 * @param parameter NONE | FROM.
 * @return String before the delimiter found, or the entire string if none.
 */
const char *File_Utils_Read_To_Delimiter(File_Info *info, const char *delimiter, int parameter);

/**
 * Advances the File_Info to the next line.
 * @param info This instance.
 * @return The string that is the next line or NULL if there is none.
 */
const char *File_Utils_Next_Line(File_Info *info);

/**
 * Goes back one line.
 * @param info This instance.
 * @return The string that is the next line, or NULL if there is none.
 */
const char *File_Utils_Previous_Line(File_Info *info);

/**
 * Sets the current line to the user-requested one.
 * @param info This instance.
 * @param line_number Line number to go to.
 * @return The string at that line, or NULL if out of bounds.
 */
const char *File_Utils_Goto_Line(File_Info *info, unsigned int line_number);

/**
 * Deletes the current line, safely. Should note that deleting it some other way
 * causes undefined behavior.
 * @param info This instance.
 * @return The next or previous line.
 */
const char *File_Utils_Delete_Line(File_Info *info);

/**
 * Deletes the text from the line from beginning to end.
 * @param info This instance.
 * @param start Where it will begin to delete from.
 * @param end Where it will end.
 * @return Newly modified string.
 */
const char *File_Utils_Delete_From_Line(File_Info *info, unsigned int start, unsigned int end);

/**
 * Searches all lines to find the first occurrence of substring. Should note:
 * this will advance the current line to the line the substring is found and the
 * current character where the substring begins. If none is found, then no change
 * to current line or character is made. If FROM parameter is chosen, then it will
 * search from the current line and character, else it will start from the beginning.
 * @param info This instance.
 * @param substring Substring to find.
 * @param parameter NONE | FROM
 * @return The line of the first occurrence the substring is found.
 */
const char *File_Utils_Seek(File_Info *info, const char *substring, int parameter);

/**
 * Counts the occurrences of the substring in the file. If FROM is selected, then
 * it will search from the current character in the current line onward, else from
 * the start.
 * @param info This instance.
 * @param substring Substring that the occurrence of is being counted.
 * @param parameter NONE | FROM.
 * @return 
 */
int File_Utils_Count(File_Info *info, const char *substring, int parameter);

/**
 * Reads the entirety of the file. This function will in fact return one
 * massive string joined together, which can be rather inefficient, but it is an
 * option. If FROM is selected, it will read everything from the current character
 * in the current line.
 * @param info This instance.
 * @param parameter NONE | FROM
 * @return Massive string with all newline characters intact.
 */
char *File_Utils_Read_All(File_Info *info, int parameter);

/**
 * Appends the character to the current character, safely. Where it will be appended
 * can be changed to either the BEGINNING or the END of the string.
 * @param info This instance
 * @param character Character to be appended.
 * @param parameter NONE | BEGINNING | END
 * @return Newly modified string.
 */
const char *File_Utils_Append_Char(File_Info *info, char character, int parameter);

/**
 * Appends a string to the current line. Depending on parameter passed, where
 * it will be appended varies from BEGINNING or END, if NONE then appended after
 * the current character.
 * @param info This instance.
 * @param string String to be appended.
 * @param parameter NON | BEGINNING | END
 * @return 1 if success
 */
const char *File_Utils_Append_String(File_Info *info, char *string, int parameter);

/**
 * Prepends the string to the beginning of the file, safely.
 * @param info This instance.
 * @param string String to be prepended.
 * @return The string that was prepended.
 */
const char *File_Utils_Prepend_To_Beginning(File_Info *info, const char *string);

/**
 * Appends the given string to the end of the file.
 * @param info This instance.
 * @param string String to be appended.
 * @return The string appended.
 */
const char *File_Utils_Append_To_End(File_Info *info, const char *string);

/**
 * Sets the current line to a string passed to it.
 * @param info This instance.
 * @param string String to replace old string.
 * @return The current line which was replaced.
 */
const char *File_Utils_Set_Line(File_Info *info, const char *string);

/**
 * Writes the contents to a file. Can be the same file by specifying it as file again.
 * Alternatively, you can save the copy of the file to another file, good for making
 * copies of read-only documents.
 * @param info This instance.
 * @param file File to be written to, can be the same file copied.
 * @return 1 if successful, 0 if failure.
 */
int File_Utils_Write_To_File(File_Info *info, FILE *file);

/**
 * Deconstructor for File_Info. Will free the array of characters, including any
 * modified with Set_Line, so no string literals should be passed.
 * @param info This instance.
 * @return 1 on success, 0 on failure.
 */
int File_Utils_Destroy(File_Info *info);

#endif /* end FILE_UTILS_H */
