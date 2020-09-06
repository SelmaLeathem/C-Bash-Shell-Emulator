#  C Bash Shell Emulator  

Developed a shell called *smallsh* in C that works like a bash shell where command line arguments are prompted for and commands are run. Features include:

*  Redirection <, > is supported
*  Supports foreground and background processes
*  Built in commands include *exit*, *cd*, and *status*
*  Comment lines begin with #
*  Ctrl-C from the keyboard sends a SIGINT signal to the parent process
*  Ctrl-Z from the keyboard sends a SIGTSTP signal to the parent shell process and all children at the same time. Send a second Ctrl-Z signal to resume.

## To Run Code in Google Colaboratory

1. Vist https://colab.research.google.com/drive/1tyOQP_J1ck2TXvtLJodm15Az-d7yEjBp?usp=sharing
1. Select Runtime-> Factory reset runtime
1. Select Runtime-> Run all 
1. In last cell enter commands into the box
1. enter *exit to exit

##  Instructions

General command line syntax is:
*command [arg1 arg2 ...] [< input_file] [> output_file] [&]*

#### To compile and run the code:

1. Make sure the makefile is in the same directory as the code
1. On the commandline enter: *make*
      This will compile the code into an executable/binary called *smallsh*
1. On the commandline enter: *./smallsh*

#### To clean up:
On the commandline enter: *make clean*
 
