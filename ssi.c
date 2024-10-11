/*
 * Simple Shell Interpreter
 *
 * This program implements a simple shell interpreter that can execute commands
 * and change directories. It also supports running commands in the background
 * and listing background processes.
 *
 * Author: hannahwangmb@uvic.ca
 * V01015199
 * Date: 2023/09/30
 *
 * Usage: make
 *       ./ssi
 *
 * Commands:
 *  - exit: exit the shell
 *  - cd: change directory
 *  - bg: run a command in the background
 *  - bglist: list all background processes
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

struct background_process
{
    pid_t pid;
    char command[1024];
};

struct background_process *bg_processes = NULL;
int num_bg_processes = 0;

// Display the prompt in the format: username@hostname: cwd >
void display_prompt() {
    char hostname[100];
    char username[100];
    gethostname(hostname, sizeof(hostname));
    getlogin_r(username, sizeof(username));
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s@%s: %s > ", username, hostname, cwd);
}

// Add a background process to the list of background processes
void add_bg_process(pid_t pid, char *command) {
    num_bg_processes++;
    bg_processes = realloc(bg_processes, num_bg_processes * sizeof(struct background_process));

    // Add the new background process to the end of the list
    if (bg_processes != NULL) {
        bg_processes[num_bg_processes - 1].pid = pid;
        strncpy(bg_processes[num_bg_processes - 1].command, command, sizeof(bg_processes[num_bg_processes - 1].command));
        bg_processes[num_bg_processes - 1].command[sizeof(bg_processes[num_bg_processes - 1].command) - 1] = '\0';

    }
    else{
        fprintf(stderr, "Error allocating memory for background processes\n");
        exit(EXIT_FAILURE);
    }
}

// Check the status of background processes
void bg_process_status() {
    int i;
    for (i = 0; i < num_bg_processes; i++) {
        pid_t pid = waitpid(bg_processes[i].pid, NULL, WNOHANG);
        if (pid > 0){
            // Process has terminated if waitpid returns a positive value
            printf("%d: %s has terminated.\n", bg_processes[i].pid, bg_processes[i].command);
            for (int j = i; j < num_bg_processes - 1; j++) {
                bg_processes[j] = bg_processes[j + 1];
            }
            num_bg_processes--;
            i--;
        }
    }

}

int main() {
    // Loop until user enters "exit"
    while (1) {
        char input[1024];
        display_prompt();

        fgets(input, sizeof(input), stdin);

        // Remove trailing newline included by fgets
        input[strcspn(input, "\n")] = 0;

        char *args[100];
        char *token = strtok(input, " \n");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " \n");
        }
        args[i] = NULL;

        // Check if user entered a command
        if (i > 0) {
            if (strcmp(args[0], "exit") == 0) {
                break;
            }
            else if (strcmp(args[0], "cd") == 0) {
                // Change directory
                if (i > 1) {
                    if (strcmp(args[1], "~") == 0) {
                        // Change to home directory
                        char *home_dir = getenv("HOME");
                        if (home_dir != NULL) {
                            if (chdir(home_dir) != 0) {
                                perror("cd");
                            }
                        }
                        else {
                            fprintf(stderr, "cd: HOME environment variable not set\n");
                        }
                    }
                    else if (args[1][0] == '~' && args[1][1]== '/'){
                        // handling cd ~/path
                        char *home_dir = getenv("HOME");
                        if (home_dir != NULL) {
                            char new_path[1024];
                            snprintf(new_path, sizeof(new_path), "%s%s", home_dir, args[1] + 1);
                            new_path[sizeof(new_path)-1]='\0';
                            if (chdir(new_path) != 0) {
                                perror("cd");
                            }
                        }
                        else {
                            fprintf(stderr, "cd: HOME environment variable not set\n");
                        }

                    }
                    else {
                        // Change to specified directory
                        if(chdir(args[1]) != 0) {
                        perror("cd");
                        }
                    }
                }
                else{
                    // No argument given, go to home directory
                    char *home_dir = getenv("HOME");
                    if (home_dir != NULL) {
                        if (chdir(home_dir) != 0) {
                            perror("cd");
                        }
                    } else {
                        fprintf(stderr, "cd: HOME environment variable not set\n");
                    }

                }
            }
            else if (strcmp(args[0], "bg")==0){
                // background process
                if ( i > 1){
                    pid_t pid = fork();
                    if (pid == 0) {
                        // Child process
                        execvp(args[1], args+1);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                    else if (pid < 0) {
                        perror("fork");
                    }
                    else {
                        // Parent process
                        add_bg_process(pid, args[1]);
                    }
                }
                else{
                    fprintf(stderr, "bg: No command given\n");
                }
            }

            else if (strcmp(args[0], "bglist")==0){
                // listing background processes
                for (int i = 0; i < num_bg_processes; i++) {
                    printf("%d: %s\n", bg_processes[i].pid, bg_processes[i].command);
                }
                printf("Total background jobs: %d\n", num_bg_processes);
            }
            else {
                // Execute other command
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    execvp(args[0], args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
                else if (pid < 0) {
                    perror("fork");
                }
                else {
                    // Parent process
                    wait(NULL);
                }
            }
        }
        bg_process_status();

    }
    if(bg_processes != NULL){
        free(bg_processes);
    }

    return 0;
}

