# Main

1. We begin execution of our program in main. Main creates a type process and
then sends it to the execution function along with the user input to get_input()
Process struct stores the pid of the last command in the chain of command, the
raw entire user input, number of exit statuses, background value of the last
command in the chain of commands, and the execution state.

# Parsing get_input()

1. First, this function parses the entire user input and stores the inputs
that were separated by space in the original input. We took care of adding
spaces around redirects and the background sign here.

2. Parsing for spaces around redirects was a little tricky because there could
be space before, after or both. Instead of checking each case and then adding
spaces accordingly, I added spaces before and after each. Then in the second
round of parsing, these spaces are ignored and stored into a new character
array.

## Second Round Parsing

3. All this is added to another array where these spaces
are removed. We checked for spaces using isspace() function and while there
continues to be no space, record the input into the args[k] main array. The
original input is preserved so that we can print it out at the complete stage.


3. After that, there is a loop to categorize and interpret the user input into
commands, followed by arguments. We have a commands array to store these
commands.

4. Each command has a line, argument list, number of arguments, fd_in, fd_out,
background boolean, and pid(to keep track of its status later). Pipes are
ignored, we use them to split up each command.
Example: ls -l is a single command. ls is the line and -l is the argument
in each command.

5. fd_in and fd_out are assigned to open() (RDWR/OCREAT) if there is a redirect
option while parsing. Mislocation is also checked here by checking where the
redirection symbol is in the array.

6. If there is a & sign, the command is marked as type background. The only
difference for the & sign is that the parent does not wait for the child.
However, checking for completion and printing accordingly required creating the
Process struct.

# Execute() + Pipe algorithm

1. After interpreting and categorizing each command into the array. There is a
for loop to process the commands. First we pipe() and then fork. The child will
then execute the process. Pipe read is closed and the STDOUT of the child is
written to the pipe. The next commands fd_in is read from the pipe READ
from the previous command. This is how we processed pipes.

## execute_cmd

1. This function checks the fd of the command that were assigned and then
redirects the outputs and the inputs into the child before actually executing
the process.
Everytime the child executes a command (ex ls -l),the exit status is stored in
the status array of the process. The last child does not output to the pipe, it
outputs to STDOUT or redirect output if there is one. After the last command in
the chain of piped commands, the pipe is closed.  

## Sleep & implementation

1. At the end of the execution line for a chain of commands
(ls -l | grep sshell | wc -l). The parameters that are needed to print the
complete statement are recorded in the Process array. In the main function,
we use waitpid with the pid of the parent's last command in the chain to check
if it's child has finished executing. If this is complete then print out the
complete status using the printPr function. Waitpid waits for the child to
change state we used the parameter WNOHANG so it does not hang the terminal and
continues to accept new processes. If the last command in the execution chain
of the string of commands is not a background command then print it right after
it is done. After printing, the state of the process in the process array is
changed to done so that it is not checked again.

## Active background process checking

1. When the user tries to issue exit, we make sure that the number of processes
that have exited is equal to the number of process that have issued, this way
we can make sure that all the background running processes are complete before
exiting the shell.

## Built in commands

2. In this function, we implemented cd and pwd. We simply check the command line
to see if it matches with cd or pwd and if it does, then it executes the
appropriate command. We also check to see if a valid directory is entered after
the user enters cd and if he/she doesn't, an error is printed. This is not
executed in the child, the parent executes this function.

## Debug

1. To debug our program, we used GDB. This was a helpful program because it
made it easier to find errors with our implementation that the compiler didn’t
see. Using this program, we were able to step through our code and find the
exact line which lead to errors. There were many times that we ran into
segmentation faults and using this program, we were able to find out that it
was because we had not allocated memory properly.

2. Manual Commands:
In order to verify that our program was doing what we wanted it to do, we
gave it numerous amounts of test cases that we created. After completing a
phase, we tried to give the program the trickiest cases to make sure that our
algorithm covered a large range of inputs. We also fed arbitrary inputs to
shell_ref and compared the output of sshell_ref with the output of the sshell
we implemented.

3. Tester:
After completing the first four phases, we ran our program through the
tester to verify whether or not our design was successful. We thought that
since the tester covered only the base cases for the program, it was extremely
crucial to make sure that our program passed all the cases in the tester.

# Challenges

1. One of the biggest difficulties we faced was when we implemented pipelines.
We had many approaches to this problem at first, but we realized that it was
best to have seperate children execute a single process in the pipe and then
keep feeding that output into the input of the next pipe.
2. Another major difficulty we faced was during implementing background
processes. At first, we didn’t know how to keep track of which processes were
running and which had finished, so we ended up creating a struct which kept
track of which processes were and which ones had already executed.


# Sources
Stackoverflow was used to understand how to implement multiple pipes, background
processes, allocating memory and freeing up the allocated space

https://www.gnu.org/software/libc/manual/html_mono/libc.html#Process-Completion
https://cboard.cprogramming.com/c-programming/83405-clearing-pipe-used-ipc.html
http://pubs.opengroup.org/onlinepubs/009695399/functions/chdir.html
https://stackoverflow.com/questions/12762944/segmentation-fault-11
https://stackoverflow.com/questions/2971709/allocating-memory-to-char-c-language
