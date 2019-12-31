#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include  <sys/types.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>


#include <fcntl.h>

#define MAX 512355

typedef struct
{
        char *line; //main command ie "ls"
        char **args; // -l -s arguments
        int tokens; //number of arguments
        int fd_in; // FD in
        int fd_out; //FD out
        int bg; //background
        int pid; //PID OF LAST PROCESS
}command;

typedef struct {
        int pidL;
        int trackstat;
        char * line;
        int stat[100];
        int bg;
        int state;


}Process;

void execute_cmd(command exC);
int builtin(command exC);
void printPr(int stat[], int trackstat, char *line);


void get_input(char* line, char **args, Process *pnum) // Separate command and arguments
{

        int i,j=0,k=0; // i for line from user argument, j to store char in temp, k to finally store the arg in the argument list

        char line2[700]; //fixed version of line
        int m=0;

        //Add space around redirect
        for(i = 0; i < strlen(line); i++,m++) {

                if(line[i] == '<' || line[i] == '>' || line[i] == '&' || line[i] =='|') {

                        line2[m] = ' ';
                        m++;
                        line2[m] = line[i];
                        m++;
                        line2[m] = ' ';

                        //m++;
                }
                else{
                        line2[m] = line[i];
                }


        } //iterate through input)

        line2[m] = '\0'; // mark end of user spaced input

        //Clean up spaces and add to list of user input
        for(i = 0; i < strlen(line2); i++) //iterate through input
        {
                if (!isspace(line2[i])) //if NOT tab space or new line carriage return..
                {
                        char temp[512] = " ";

                        while((!isspace(line2[i]))) { //character found, start recording
                                temp[j] = line2[i]; //store character
                                j++;
                                i++;
                        }
                        j=0; //reset position in the termporary character array

                        //temp[j++] = ' '; //newline between arguments- debgging
                        args[k] = malloc(sizeof(temp) + 1); //allocate memory for that argument + terminating
                        strcpy(args[k], temp);
                        k++; //go to next
                }

        }
        command commands[10]; //10 commands (With args)


        //initialize commands
        for(int x = 0; x < 10; x++) {
                commands[x].line = " ";
                commands[x].args = malloc(sizeof(char) * 100); //I guess you have to allocate for TOTAL ARGUMENTS EACH COMMAND during initialization
                commands[x].args[0] = "";
                commands[x].tokens = 0; //because execvp skips first argument
                commands[x].fd_in = 0; //initialize as normal command
                commands[x].fd_out = 0; //initialize as normal command
                commands[x].bg = 0; //initliaze as bg 0
                //commands[x].pid;
        }

        int cmdC=0; //commands countbg

        //mark end of arugment list EVERYTHING GATHERED FROM USER LIST. Ex: 'ls' '-l' \0
        args[k] = NULL;


        commands[0].line = malloc(sizeof(args[0]) + 1);
        strcpy(commands[0].line, args[0]); //copy first command

        //Iterate through split up input and create Commands with arguments
        for (int l=1; l < k; l++) {

                if(strcmp(args[l], "&")==0) {
                        if(l+1 == k) { //last command
                                commands[cmdC].bg =1;
                                break;
                        }
                        else{
                                fprintf(stderr, "Error: mislocated background sign\n" );
                                return;
                        }
                }
                if(strcmp(args[l], ">")==0) { //OUTPUT REDIRECT DETECTED
                        commands[cmdC].args[commands[cmdC].tokens+1] = NULL; //END OF THE COMMAND
                        //move past REDIRECT
                        if(l+1 == k) { // last command
                                fprintf(stderr, "Error: no output file\n");
                                return;
                        }
                        else if(l+1 != k-1) { //second to last command
                                fprintf(stderr, "Error: mislocated output redirection\n");
                                return;
                        }
                        else{
                                l++;
                                if ((commands[cmdC].fd_out = open(args[l], O_CREAT | O_RDWR, 00400 | 00200))  < 0) {
                                        fprintf(stderr, "Error: cannot open output file\n");
                                        return;
                                }
                        }

                }
                else if((strcmp(args[l], "<")==0)) { //INPUT REDIRECT DETECTED
                        commands[cmdC].args[commands[cmdC].tokens+1] = NULL; //END OF THE COMMAND

                        if(l+1 == k) { // last command
                                fprintf(stderr, "Error: no input file\n");
                                return;
                        }
                        else if(cmdC == 1) { //should be argument of the first command
                                fprintf(stderr, "Error: mislocated input redirection\n" );
                                return;
                        }
                        else{
                                l++; //move past REDIRECT
                                if ((commands[cmdC].fd_in = open(args[l], O_RDWR, S_IRUSR | O_RDWR, 00400 | 00200)) < 0) {
                                        fprintf(stderr, "Error: cannot open input file\n");
                                        return;
                                        //cmdC++;         //ready for next command
                                }
                        }

                }
                else if(strcmp(args[l], "|")!=0) //check for NO PIPE
                {
                        commands[cmdC].tokens++; //next argument
                        //printf("%d recorded %s \n", commands[cmdC].tokens, args[l]);
                        //sizeof(char) * len(args[i])
                        commands[cmdC].args[commands[cmdC].tokens] =  malloc(sizeof(args[l]) + 1); //allocate memory for EACH argument
                        strcpy(commands[cmdC].args[commands[cmdC].tokens], args[l]); //copy argument to object

                }
                else{ //Pipe detected
                        commands[cmdC].args[commands[cmdC].tokens+1] = NULL; //END OF THE COMMAND
                        l++; //move past pipe
                        cmdC++; //ready for next command

                        commands[cmdC].line = malloc(sizeof(args[l]) +1); //Allocate memory for Line Command
                        strcpy(commands[cmdC].line, args[l]);

                }

        }



// Remove new line character from user input
        for (int li = 0; li < strlen(line); li++) {
                if(line[li] == '\n') {
                        line[li] = '\0';
                }
        }

        int stat[500]; //Store all the statuses from the processes
        int status; // each individual status
        int trackstat=0; // the index in the status array


        int b_in; //tracking status of built in command

        if((b_in = builtin(commands[0]))>=0) {
                stat[trackstat++] = b_in;
                printPr(stat, trackstat, line);
                return;
        }

        //Execute all command objects
        for(int m=0; m < cmdC+1; m++) { //main loop
                pid_t pid;
                int des_p[2];

                pipe(des_p);

                if(commands[cmdC].tokens > 16) {
                        fprintf(stderr, "Error: too many arguments\n");
                        return;
                }


                pid = fork();

                if(pid < 0) {
                        perror("fork");
                        exit(0);
                }
                if(pid == 0) { //child process writes to pipe

                        close(des_p[0]);         //close CHILD read to pipe

                        if(m+1 < cmdC+1) { //Dont put studout in PIPE if last command
                                dup2(des_p[1], STDOUT_FILENO); //Write to pipe

                                close(des_p[1]); //close Write to pipe
                        }

                        close(des_p[1]);

                        execute_cmd(commands[m]);         //child executes process
                        //exit(0);
                }
                else{ //parent
                        commands[m].pid = pid;
                        //fprintf(stderr, "HELLO2 %d\n", pid);

                        if(commands[m].bg ==0) {
                                waitpid(pid, &status, 0);
                                stat[trackstat++] = WEXITSTATUS(status);
                        }

                        close(des_p[1]);         //don't need to write to pipe

                        //check if not last command
                        if(m+1 < cmdC+1) {
                                commands[m+1].fd_in = des_p[0];         //next command reads from pipe

                        }
                        else{

                                //Storing process information
                                pnum->pidL = commands[cmdC].pid;
                                for(int stati = 0; stati < trackstat; stati++) {
                                        pnum->stat[stati] = stat[stati];
                                }
                                pnum->trackstat = trackstat;
                                pnum->line = malloc(sizeof(line)*2);
                                pnum->state = 0;
                                strcpy(pnum->line,line);

                                pnum->bg = commands[cmdC].bg;

                                close(des_p[0]);

                        }


                }

        }

}

