/***************************************************************************
 * Name:         Selma Leathem
 * Date:         2/23/2020
 * Description:  bashShell implementation.
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

#include "bashShell.h"

/* Signal catcher for SIGTSTP. Sets a global atomic variable and prints a message
 * when activated.
 */
static void catchSIGTSTP(int theSignal)
{
    char* messageEnter = "Entering foreground-only mode (& is now ignored)\n";
    char* messageLeave = "Exiting foreground-only mode\n";
    char* prompt = ": ";

    /* If the process is not in foreground only mode then put it in foreground
    * only mode by setting canPutInBackground to FALSE and printing messageEnter.
     */
    if (canPutInBackground == TRUE)  
    {
        write(STDOUT_FILENO, messageEnter, 49);
        write(STDOUT_FILENO, prompt, 2);
        canPutInBackground = FALSE;
    }
    /* If the process is in foreground only mode then take it out of foreground
     * only mode by setting canPutInBackground to TRUE and printing messageLeave.
     */
    else if (canPutInBackground == FALSE)
    {
        write(STDOUT_FILENO, messageLeave, 29);
        canPutInBackground = TRUE;
        write(STDOUT_FILENO, prompt, 2); 
    }  
    
}

int main(int argc, char *argv[])
{
    char *buffer;  /* used to hold the string returned by getLine */

    char bufferExpanded[MAX_COMMAND_LINE];

    size_t bufferSize;  /* set to 0 so getLine will read an arbitrary length */

    ssize_t lineSize; /* length of the line returned by getLine */

    /* resource: https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm*/
    char *token; /* Use to hold tokens extracted from the commandline with strtok*/

    /* Holds the commandline arguments that are passed to execvp */
    char *args[MAX_NUM_ARGS]; 

    /* the size of the string pointed to by an element of the args pointer array */
    int argSize; 

    int i, index=0; /* used for array element access */

    /* holds the status after trying to expand $$ in the commandline string */
    enum expandStatus expandstatus; 

    bool haveInput = false; /* true if user enters < */
    bool haveOutput = false; /* true if user enters > */
    bool inBackground = false; /* true if process is in the background */

    /* used to indicate if an arguement can't be run in the background */
    bool noBackground = false;  

    int execvpReturn; /* return value from calling execvp() */

    int terminationSignal= 0; /* the value is 0 or that of a sent signal */

    /* Arg to hold the termination/exit signal for a background process */
    int terminationSignalBg= 0;  

    /* use to get process exit and termination information after using the waitpid() function */
    int childExitMethod = -5; 

    /* stores process exit and termination information that is printed to stdout */
    char exitStatusStr[STATUS_LENGTH]; 

    /* For a background process: stores process exit and termination information that 
     *is printed to stdout 
     */
    char exitStatusStrBg[STATUS_LENGTH];
    
    pid_t spawnPid = -5; /* process number returned by calling fork() */
    pid_t parentPID; /* process number of the bashShell program */
    pid_t foregroundPID; /* process number of the foreground process */

    pid_t waitPidReturn; /* the return value from calling waitpid() */

    pid_t *pidReturn; /* A dynamic array holding all active background process PIDs */

    int pidReturnCapacity = ARRAY_CAPACITY_BG; /* size of the pidReturn array */

    int numProcesses = 0; /* the number of currently active background processes*/
    
    /* the file name of the input file from which input is to be redirected from */
    char fileNameIn[MAX_FILE_NAME]; 

    /* The file descriptor returned from opening a file for reading */
    int inFileDescriptor; 

     /* The file name of the output file to which output is to be redirected to */
    char fileNameOut[MAX_FILE_NAME];

    /* The file descriptor returned from opening a file for writing */
    int outFileDescriptor; 

    /* An absolute or relative path to a directory to move into with the cd command */
    char cdDirName[MAX_DIR_NAME]; 

    /* Use below to temporarily delay the TSTP signal until the foreground process has completed.
    * Reference: The Linux Programming Interface by Kerrisk pg 410-411
    * prevMask holds the previous mask and blockset is defined to block SIGTSTP
    */
    sigset_t blockSet, prevMask;

    /* Declare sigaction struct variables for handling the indicated signal
     * Note that ignore_action variable is used for ignoring the signal
     * that uses it.
     */
    struct sigaction SIGINTaction = {0};
    struct sigaction ignore_action = {0};
    struct sigaction SIGTSTPaction = {0};
    struct sigaction default_action = {0};

    /*********** for SIGINT**********/
    /* set ignore_action signal handler to ignore SIG_IGN */
    ignore_action.sa_handler = SIG_IGN;
    /* specify the action to be taken with a SIGINT signal */
    sigaction(SIGINT, &ignore_action, NULL);  

     
     /*********** for SIGTSTP**********/
    /* set the SIGTSTP signal handler to  catchSIGTSTP */ 
    SIGTSTPaction.sa_handler = catchSIGTSTP;
    /*Ensure if the signal delivered during an open, read, or write
    * operation that these can restart rather than return a failure
    */
    SIGTSTPaction.sa_flags = SA_RESTART;
    /* set to block all signals while the signal handler is executing */
    sigfillset(&SIGTSTPaction.sa_mask);
    /* specify the action to be taken with a SIGTSTP signal */
    sigaction(SIGTSTP, &SIGTSTPaction, NULL);


    /**************** initialize program variables ***************/
    
    /* Create the pidReturn dynamic array */
    pidReturn = getPidReturnArray(pidReturnCapacity);

    /* initialize the pidReturn array */
    initializeBgPidArray(pidReturn, pidReturnCapacity);

    /* If status is run before any foreground command is run then it should
     * return exit status 0
     */
    memset(exitStatusStr,'\0', STATUS_LENGTH);
    sprintf(exitStatusStr,"exit value %d \n",0);

    /* https://stackoverflow.com/questions/2595503/determine-pid-of-terminated-process */
    parentPID = getpid();


    /***** The shell itself.******
    * When the user exits the shell this loop is broken out of.
    * The reference for the general structure of the code for the shell is:
    * The Linux Programming Interface by Kerrisk, Chapter 27 and pg 581
    */
    while(true)
    {

            /* With the -1 argument to waitpid this means wait for any child as opposed
             * to one with a specific PID. When a background process finishes this
             * while loop will catch it and store the child PID in the return value
             * waitPidReturn. Resources:
             * The Linux Programming Interface by Kerrisk pg 556
             * https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
             * https://stackoverflow.com/questions/2595503/determine-pid-of-terminated-process
             */
            while ( (waitPidReturn = waitpid(-1, &childExitMethod, WNOHANG)) > 0 )
            { 
                /* If the returned value of waitpid is not the last run foreground process */
                if (waitPidReturn != foregroundPID)
                {
                    /* Get the termination or exit status and put in the char * exitStatusStrBg */
                    terminationStatus(childExitMethod, exitStatusStrBg, &terminationSignalBg);
                    /* Print the exit or termination status of the background process */
                    printf("background pid %d is done: %s\n", waitPidReturn, exitStatusStrBg);
                    --numProcesses;
                    /* Reset the array element that held the background PID in the pidReturn
                     * array to BG_UNDEFINED
                     */
                    resetPidArrayElement(pidReturn, waitPidReturn, pidReturnCapacity);
                } 
                
            }

        /* Re-initialize the following variables after the previous commandline has been
        * executed.
        */
        fflush(stdout); /* flush the output stream */
        printf(": ");  /* print the commandline prompt */
        haveInput = false;
        haveOutput = false;
        inBackground = false;
        lineSize = UNDEFINED;

        /* Initialize the args array which holds the commandline arguments*/
        initializeArgs(args, MAX_NUM_ARGS);

        /* Read in the commandline using getline and store in the variable buffer */
        /*resource:  http://man7.org/linux/man-pages/man3/getline.3.html */

        buffer = NULL;      /* Must be set to NULL for arbitrary line length */
        bufferSize = 0;     /* Must be set to 0 for arbitrary line length */

        lineSize = getline(&buffer, &bufferSize, stdin);

        if (lineSize < 0 )  /* Catches lineSize is UNDEFINED and getline return -1 */
        {
           /* https://www.tutorialspoint.com/c_standard_library/c_function_clearerr.htm */
            clearerr(stdin);       
        }   
        else{

            /* Expand occurences of $$ in the commandline string, buffer */ 
            expandstatus = expandString(buffer, parentPID, bufferExpanded);

            /* If the commandline exceeds the maximum allowed length print a message and 
             * bring the user back to the prompt.
             */
            if (expandstatus == commandLineTooLong )
            {
                printf("The command line has a maximum length of %d\n",MAX_COMMAND_LINE );
                
            }
            /* If the commandline was successfully expanded then continue on to execute it */
            else if (expandstatus == successful)
            {

                /*If the commandline is "exit" then clean up and exit from the program */
                if (strncmp(bufferExpanded,"exit", strlen("exit"))== 0)
                {
                    /* free the buffer */
                    if (buffer != NULL)
                    {
                        free(buffer); 
                    }

                    /* kill any remaining background jobs */
                    for (i =0; i < pidReturnCapacity; i++)
                    {
                        /* Background job PIDs are stored in pidReturn, and when a job 
                         * returns that element in the pidReturn array is set to 
                         * BG_UNDEFINED which is an arbitrary negative number, so if
                         * an element of pidReturn is > BG_UNDEFINED then there is a
                         * background process with that pid running.
                         */
                        if (pidReturn[i] > BG_UNDEFINED )
                        {
                            /* send the SIGTERM signal to kill the active process */
                            kill(pidReturn[i], SIGTERM);  
                        }

                    } 

                    break;  /* break out of the while loop to exit the shell */
                    
                }
                /* If the user enters a comment(starts with #) or nothing then do nothing */
                else if (strncmp(bufferExpanded,"#", strlen("#"))== 0 || lineSize == 1)
                {
                    /* do nothing */
                }
                /* If the user enters "cd" then change to the indicated directory or HOME*/
                else if (strncmp(bufferExpanded,"cd",strlen("cd")) == 0 )
                {
                   
                    /* Get the next argument on the commandline by splitting it into
                     * space delimited tokens.
                     */

                     /* First, ignore the word "cd" */
                    token = strtok(bufferExpanded," \n"); 

                    /* Then get the next token, if any */
                    token = strtok(NULL," \n"); /* get next token */

                    /* If there isn't a next token then move the user to their HOME directory */
                    if ( token == NULL)  /* if there isn't an arg provided to cd then move to HOME*/
                    {
                        /*resource: https://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm */
                        /* resource: https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/ */
                        chdir(getenv("HOME"));
                    }
                    else
                    {
                        /* If there is a next token then it represents the directory to move into,
                         * so move into it.
                         */
                        chdir(token);
                        
                    }
                    
                }
                /* Print the latest status or terminating signal number, if the 
                 * user enters "status" on the commandline
                 */
                else if (strncmp(bufferExpanded,"status", strlen("status")) == 0)
                {
                   /* The latest status is stored in exitStatusStr */
                    printf("%s\n",exitStatusStr);
                    
                }
                else
                {

                    /* Tokenize the expanded input buffer and put into an args array, the
                     * variable index holds the index + 1 of the latest element in the args 
                     * array and thereby indicates its size . The value of this variable is
                     * later used to delete pointer references to dynamic strings in the
                     * args array.
                     */
                    index = 0;

                    /* The tokenization and parsing of arguments occurs in the parseArguments
                     * function which additionaly parses and sets the appropriate variables
                     * if any of the arguements equal <,>, &.
                     */
                    parseArguments(&index, bufferExpanded, args, &haveInput, &haveOutput,
                                    &inBackground, fileNameIn, fileNameOut);

                    /* Fork off the current process */
                    spawnPid = fork();

                    switch (spawnPid)
                    {
                        /* Fork was unsuccessful */
                        case -1:
                        {
                            perror("Error creating fork\n");
                            exit(1);
                        }
                        /* The child process */
                        case 0:
                        {     

                            /* Redefine the signal handler parameters in the child */

                            /* Resource: Office hours questions and chapters 26, 27 in
                             * The Linux Programming Interface by Kerrisk
                             */

                            /*********** for SIGINT**********/
                            /* The child sets the sigaction handler to the SIG_DFL 
                             * which means to take the default action. This funtion 
                             * overrides the parent's handling of this signal
                             * which was to ignore it.
                             */
                            default_action.sa_handler = SIG_DFL;
                            sigaction(SIGINT, &default_action, &ignore_action);
                            
                             /*********** for SIGTSTP**********/
                            /* The child sets the sigaction handler to the ignore the
                             * SIGTSTP signal which overrides the parent's handling of 
                             * this function which was to call the catchSIGTSTP handler.
                             */

                            /* empty the set filled by the parent*/
                            sigemptyset(&SIGTSTPaction.sa_mask);  
                            sigaction(SIGTSTP, &ignore_action, &SIGTSTPaction);
                        
                            /* If the user entered < on the commandline then haveInput is 
                             * true. Get the file descriptor so the file can later be
                             * closed and set stdin to take its input from fileNameIn
                             * via the function setInputStream
                             */
                            if ( haveInput == true)
                            {
                                inFileDescriptor = setInputStream(fileNameIn);
                            }

                            /* If the user entered > on the commandline then haveOutput is 
                             * true. Get the file descriptor so the file can later be
                             * closed and set stdout to take put its output to fileNameOut
                             * via the function setOutputStream
                             */
                            if ( haveOutput == true)
                            {
                                outFileDescriptor = setOutputStream(fileNameOut);
                            }

                            /* From: https://linux.die.net/man/3/execvp
                                The execvp(functions duplicate the actions of the shell in searching for an executable file if the specified
                                filename does not contain a slash (/) character. The file is sought in the colon-separated list of directory 
                                pathnames specified in the PATH environment variable. If this variable isn't defined, the path list 
                                defaults to the current directory followed by the list of directories returned by confstr(_CS_PATH).
                            */
                            if (inBackground == true)
                            {
                                /* Ignore the SIGINT signal if process is in the background */
                                /* specify the action to be taken with a SIGINT signal */
                                sigaction(SIGINT, &ignore_action, &default_action);

                                execvpReturn = execvp(args[0], args);

                            /* If execvp returned an error then print a message and exit with a value of 1 */
                                if (execvpReturn  == -1)
                                {
                                    printf("%s: no such file or directory\n", args[0]);
                                    exit(1);
                                }
                            }
                            else
                            {
                                execvpReturn = execvp(args[0], args);

                                /* If execvp returned an error then print a message and exit with a value of 1 */
                                if (execvpReturn  == -1)
                                {
                                    printf("%s: no such file or directory\n", args[0]);
                                    exit(1);
                                }
                            }
                            

                            /* If the user entered < on the commandline then close the opened file */
                            if ( haveInput == true)
                            {
                                close(inFileDescriptor);
                            }
                            
                            /* If the user entered > on the commandline then close the opened file */
                            if ( haveOutput == true)
                            {
                                close(outFileDescriptor);
                            } 
                            
                        }

                        /* The parent process */
                        default:
                        {
                        
                            /* If the user entered & as the last character on the commandline then
                             * inBackground is true and it will be run as a background process
                             */
                            if (inBackground == true)
                            {
                                
                                /* Print the PID of the background process */
                                printf("background PID is %d\n", spawnPid);

                                /* Wait for the background process to run. The Flag WNOHANG means
                                 * don't block while the child process is running. When the childprocess
                                 * has finished running waitPidReturn is equal to spawnPid
                                 * Resource: The Linux Programming Interface by Kerrisk Chapter 26
                                 */
                                waitPidReturn = waitpid(spawnPid, &childExitMethod,WNOHANG);  

                                /* Store each background spawnPid in the pidReturn Array so it can
                                 * later be killed if for example the user chooses to exit. The pidReturn
                                 * array is dynamic. If numProcesses (the number of background processes)
                                 * is equal to the capacity of the array then double its capacity
                                 */
                                if (numProcesses == pidReturnCapacity)
                                {
                                    pidReturnCapacity = increasePidReturnArray(&pidReturn, pidReturnCapacity);
                                }

                                /* Store the background spawnPid in the pidReturn Array */
                                pidReturn[numProcesses] = spawnPid;
                                ++numProcesses;
                                
                            }
                            else
                            /* The process is run in the foreground */
                            {
                                /* Temporarily delay the TSTP signal until the foreground process has completed.
                                 * Reference: The Linux Programming Interface by Kerrisk pg 410-411
                                 * prevMask holds the previous mask and blockset is defined below
                                 * to block SIGTSTP
                                 */
                                
                                sigemptyset(&blockSet);
                                sigaddset(&blockSet, SIGTSTP);

                                /* Start the temporary signal delay */
                                if (sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1)
                                {
                                    perror("sigprocmask error\n");
                                    exit(1);
                                }
                                /* The foreground PID */
                                foregroundPID = spawnPid;

                                /* Wait for the process to run. The 0 in the last argument to waitpid 
                                 * means to block while the child process is running.
                                 */
                                waitPidReturn = waitpid(spawnPid, &childExitMethod,0);
                                
                                /* Get the termination status and if the process recieved a 
                                 * termination signal then print the signal.
                                 */     
                                terminationSignal = 0;
                                /* Get's the termination and exit status and puts it in the string 
                                 * exitStatusStr
                                 */
                                terminationStatus(childExitMethod, exitStatusStr, &terminationSignal);
                                if (terminationSignal != 0)
                                {
                                    printf("%s\n", exitStatusStr);
                                }

                                /* End the temporary signal delay. Note that this delays the signal rather
                                 * than prevent it from occuring
                                 */
                                if (sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1)
                                {
                                    perror("sigprocmask error\n");
                                    exit(1);
                                }
                            
                            }

                        }
                    }
                }
            }
            
        }   

      
        /* free any dynamic char * strings pointed to by the args array */
        i= 0;
        while (args[i] != NULL)
        {
            freeString(args[i]);
            ++i;
        }

        
       /* If necessary free the buffer, which is used to store the commandline */
        if (buffer != NULL)
        {
            free(buffer); 
        }
        
    }

    
    /* pidReturn is a dynamic array which is freed before the program exits */
    free(pidReturn);

    return 0;
}

