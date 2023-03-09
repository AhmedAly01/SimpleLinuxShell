#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <time.h>
#define MAXSIZE 1000
#define LOG_FILE "/home/ubuntu/projects/OS labs/OS lab 1/log.txt"

//function to initiate a new session in the log file
void init_file(){
    FILE *fptr;
    fptr = fopen(LOG_FILE, "a");
    fprintf(fptr, "██████████████████████ New Session ██████████████████████\n\n");
    fprintf(fptr, "PID\t\t\tTime of execution\t\tEvent\n\n");
    fclose(fptr);
}

//function that takes in command information to write it into the log file
void write_to_log(char* command, int PID, time_t time){
    FILE *fptr;
    //formatting the time
    int sec, min, hour;
    hour = (time / 3600);
    min = (time - (3600 * hour)) / 60;
    sec = (time - (3600 * hour) - (min * 60));
    hour = (hour % 24) + 2;
    //writing to the file
    fptr = fopen(LOG_FILE, "a");
    fprintf(fptr, "%d\t\t%.2d:%.2d:%.2d\t\t\t\t%s\n\n", PID, hour, min, sec, command);
    fclose(fptr);
}

//This function makes input_type = 1 if the command is built-in, 0 if the command is executable
void evaluate_command(char* command, int* input_type){
    if (!strncmp(command, "cd", 2) || !strncmp(command, "echo", 4) || !strncmp(command, "export", 6)) {
        *input_type = 1;
    }
    else {
        *input_type = 0;
    }
}

//function to parse command and execute cd
void execute_cd(char* command){
    char dir[MAXSIZE] = {0};
    for (int i = 3; i < strlen(command); i++) {
        dir[i-3] = command[i];
    }
    if (!strcmp(dir, "..")) chdir(dir);
    else if (!strcmp(dir, "~") || strlen(dir) == 0) chdir(getenv("HOME"));
    else if (chdir(dir)){
        perror("Error");
    }
}

//function to parse command and execute echo
void execute_echo(char* command){
    system(command);
}

//function to parse command and execute export
void execute_export(char* command) {
    char variable[MAXSIZE] = {0};
    char value[MAXSIZE] = {0};
    int isLHS = 1;
    int isQuotations = 0;
    for (int i = 0; i < strlen(command) - 7; i++) {
        if (command[i+7] == '=') isLHS = 0;
        else if (command[i+7] == '\"') {
            if (command[strlen(command)-1] == '\"') isQuotations = 1;
            else printf("Invalid command: Improper quotations. \n");
        }
        else if (isLHS) {
            variable[i] = command[i + 7];
        }
        else if (!isLHS && isQuotations) {
            value[i - strlen(variable) - 2] = command[i + 7];
        }
        else {
            if (command[i+7] == ' ') break;
            else value[i - strlen(variable) - 1] = command[i + 7];
        }
    }
    setenv(variable, value, 1);
}

//function to direct flow of execution of the built-in commands
void execute_builtin(char* command) {
    write_to_log(command, getpid(), time(0));

    //handling the cd command
    if (!strncmp(command, "cd", 2)) {
        execute_cd(command);
    }
    
    //handling the echo command
    else if (!strncmp(command, "echo", 4)){
        execute_echo(command);
    }

    //handling the export command
    else if (!strncmp(command, "export", 6)) {
        execute_export(command);
    }

    //in case of error
    else {
        printf("invalid command\n\n");
    }
}

//function to execute non-built-in command
void execute_command(char* command) {
    pid_t child_pid = fork();
    if (child_pid < 0) perror("Error");
    else if (child_pid == 0) {
        //saving a temporary copy of the command
        char temp[MAXSIZE] = {0};
        for (int i = 0; i < strlen(command); i++) {
            temp[i] = command[i];
        }

        //tokenize the command
        int i = 0;
        char *args[MAXSIZE];
        char * token = strtok(command, " ");
        while( token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        //executing the command
        write_to_log(temp, getpid(), time(0));
        if ((args[1] == NULL || strncmp(args[1], "$", 1)) && strcmp(args[i-1], "&")) {
            execvp(args[0], args);
            perror("Error");  
        }
        else if (!strcmp(args[i-1], "&")) {
            args[i-1] = NULL;
            execvp(args[0], args);
            perror("Error");
        }
        else system(temp);
        exit(0);
    }
    else {
        //the parent waits for child to finish then takes over
        int status;
        if (command[strlen(command)-1] != '&') {
            waitpid(-1, &status, 0);
            write_to_log("Child Terminated", child_pid, time(0));
        }
        else waitpid(-1, &status, WNOHANG);
    }
}

//function that contains the shell running loop and input scanning
void run_shell() {
    char command[MAXSIZE] = {0};
    while (1) {
        //getting current working directory
        char cwd[MAXSIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("\033[34;1m%s > \033[0m", cwd);
        else perror("Error");
        //Read the command
        scanf("%[^\n]%*c", command);
        //Check if command is exit
        if (!strcmp(command, "exit")) {
            FILE *fptr;
            fptr = fopen(LOG_FILE, "a");
            fprintf(fptr, "██████████████████████ End Session ██████████████████████\n\n\n\n");
            fclose(fptr);
            exit(0);
        }
        //if command is not exit then we parse the command
        else {
            int input_type = 0;
            evaluate_command(command, &input_type);
            //in case of a Built-in command
            if (input_type) execute_builtin(command);
            //in case of non-Built-in commands
            else execute_command(command);
        }
    }
}

void on_child_exit() {
	int wstat;
	pid_t pid;

	while (1) {
		pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL);
		if (pid == 0) return;
		else if (pid == -1) return;
        else write_to_log("Child Terminated", pid, time(0));
	}
}

int main(){
    //registering child signal
    signal(SIGCHLD, on_child_exit);
    
    //initiate a new session in the log file
    init_file();

    //run the shell
    run_shell();

    return 0;
}