void printPr(int stat[], int trackstat, char *line){

        fprintf(stderr, "+ completed '%s' ",line);
        for(int stati = 0; stati < trackstat; stati++) {
                fprintf(stderr, "[%d]", stat[stati]);
        }
        fprintf(stderr, "\n");
}

int builtin(command exC){
        //pwd
        if(strcmp(exC.line, "pwd") == 0)
        {
                char working_dir[3000];
                if (getcwd(working_dir, 3000) == working_dir)
                {
                        printf("%s\n", working_dir);
                        return 0;

                }
        }
        //cd
        else if(strcmp(exC.line, "cd") == 0)
        {
                int x = chdir(exC.args[1]);
                //fprintf(stderr, "%s\n", exC.args[1]);
                if(x == 0)
                {
                        return 0;
                }
                else
                {
                        fprintf(stderr, "Error: no such directory \n");
                        return 1;

                }

        }
        return -1; //return -1 if not built in command

}

void execute_cmd(command exC)
{

        if(exC.fd_out !=0 && exC.fd_in ==0) {                         //IF ONLY >

                dup2(exC.fd_out, STDOUT_FILENO);                         //take output to Pipe
        }
        else if(exC.fd_out ==0 && exC.fd_in !=0) {                         //IF ONLY <

                dup2(exC.fd_in, STDIN_FILENO);
        }
        else if(exC.fd_out !=0 && exC.fd_in !=0) {                         //IF  > <, < >,

                //int fd_out = open
                dup2(exC.fd_in, STDIN_FILENO);                         //input from buffer
                dup2(exC.fd_out, STDOUT_FILENO);                         //take output to Pipe
                //fprintf(stderr, "HERE \n");
        }
        if (execvp((exC.line), exC.args) < 0)
        {                         /* execute the command  */
                fprintf(stderr, "Error: command not found\n");
                _exit(EXIT_FAILURE);
        }

        exit(0);

}