/* Takes as input the commandline, the pid that results from $$ and expands all
 * instances of $$ in the commandline then puts the result in the input char array
 */
enum expandStatus expandString(char *str, pid_t pid, char *strExpanded)
{
   char pidStr[MAX_INT_LENGTH]; /* holds the char equivalent of pid */
   char *location;  /* location in str of first occurence of $$ */
   int index =0;  /* current array index of str[] */
   int indexExpanded =0;  /* current array index of strExpanded[] */
   int start;  /* index of location of first $$ */
   int strLength; /* length of str[] */
   int i;

    /* initialize pidStr[] and strExpanded[] */
   memset(pidStr,'\0', MAX_INT_LENGTH*sizeof(char));
   memset(strExpanded,'\0', MAX_COMMAND_LINE*sizeof(char));

   strLength = strlen(str); /* get the length of the commandline str */

    /* If strLength is greater than the maximum allowable number of characters
     * then return commandLineTooLong flag.
     */
   if (strLength >= MAX_COMMAND_LINE)
   {
       return commandLineTooLong;
   }

   /* get the location of the first occurence of $$ if any */
   location = strstr(str,"$$");

   /* convert pid to a char array */
   intToString(pidStr,(int)pid);

   if (location != NULL)
   {
        start = location - str;  /* index of location of first $$ */

        /* copy all letters up to the first occurence */
        for (i = 0; i< start; i++)
        {
            strExpanded[i] = str[i];
        }

        index = start + 2; /* jump past $$ in the input str[] */

        /* concatenate the numerical value for $$ at the end of strExpanded */
        strcat(strExpanded, pidStr);
        indexExpanded = strlen(strExpanded); /* get the string length so far */

        /* while both array index positions are less than the array lengths ...*/
        while (index < strLength  && indexExpanded < MAX_COMMAND_LINE_EXPANDED)
        {
            /* copy the chars from str to strExpanded until come across the next $$*/
            while (index < (strLength-1) && !(str[index] == '$' && str[index+1]== '$'))
            {
                strExpanded[indexExpanded] = str[index];
                ++index;
                ++indexExpanded;
            }

            /* if index < strLength -1 then came accross another $$ */
            if ( index < (strLength-1))  
            {
                index += 2; /* jump past $$ in the input str[] */

                /* concatenate the numerical value for $$ at the end of strExpanded */
                strcat(strExpanded, pidStr); 
                indexExpanded = strlen(strExpanded); /* get the string length so far */
            }
            else if (index == (strLength -1))  /* get the last character of str */
            {
                strExpanded[indexExpanded] = str[index];
                ++index;
            }
        }
   }
   else  /* if location == NULL then there were no occurances of $$ */
   {
       strcpy(strExpanded,str);
   }

