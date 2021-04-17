#  C Bash Shell Emulator  

Developed a shell in C that emulates a bash shell where command line arguments are prompted for and commands are run. Features include:

*  Redirection <, > is supported
*  Supports foreground and background processes
*  Built in commands include *exit*, *cd*, and *status*
*  Comment lines begin with #
*  Ctrl-C from the keyboard sends a SIGINT signal to the parent process
*  Ctrl-Z from the keyboard sends a SIGTSTP signal to the parent shell process and all children at the same time. Send a second Ctrl-Z signal to resume.

##  Instructions

General command line syntax is:
*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*

#### To compile and run the code:

1. Make sure the makefile is in the same directory as the code
1. On the commandline enter: *make*
      This will compile the code into an executable/binary called *bashShell*
1. On the commandline enter: *./bashShell*

#### To clean up:
On the commandline enter: *make clean*
 