int main(void)
{
        char line[512] = ""; //initialize to empty string
        char* args[16];

        int totalp=0;
        int finCount =0;

        Process pr[120];

        while (1)
        {
                fprintf(stdout, "sshell$ ");
                fgets(line, 512, stdin);

                if (!isatty(STDIN_FILENO)) {
                        printf("%s", line);
                        fflush(stdout);
                }

                if(line[0] != '\n' && line[1] != '\0') {
                        //EXIT CASE
                        if(strcmp(line, "exit\n")==0)
                        {
                                if(finCount == totalp) {
                                        fprintf(stderr, "Bye...\n");
                                        exit(0);
                                }
                                else{
                                        fprintf(stderr, "Error: active jobs still running\n");
                                }
                        }
                        else{
                                get_input(line, args, &pr[totalp]);
                                char buff[5];
                                memcpy(buff, &line[0], 3);
                                buff[4] = '\0';

                                if((strcmp(buff, "cd ")!=0) && (strcmp(buff, "pwd")!=0)) {
                                        //fprintf(stderr, "HELLO\n");
                                        totalp++;
                                }
                        }
                }

                int active;
                int estat;
                for(int p_num=0; p_num < totalp; p_num++ ) {
                        if(pr[p_num].state == 0) {
                                active = waitpid(pr[p_num].pidL, &estat, WNOHANG); //take care and wait for Sleep background process

                                if(pr[p_num].bg == 0 || active) {
                                        if(active && pr[p_num].bg != 0) { //Exiting a sleep background process
                                                if(pr[p_num].line) {
                                                        pr[p_num].stat[pr[p_num].trackstat++] = WEXITSTATUS(estat);
                                                        printPr(pr[p_num].stat, pr[p_num].trackstat, pr[p_num].line);
                                                        pr[p_num].state = 1; //dont' execute again
                                                        if((strcmp(pr[p_num].line, "cd\n")!=0) && (strcmp(pr[p_num].line, "pwd\n")!=0)) {
                                                                finCount++;
                                                        }
                                                }
                                        }
                                        else{
                                                if(pr[p_num].line) {
                                                        printPr(pr[p_num].stat, pr[p_num].trackstat, pr[p_num].line);
                                                        pr[p_num].state = 1; //dont' execute again
                                                        if((strcmp(pr[p_num].line, "cd\n")!=0) && (strcmp(pr[p_num].line, "pwd\n")!=0)) {
                                                                finCount++;
                                                        }
                                                }
                                        }

                                }
                        }
                }



        }

}