   return successful;  /* the input string was expanded and was not too long */
}

/* Converts an integer to a char array and puts the result in the input
 * char array. Note, this function assumes only positive numbers are
 * passed.
 */
void intToString(char str[],int number)
{
    int ASCIIdigit;  /* the ASCII value of the digit */ 
    int index = 0, i; /* used access the char array */
    char temp[MAX_INT_LENGTH];  /* temporarily holds the number as a char* */

    /* initialize the strings */
    memset(str, '\0', MAX_INT_LENGTH);
    memset(temp, '\0', MAX_INT_LENGTH);

    /* Get the next tens digit and add it to temp, this produces the
     * digits in reverse order in the temp array
     */
    while (number > 0)
    {
        ASCIIdigit = number%10 + 48;  /* the ASCII number of the next digit*/
        temp[index] = (char)(ASCIIdigit);
        number = number/10;
        ++index;
    }

    /* reverse the string and put in str[] */
    for (i = 0; i < index; i++)
    {
        str[i] = temp[index - (i + 1)];
    }

}

/* Takes the $$ expanded commandline and parses it into an args[] array to 
 * pass onto excvp and also gets the relevant stream data if <, >, & is found
 * in the commandline. In addition, if <, >, and/or & are found in the command
 * line appropriate flags are set.
 */
