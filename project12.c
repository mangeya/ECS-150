#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>   
#include <string.h>  

#MANYA KAUSHIK 919929405
#SNEHA KALE 919235319
# PROJECT 1.2 ECS 150

#define CMDLINE_MAX 512
#define MAX_NO_OF_ARGS 16
#define MAX_LEN_OF_INDIVIDUAL_ARG 32
#define FALSE 0
#define TRUE 1
// 16 non null args
// 16 pipes
#define MAX_COMMANDS  25 

//  one background job at any given time.
#define MAX_BACKGROUND_JOBS 1

//defining variables 
pid_t background_pids;
char *background_cmd = NULL;

int background_command_status[1]; 

int BACKGROUND_COMMAND_PRESENT = FALSE;
int BACKGROUND_STATUS_PRESENT = FALSE;

int *command_status = NULL;
int total_commands = 0;
int BUILTIN_COMMAND = FALSE;

char *orig_command;


// global reset function

void reset_globals(){
        BACKGROUND_COMMAND_PRESENT = FALSE;
        BUILTIN_COMMAND = FALSE;

        total_commands = 0;

        if(command_status != NULL ){
                free(command_status);
                command_status = NULL;
        }
        //printf("RESET ON 1 11 \n");
        if(orig_command != NULL){
                free(orig_command);
                orig_command = NULL;
        }
        //printf("RESET DOEN\n");

        
}



//sshell@ucd$ cat /dev/urandom | base64 -w 80 | head -5
//Should result in 
//+ completed 'cat /dev/urandom | base64 -w 80 | head -5' [0][0][0]


void command_execution_status(char *cmd, int *status, int command_count) {
        if(command_count == 0) return;
        fprintf(stderr, "+ completed '%s' ", cmd); 
        for (int i = 0; i < command_count; ++i) {
                fprintf(stderr, "[%d]", status[i]);

        }
        fprintf(stderr, "\n"); 
        fflush(stdout);
        fflush(stderr);
}

