/***************************************************************************
 * Name:         Selma Leathem
 * Date:         2/23/2020
 * Description:  bashShell function, constant and global definitions.
 * 
 *               Emulates a bash shell with the following functionality:
 *
 *               Inbuilt cd, status and exit commands
 *
 *               The ability to put jobs in the background by putting & at 
 *               the end of a commandline.
 *
 *               Redirection <, > from/to input/output files.
 *
 *               Enforce foreground jobs exclusively using ^Z to enter and
 *               exit this mode.
 *
 *               Kill running jobs using ^C.
 *
 *               $$ expansion is supported.
 *
 *  The book The Linux Programming Interface by Kerrisk was heavily 
 *  referenced in the development of the following code as well as the course
 *  lecture examples.
 *  Chapters referenced include: chapters 20 - 27
 *              
 ****************************************************************************/ 

#include "pid_tDynArr.h"  /* manages and initializes dynamic arrays of pid_t type */
#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h> /* emulates the boolean type */
#include <string.h>
#include <signal.h>  /* for managing signals */
#include <unistd.h>  /* use for fork, read/write primatives etc... */
#include <sys/types.h>  /* includes pid_t type  */
#include <fcntl.h>  /* use for opening files to get descriptors */
#include <errno.h>  /* use to get error information */

/* maximum length of the command line including 1 char for '\0'*/
#define MAX_COMMAND_LINE 2048 + 1  

/* Given the above command line length the following is the resulting
 * maximum possible length due to $$ expansion including 1 char for '\0' .
 */
#define MAX_COMMAND_LINE_EXPANDED 5120 + 1

/* Maximum possible length of a directory name given the above limitations */ 
#define MAX_DIR_NAME    MAX_COMMAND_LINE_EXPANDED-3 

/* Maximum number of allowable commandline arguments */
#define MAX_NUM_ARGS 512

/* Maximum possible length of a filename */
#define MAX_FILE_NAME  MAX_DIR_NAME

/* Initial size of an array holding active background PIDs */
#define ARRAY_CAPACITY_BG 10  

/* Use to indicate an undefined variable */
#define UNDEFINED -5

/* Maximum number of digits including '\0' in an integer */
#define MAX_INT_LENGTH 11

/* Maximum length of an exit with, or terminated by message */
#define STATUS_LENGTH 60

/* Maximum length of an error message used in file opening */
#define ERROR_MESSAGE_MAX 80

/* define boolean true and false for use with an atomic data type */
#define TRUE 1  
#define FALSE 0

/* Used to indicate if the commandLine is too long and if not whether
 * or not the line's $$ were correctly expanded and placed into a 
 * larger array.
 */
enum expandStatus{ successful, commandLineTooLong};

/* Used by a signal handler and so declare to be volatile and atomic to 
 * avoid potential compiler optimizations and non-atomic reads and writes
 * This variable is TRUE when the user presses ^Z and sets the shell into
 * foreground mode. Otherwise it is false.
 * Reference: Office hours and The Linux Programming Inteface by Kerrisk
 * pg 428
 */
static volatile sig_atomic_t canPutInBackground = TRUE;

/* Takes as input the commandline, the pid that results from $$ and expands all
 * instances of $$ in the commandline then puts the result in the input char array
 * input: the commandline: char *
 *        the pid of bashShell: int
 *        a char array to put the expanded string in
 * output: expandStatus return indicates if the commandLine was too long and as
 *         to whether expansion was successful otherwise
 */      
enum expandStatus expandString(char *str, pid_t pid, char *strExpanded);

/* Converts an integer to a char array and puts the result in the input
 * char array. Note, this function assumes only positive numbers are
 * passed.
 * input: a char array to hold the string: char []
 *        the number to be converted: int
 */
void intToString(char str[],int number);

/* Takes the $$ expanded commandline and parses it into an args[] array to 
 * pass onto excvp and also gets the relevant stream data if <, >, & is found
 * in the commandline. In addition, if <, >, and/or & are found in the command
 * line appropriate flags are set.
 * input: the starting index in the args array: int
 *        the $$ expanded commandline: char[]
 *        the args array that is to be passed to execvp: char[]
 *        a boolean that will be set to true if < is found: bool &
 *        a boolean that will be set to true if > is found: bool &
 *        a boolean that will be set to true if & is found: bool &  
 *        a char array that will hold the name of an input file if < is found: char[]   
 *        a char array that will hold the name of an input file if > is found: char[]              
 */
void parseArguments(int *index, char bufferExpanded[], char *args[], bool *haveInput, bool *haveOutput,
                   bool *inBackground, char fileNameIn[], char fileNameOut[]);

/* Takes in the status returned from waitpid (childExitMethod) and runs it
 * through WIFSIGNALED, WTERMSIG, WIFEXITED and/or WEXITSTATUS to get either an
 * exit or termination status and puts the result in the string argument.
 * input: the status returned from waitpid: int
 *        a string to put the return status in: char[]
 */
void terminationStatus(int childExitMethod, char exitStatusStr[], int *terminationSignal);


/* Takes as input the name of a file from which the input stream stdin will
 * be diverted to, opens it, sets it to replace stdin and returns the file
 * descriptor obtained by opening the file for reading.
 * input: the file name: char[]
 * output: the file descriptor: int
 */
int setInputStream(char *fileNameIn);

/* Takes as input the name of a file from which the output stream stdout will
 * be diverted to, opens it, sets it to replace stdout and returns the file
 * descriptor obtained by opening the file for writing.
 * input: the file name: char[]
 * output: the file descriptor: int
 */
int setOutputStream(char *fileNameOut);

/* Initialize each element of the args array used to hold the commandline
 * arguments to pass to execvp. Each element is initialized to NULL.
 * input: the args array: char *[]
 *        the size of the args array
 */
void initializeArgs(char *ptrArray[], int size);

/* Creates a dynamic char array for an element of the args array to point to.
 * The array has size elements.
 * input: the size of the dynamic array
 * output: a one-dimensional dynamic array
 */
char *getEmptyString(int size);

/* Frees a dynamic char string and sets its pointer value to NULL.
 * input: the dynamic char array: char *
 */
void freeString(char *);  