void parseArguments(int *index, char bufferExpanded[], char *args[], bool *haveInput, bool *haveOutput,
                   bool *inBackground, char fileNameIn[], char fileNameOut[])
{
    char *token;  /* holds each string token from the commandline */
    int  argSize; /* holds the size of token being examined */
    
    /* tokenize the input buffer and put into an args array */
    token = strtok(bufferExpanded," \n");

    while( token != NULL ) {
        
        /* if find < then get the input filename and store it in "fileNameIn" */
        if (token != NULL && strcmp(token, "<") == 0)
        {
            
            /* read in the next token which should be the file name */
            token = strtok(NULL, " \n");
            
            if ( token != NULL)
            {
        
                memset(fileNameIn,'\0', MAX_FILE_NAME);
                strcpy(fileNameIn, token);

                /* boolean indicates to expect input from file */
                *haveInput = true; 

                token = strtok(NULL, " \n"); /* read in the next token */

            }
            else
            {
                fprintf(stderr, "Error getting input filename\n");
                exit(3);
            }
            
        }
         /* if find > then get the input filename and store it in "fileNameOut" */
        if (token != NULL && strcmp(token, ">") == 0)
        {
            
            /* read in the next token which should be the file name */
            token = strtok(NULL, " \n");
            
            if ( token != NULL)
            {
                memset(fileNameOut,'\0', MAX_FILE_NAME);
                strcpy(fileNameOut, token);

                /* boolean indicates to expect to output file */
                *haveOutput = true;

                token = strtok(NULL, " \n"); /* read in the next token */

            }
            else
            {
                fprintf(stderr, "Error getting output filename\n");
                exit(3);
            
            }

        }
        /* if the next token is "&" then process accordingly: */
        if (token != NULL && strcmp(token, "&") == 0 )
        {
            /* read in the next token */
            token = strtok(NULL, " \n");

            /* If the next token is not NULL the & is not at the end of the 
             * command line and hence is treated like a char, in which case
             * it is added to the array of args that is sent to execvp()
             */
            if (token != NULL )
            {
                argSize = strlen("&");

                /* have args[*index] point to a dynamic char[] of size argSize */
                args[*index] = getEmptyString(argSize);

                /* put the token in the array element */
                strcpy(args[*index], "&"); 

                *index += 1; /* increment the array index */
            }
            /* If the next token is NULL the the & is at the end of the command
             * line and hence indicates the command is to be run in the background.
             */
            else if (canPutInBackground == TRUE ) /* TRUE if not in foreground-only mode*/
            {
                /* set boolean to indicate process is in the background */
                *inBackground = true;

                /* If input is not redirected from a file then set it to redirect
                 * from /dev/null
                 */
                if (*haveInput == false)
                {
                    strcpy(fileNameIn, "/dev/null");
                    *haveInput = true;  /* indicate that input is redirected */
                }

                /* If output is not redirected to a file then set it to redirect
                 * to /dev/null
                 */
                if (*haveOutput == false )
                {
                    strcpy(fileNameOut, "/dev/null");
                    *haveOutput = true; /* indicate that output is redirected */
                }  
            }
            
            
        }

        /* if a token is anything except <,>, or & then put it in the args array that is
         * sent to execvp()
         */
        if (token != NULL && !(strcmp(token, ">") == 0 || strcmp(token, "<") == 0 || strcmp(token, "&") == 0 ))
        {
            argSize = strlen(token) + 1; /* add extra room for '\0' */

             /* have args[*index] point to a dynamic char[] of size argSize */
            args[*index] = getEmptyString(argSize);

            strcpy(args[*index], token);

            *index += 1;    /* increment the array index */

            token = strtok(NULL, " \n"); /* get the next token */
        }
    
    }

}

