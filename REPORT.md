# ECS 150 Simple Shell Report

## Summary

This program, `sshell`, will addess a prompt and accpet a line of commands by
the user. The command line will be read and the program will execute the 
requested commands. Any output message will be printed back on the terminal 
and another prompt will be displayed. The program will continue to accpet 
commands until user inputs 'exit'. Then the program will close.

## Implementation

The implementation of this project follows three distinct steps:

1. Parsing the command line given as program parameters
2. Executing the appropriate commands that were requested by users
3. Managing potential errors that may rise when performing

### Parsing the Command Line

There are three main categories we focused our parsing structure on when the 
user inputs a line of commands:

1. Commands and arguments
2. Pipline command '|'
3. Output redirection '>'

#### Commands and Arguments

The shell program expects the user to input a formulated command on the 
accepting commandline. It is assumed that a simple user command is formulated 
with the performing single command first and the corresponding arguments 
followed. We created a character array that will store characters from the 
input command string. Then we structured our parsing code to indicate the first 
location of a white space. This will grab the single string command that the 
program will be performing and store it in the first index of the character 
array. Then we looped through the rest of the input string. After every 
indication of a new white space, that section of the string will be stored in 
the next empty index of the character array. Every index after index 0 is 
recognized as an argument.

#### Piping 

There are 2 levels of parsing for piping functionality. In first level of 
parsing the piping command we needed to check for '|' character, and then 
seperate different processes that need to be pipied. We saved the seperate 
processes in a 'char*' array, so that we can run them and pipe them one by one. 
In second level we parse each process, i.e, seperating the command and 
arguments and saving them in buffer (like in the normal parsing) to be executed 
later.

The extra feature of piping is '|&', where we just add a small feature to the 
existing parsing, that is a check for '&' character in the processes, and since 
we split the command line at '|' character, we will find the '&' symbol one 
process after the actual process that needs to be redirected. And then we turn 
on a flag that will be checked later discussed in execution section. 

#### Output Redirection
Parsing output redirection is similar to the parsing of piping discussed above. 
We check for the '>' character and save the file name in a buffer, and send the 
rest as the command to be parsed and executed later.

Similar to the extra feature of piping, in extra feature of output redirection 
we just check for '&' character in the file name part of teh string, since we 
broke the command line string at '>' character, so if the extra feature is used 
then it would have been sent to the file name buffer and turn on a flag to be 
checked later and execute it accordingly. Discussed further in execution 
section.

### Execution

The exectable step consist of three sub categories:

1. Simple and builtin commands
2. Pipline commands '|' and '|&'
3. Output redirection '>' and '>&'

#### Simple & Builtin Commands

In this step, every parsed user input will have a single, performing command.
The command may either be a simple command or a builtin command. The simple 
command can be executed with the fork + exec + wait method. More specifically,
we used the function `execvp()` to automatically search programs in the $PATH.

As for the builtin commands, we individually implemented cd, pwd, and exit. The 
implementation starts by a list of comparing the first index of the created 
character array to the commands' strings: 'cd', 'pwd', and 'exit'. When the 
user's command matches the listed builtin commands, that command will execute 
through the coded functions to perform cd and pwd. However, the exit command 
with only print the ending message to stderr and break out of the main while 
loop in the shell. 

Function `change_dir()` parses through both the current location path and the 
given path, storing the individual directory names in character arrays. We then 
compare the current path array with the given path array to indicate what is 
the how many directories in the two paths are the same. The program will then 
cd back to the common directory of the two paths and then cd forward through 
the rest of the given path array.

Function `print_work_dir()` calls the `getcwd()` function to retrieve the full 
path of where the current directory is located and then prints to the stdout.

#### Piping

After the processes are parsed, we fork from the existing process and then we 
create pipe and setup the input and output redirections, then we implement the 
pipe and execute the processes. For multiple piping we check if the process 
array has more processes, and then used conditional statements to create 
multiple pipes. When we do the output redirection to the pipe, we check for the 
if the flag is on, then redirect the stderr in the pipe as well. 

#### Output Redirection

Before we execute the command, we check if the output redirection flag is on. 
If it is on then we open or create the file using the filename saved in the 
buffer, and redirect the output to the file, and then let the command run 
normally. The flags specify the type of output redirection we need to perform, 
i.e., normal or extra feature. So, according to the flag value we decide if we 
need to redirect only stdout or stderr as well. 

### Error Management

Function `error_message()` switches through a list of error cases and prints 
out the corresponding error message. At every location that a potential error 
may occur, the function is called with appropriate error.

Function `perror()` is used to report any failures of library functions.

### Sources

1. Source links provided by Professor Proquet in Project1. html.
2. '<dirent.h>`: https://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html