int main(void)
{

        //signal(SIGCHLD, sigchld_handler);
        
  
        while (1)
        {

                
                char cmd[CMDLINE_MAX];

                //global reset

                reset_globals();

                
                // Print prompt 
                printf("sshell@ucd$ ");
                fflush(stdout);

                // Get commnd line 
                if (fgets(cmd, CMDLINE_MAX, stdin) == NULL) {
                        break; 
                }

                char *nl;
                // Remove trailing newline 

                nl = strchr(cmd, '\n');
                if (nl) {
                        *nl = '\0';
                }
                // Print command line if stdin is not provided by terminal 
                if (!isatty(STDIN_FILENO))
                {
                        fprintf(stdout,"%s\n", cmd);
                        fflush(stdout);
                }
                // for blank lines
                 
                if (strlen(cmd) == 0) continue;

                orig_command = malloc(strlen(cmd) + 1);
                if (!orig_command)
                {
                        perror("error while malloc");
                        exit(1);
                }
                

                strcpy(orig_command, cmd);

                if (strchr(cmd, '&')) {
                        BACKGROUND_COMMAND_PRESENT = TRUE;
  
                       // check ampersand error
                        char *last_ampersand = strrchr(cmd, '&');
                        if (last_ampersand != NULL && *(last_ampersand + 1) != '\0') {
                                fprintf(stderr, "Error: mislocated background sign\n");
                                free(orig_command);
                                orig_command = NULL;
                                continue;
                        }
                        // check if there is only one &
                        //strchr check for first occurance
                        // strrchr checks for last occurance
                        // first and last should be same ortherwise error
                        if (strchr(cmd, '&') && strchr(cmd, '&') != strrchr(cmd, '&')) {
                                fprintf(stderr, "Error: mislocated background sign\n");
                                free(orig_command);
                                orig_command = NULL;
                                continue;
                        }

                        //check if | is there in the command
                        if (strchr(cmd, '|')) {
                                fprintf(stderr, "Error: mislocated background sign\n");
                                free(orig_command);
                                orig_command = NULL;
                                continue;
                        }
                        *strchr(cmd, '&') = '\0';
                        //copy cmd to background_cmd
                        background_cmd = malloc(strlen(orig_command) + 1);
                        if (!background_cmd)
                        {
                                perror("error while malloc");
                                exit(1);
                        }
                        strcpy(background_cmd, orig_command);
                }


                fflush(stdout);


                // Pipe split

                char *commands[MAX_COMMANDS];
                
                int total_commands = 0;
                char *next_command = strtok(cmd, "|");
                
                while (next_command != NULL && total_commands < MAX_COMMANDS) {
                        
                        //fflush(stdout);
                        //exit(0);        
                        //while (*single_command == ' ') single_command++; // trim leading spaces
                        commands[total_commands++] = next_command;
                        next_command = strtok(NULL, "|");
                }
                
                //printf("total_commands %d \n", total_commands);

                int in_fd = 0;
                int pipe_fd[2];
                
                command_status = malloc(sizeof(int) * total_commands);
                if (!command_status)
                {
                        perror("error while malloc");
                        exit(1);
                }
                pid_t pids[total_commands];
                for (int i = 0; i < total_commands; i++) {
                        char *arguments[MAX_NO_OF_ARGS];
                        char *input_redirection = NULL; // for < 
                        char *output_redirection = NULL; // for > 
                        int input_redirection_detected = FALSE;
                        int output_redirection_detected = FALSE;

                        int stop = FALSE;


                        int arguments_count = 0;

                        char *argument = strtok(commands[i], " ");
                        while (argument != NULL)
                        {
                                if (arguments_count >= MAX_NO_OF_ARGS)
                                {
                                        // fprintf(stderr, "Error");
                                        stop = TRUE;
                                        break;
                                }

                                if (strlen(argument) > MAX_LEN_OF_INDIVIDUAL_ARG)
                                {
                                        // fprintf(stderr, "Error");
                                        stop = TRUE;
                                        break;
                                }
                                // check for redirections 

                                if (strcmp(argument, "<") == 0) {
                                        argument = strtok(NULL, " ");
                                        input_redirection = argument;
                                        input_redirection_detected = TRUE;
                                } else if (strcmp(argument, ">") == 0) {
                                        argument = strtok(NULL, " ");
                                        output_redirection = argument;
                                        output_redirection_detected = TRUE;
                                } else {
                                        arguments[arguments_count++] = argument;
                                }
                                
                                // NULL to ensure the same string is used and get the next argument
                                argument = strtok(NULL, " ");
                        }
                        //for (i = arguments_count ; i < MAX_NO_OF_ARGS; i++)
                       // {
                        //        arguments[i] = NULL;
                        //}
                        arguments[arguments_count] = NULL;

                        if (stop){
                                fprintf(stderr, "Error: too many process arguments\n");
                                fflush(stdout);
                                fflush(stderr);
                                //exit(1);
                                continue;
                        }
                        if (i < total_commands - 1 && pipe(pipe_fd) == -1) {
                                perror("pipe");
                                exit(1);
                        }

                        
                        // check if the command is empty
                        if (arguments[0] == NULL) {
                                fprintf(stderr, "Error: empty command\n");
                                continue;
                        }

                        //pritn rguments[0]
                        //printf("$$$$$$ arguments[0] %s \n", arguments[0]);
                        //printf("$$$$$$ arguments[1] %s \n", arguments[1]);


                        // handle pwd, cd and exit.
                        if (strcmp(arguments[0], "exit") == 0) {
                                BUILTIN_COMMAND = TRUE;
                                fprintf(stderr, "Bye...\n");
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                        orig_command, 0);
                                fflush(stdout);
                                fflush(stderr);
                                exit(0);
                        }

                        if (strcmp(arguments[0], "cd") == 0) {
                                BUILTIN_COMMAND = TRUE;
                                if (arguments_count < 2) {
                                        fprintf(stderr, "Error: invalid directory\n");
                                } else {

                                        int cd_status = 0;
                                        //int cd_status1 = 0;
                                        if (chdir(arguments[1]) != 0) {
                                                fprintf(stderr, "Error: cannot cd into %s\n", arguments[1]);
                                                //fprintf(stderr, "+ completed '%s' [%d]\n",
                                                //        orig_command, 0);
                                                cd_status = 1;
                                        } else {
                                                cd_status = 0;
                                        } 
                                        
                                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                                orig_command, cd_status);
                                
                                        
                                }
                                continue;
                        }

                        if (strcmp(arguments[0], "pwd") == 0) {
                                BUILTIN_COMMAND = TRUE;

                                char cwd[1024];
                                if (arguments_count > 1 ) {
                                        fprintf(stderr, "Error: invalid directory\n");
                                } else {
                                        if (getcwd(cwd, sizeof(cwd))) {
                                                fprintf(stdout,"%s\n", cwd);
                                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                                        orig_command, 0);
                                        } else {
                                                perror("error : pwd failed");
                                        }
                                }
                                continue;
                        }

                     
                        //creates a pipe only if there’s another command after the current one
                        // and the current command is not the last one
                        if (i < total_commands - 1) {
                                if (pipe(pipe_fd) == -1) {
                                        perror("pipe");
                                        exit(1);
                                }
                        }
                        // Fork and execvp
                        // Fork a child process
                        // to execute the command
                        //printf("forking %s \n", commands[i]);

                        
                        
                        pids[i] = fork();
                        if (pids[i] == 0) {
                                // child
                                if (in_fd) {
                                        // connect the output of the previous command to the input of the current command.
                        
                                        dup2(in_fd, STDIN_FILENO);
                                        close(in_fd);
                                }
                                if (i < total_commands - 1) {

                                        // create pipe if doesnt exist and write
                                        dup2(pipe_fd[1], STDOUT_FILENO);
                                        close(pipe_fd[0]);
                                        close(pipe_fd[1]);
                                }
                                // Input redirection
                                if (input_redirection_detected) {
                                        //input is comoing from a file 
                                        int fd = open(input_redirection, O_RDONLY);
                                        if (fd < 0) { 
                                                perror("open <"); exit(1); 
                                        }
                                        dup2(fd, STDIN_FILENO);
                                        close(fd);
                                } else if (i > 0) {
                                        // input is coming from a previous pipe

                                }
                                

                                if (output_redirection_detected) {
                                        //printf ("$$$$$$$$$$$$$$$$$$$$$\n");
                                        int fd = open(output_redirection, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                        if (fd < 0) { 
                                                perror("open >"); exit(1); 
                                        }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                } else if (i < total_commands - 1) {

                                }

                                
                                execvp(arguments[0], arguments);
                                //printf("FAILED \n");
                                fprintf(stderr, "Error: command not found\n");
                                //fprintf(stderr, "execvp failed: %s\n", strerror(errno));
                                //fprintf(stderr, "Error: command not found\n");
                                exit(1);
 
                        } else if (pids[i] < 0){
                                perror("fork");
                                exit(1);
                        }

                        if(in_fd) close(in_fd);
                        if (i < total_commands - 1) {

                                close(pipe_fd[1]);
                                in_fd = pipe_fd[0];
                        }

                }

                if (BACKGROUND_COMMAND_PRESENT) {
                        //printf("BACKGROUND_COMMAND_PRESENT background pid %d\n", pid);
                        background_pids = pids[0];

                } else {
                        for (int i = 0; i < total_commands; i++) {
                                int status;
                                //printf("waitpid %d \n", pids[i]);
                                waitpid(pids[i], &status, 0);
                                if(WIFEXITED(status)){
                                        //printf("11111 ^^^^^^^^^^^^status %d\n", status);
                                        command_status[i] = WEXITSTATUS(status);
                                } else if (WIFSIGNALED(status)) {
                                        command_status[i] = WTERMSIG(status);;
                                } else {
                                        //printf("5555 ^^^^^^^^^^^^JATAYU %d\n", status);
                                        command_status[i] =  1;
                                }
                               
                
                        }
                }
                if(!BUILTIN_COMMAND){
                        //printf("$$$$$$$$$$$$$$$$$$$$$$$ %d |  %d\n", total_commands , command_status[0]);
                        command_execution_status(orig_command, command_status, total_commands);

                }
        }

        return EXIT_SUCCESS;
}