/* Takes in the status returned from waitpid (childExitMethod) and runs it
 * through WIFSIGNALED, WTERMSIG, WIFEXITED and/or WEXITSTATUS to get either an
 * exit or termination status and puts the result in the string argument.
 */
void terminationStatus(int childExitMethod, char exitStatusStr[],int *terminationSignal)
{
    int exitStatus;
    
    /* initialize the input string */
    memset(exitStatusStr,'\0', STATUS_LENGTH);

    /* If the process was signaled then get the signal number and put it
     * in the exitStatusStr string.
     */
    if (WIFSIGNALED(childExitMethod) !=0 )
    {
        *terminationSignal = WTERMSIG(childExitMethod);/* returns the signal number*/
        sprintf(exitStatusStr, "terminated by signal %d",*terminationSignal);
    }
    /*Else, get the exit status and put it in the exitStatusStr string */
    else if (WIFEXITED(childExitMethod))
    {
        exitStatus = WEXITSTATUS(childExitMethod); /* returns the exit value */
        sprintf(exitStatusStr,"exit value %d",exitStatus);
    }
}


/* Takes as input the name of a file from which the input stream stdin will
 * be diverted to, opens it, sets it to replace stdin and returns the file
 * descriptor obtained by opening the file for reading.
 */
int setInputStream(char *fileNameIn)
{
    int inFileDescriptor;  /* the file descriptor value returned by open() */
    int dup2InReturn; /* the return value of dup2() */
    char errorMessage[ERROR_MESSAGE_MAX];  /* used to hold an error message */

    /* open a file for reading */
    inFileDescriptor = open(fileNameIn, O_RDONLY);

    /* if unsuccessful in opening file print an error message to stderr and exit*/
    if (inFileDescriptor == -1)
    {
        memset(errorMessage,'\0', ERROR_MESSAGE_MAX);
        sprintf(errorMessage, "cannot open %s for input\n", fileNameIn);
        fprintf(stderr,errorMessage);
        exit(1);
    }

    /* use dup2 to set stdin to the file opened above */
    dup2InReturn = dup2(inFileDescriptor, 0);

    /* if dup2 is unsuccessful set an error message and exit*/
    if (dup2InReturn == -1 )
    {
        perror("dup2() unsuccessful\n");
        exit(1);
    }

    /* return the file descriptor */
    return inFileDescriptor;
}

/* Takes as input the name of a file from which the output stream stdout will
 * be diverted to, opens it, sets it to replace stdout and returns the file
 * descriptor obtained by opening the file for writing.
 */
int setOutputStream(char *fileNameOut)
{
    int outFileDescriptor;  /* the file descriptor value returned by open() */
    int dup2OutReturn;  /* the return value of dup2() */
    char errorMessage[ERROR_MESSAGE_MAX]; /* used to hold an error message */

    /* open a file for writing, where if the file does not exist it will be
     * created and if it does it will be truncated.
     */
    outFileDescriptor = open(fileNameOut, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    /* if unsuccessful in opening file print an error message to stderr and exit*/
    if (outFileDescriptor == -1)
    {
        memset(errorMessage,'\0', ERROR_MESSAGE_MAX);
        sprintf(errorMessage, "cannot open %s for output\n", fileNameOut);
        fprintf(stderr,errorMessage);
        exit(1);
    }

    /* use dup2 to set stdout to the file opened above */
    dup2OutReturn = dup2(outFileDescriptor, 1);

    /* if dup2 is unsuccessful set an error message and exit*/
    if (dup2OutReturn == -1 )
    {
        perror("dup2() unsuccessful\n");
        exit(1);
    }

    /* return the file descriptor */
    return outFileDescriptor;
}

/* Initialize each element of the args array used to hold the commandline
 * arguments to pass to execvp. Each element is initialized to NULL.
 */
void initializeArgs(char *ptrArray[], int size)
{
    int i;
    
    for (i = 0; i < size; i++ )
    {
        ptrArray[i] = NULL;
    }
    
}

/* Creates a dynamic char array for an element of the args array to point to.
 * The array has size elements.
 */
char *getEmptyString(int size)
{
    char *temp;

    temp = (char *)malloc(size*sizeof(char));

    if (temp == NULL)
    {
        fprintf(stderr, "Malloc not successful\n");
        exit(1);
    }

    return temp;
}

/* Frees a dynamic char string and sets its pointer value to NULL.
 * input: the dynamic char array: char *
 */
void freeString(char *str)
{
    free(str);
    str = NULL;
}
