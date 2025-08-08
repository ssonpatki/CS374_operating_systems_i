// Initial program created with pseudocode provided 
// 	in the assignment instructions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>



// GLOBAL VARIABLES
char command[3001]; // extra characters to account for PID expansion
int status;
volatile sig_atomic_t run_mode;   // 0 if run in foreground, 1 if run in background
struct sigaction sigint_action = {0};
struct sigaction sigtstp_action = {0};
pid_t fork_result;
int fork_pids[3001];
int number_forks;
int commentCommand;

// struct to help organize user's command
struct commandString {
    char *command;
    // at most 20 arguments 
    char *arguments[20];
    char *input_file;
    char *output_file;
    int run_background;
};

// Function used to free dynamically allocated memory
void free_command(struct commandString* current) {
    if (current != NULL) {
        free(current->command);

        // loop needed to array of char pointers
        for (int i = 0; i < 20; i++) {
            free(current->arguments[i]);
        }

        free(current->input_file);
        free(current->output_file);
        
        free(current);
    }
}

// Function used to populate struct instance with line from user input
struct commandString *read_command(char* user_input, ssize_t line_length) {
	struct commandString *c = malloc(sizeof(struct commandString));
    
    // check that c is not NULL before populating it
    if (c == NULL) {
        return NULL;
    }

    // pre-populate memory associated with pointers in struct to NULL
    // [how to use memset] https://www.geeksforgeeks.org/memset-c-example/
    memset(c, 0, sizeof(struct commandString));

    // clean up trailing new-line character 
    user_input[line_length - 1] = '\0';
    line_length--;
    
    // tokenize the line and use it to seperate attribute data
    char* token = strtok(user_input, " ");
    int index = 0;

    // while token has not reached the end (i.e., there is still data to be read)
    while (token != NULL) {

        // if characters in token equals < (i.e., an input file was supplied)
        if(strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                // stores input_file supplied from user input (optional)
                c->input_file = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(c->input_file, token);
            }
        
        // else if characters in token equals > (i.e., an output file was supplied)
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                // stores output_file supplied from user input (optional)
                c->output_file = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(c->output_file, token);
            }
        
        // else if characters in token equals & (i.e., user wants to run command in background)
        } else if (strcmp(token, "&") == 0){
            c->run_background = 1;

        // if no <[input_file], >[output_file], or & was supplied in command line 
        } else {

            // check that command is NULL
            if (c->command == NULL) {
                // stores command supplied from user input (required)
                c->command = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(c->command, token);

            // check that max number of arguments hasnt already been populated
            } else if (index < 20) {
                // stores arguments supplied from user input (optional)
                c->arguments[index] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                strcpy(c->arguments[index], token);
                index++;
            }
        }

        // get next token in command string
        token = strtok(NULL, " ");
    }

    // set the unused c->arguments elements to null
    // (; index < 20; index++) = increment index from last known value till < 20
    for (; index < 20; index++) {
        c->arguments[index] = NULL;
    }
	
	return c;
}

// Function to print struct - used for debugging
void printCommand(struct commandString *current) {
    if (current != NULL && current->command != NULL) {
        printf("%s ", current->command);

        for (int i = 0; i < 20; i++) {
            if (current->arguments[i] != NULL) {
                printf("%s ", current->arguments[i]);
            }
        }

        if (current->input_file != NULL) {
            printf("< %s", current->input_file);
        }

        if (current->output_file != NULL) {
            printf("> %s ", current->output_file);
        }

        if (current->run_background == 1) {
            printf("&");
        }

        printf("\n");
    }
}

/*
    FUNCTIONS FOR BUILT-IN COMMANDS (excluding exit)
*/

void exitCmd() {
    // send kill signal to process(es)
    // int kill(pid_t pid, int sig);

    for (int i = 0; i < number_forks; i++) {
        kill(fork_pids[i], SIGKILL);
    }
}


// change working directory
void cdCmd(struct commandString *userCommand) {
    // get the path stored in the HOME environment variable
    char *name = "HOME";  // [CHECK] should this be "HOME" or "./" 
    char *curr_dir;
    curr_dir = getenv(name);
    int change_dir = -1;

    // change current working directory of program
    if (userCommand->arguments[0] == NULL) {
        // no new path was given so reapply current directory
        change_dir = chdir(curr_dir);
    } else {
        // change path to the one that was given
        change_dir = chdir(userCommand->arguments[0]);
    }
   
    // [DEBUGGING]
    if (change_dir == 0) { 
        char new_path[2048];
        getcwd(new_path, 2048);
        //printf("New Working Directory: %s\n", new_path);
    } else {
        printf("Error while attempting to change directory");
    }
}


// print the exit code (exit status) or terminating signal 
// of the most recently executed foreground command
void statusCmd() {
    int exit;
    int signal;

    if (WIFEXITED(status)) {    // check that process was exited normally
        exit = WEXITSTATUS(status); // return exit code specified by process
        printf("exit value %d\n", exit);
    } else {
        signal = WTERMSIG(status); // get signal number that caused process to terminate  
        printf("terminated by signal %d\n", signal);
    }
    
}



// revise fork_pids array when pid is removed
void fix_pid_array(pid_t pid_to_delete) {
    for (int i = 0; i < number_forks; i++) {
        // find specific pid in array
        if (fork_pids[i] == pid_to_delete) {
            // shift array to "delete" pid
            for (int j = i; j < number_forks - 1; j++) {
                fork_pids[j] = fork_pids[j + 1];
            }
            number_forks--;
            break;
        }
    }
}

// function to check termination of child background processes
void check_background() {
    int child_status;
    pid_t pid;

    // look for any completed child processes
    while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0) {
        if (WIFEXITED(child_status)) {
            printf("background pid is %d is done: exit value is %d\n", pid, WEXITSTATUS(child_status));
        } else if (WIFSIGNALED(child_status)) {
            printf("background pid is %d is done: terminated by signal %d\n", pid, WTERMSIG(child_status));
        }
        fflush(stdout);  // display output immediately
        fix_pid_array(pid);
    } 

}


/*
    FUNCTION TO HANDLE NON-BUILT-IN COMMANDS
*/

void non_built_in(struct commandString *userCommand) {
    fork_result = fork();

    if (fork_result == -1) {
        printf("Creation of child process was unsuccessful.");

// the following section of the if-statement handles the child process
    } else if (fork_result == 0) {
        //printf("This child process was created");

        // create an argv for the second argument of execvp 
        // combines command and arguments from struct
        char *argv[22]; 
        argv[0] = userCommand->command;
        for (int i = 0; i < 20; i++) {
            if (userCommand->arguments[i] != NULL) {
                argv[i+1] = userCommand->arguments[i];
            } else {
                argv[i+1] = NULL; 
            }
        }

        // if child should run in the foreground
        if (userCommand->run_background == 0) {
            struct sigaction child_sigint_action = {0};
            child_sigint_action.sa_handler = SIG_DFL;
            sigfillset(&(child_sigint_action.sa_mask));
            child_sigint_action.sa_flags = 0; 
            sigaction(SIGINT, &child_sigint_action, NULL);

            
        }

        // ignore SIGTSTP
        struct sigaction child_sigtstp_action = {0};
        child_sigtstp_action.sa_handler = SIG_IGN;    // if sigtstp occurs, ignore it 
        sigaction(SIGTSTP, &child_sigtstp_action, NULL); // catch SIGTSTP (CTRL+Z)

        // if input file was specified by user command
        if (userCommand->input_file != NULL) {
            int in_success = open(userCommand->input_file, O_RDONLY);   // open file for reading only
            if (in_success == -1) {
                printf("%s: No such file or directory\n", userCommand->input_file);
                exit(1);
            }

            // redirects file descriptor 0 (stdin) to point to arg[0] in dup2()
            int result = dup2(in_success, 0);

            if (result == -1) {
                printf("Error occured during dup2 redirection\n");
                exit(1);
            } 
        } else if (userCommand->run_background == 1) {
            int in_success = open("/dev/null", O_RDONLY);   // open file for reading only
            if (in_success == -1) {
                printf("%s: No such file or directory\n", userCommand->input_file);
                exit(1);
            }

            // redirects file descriptor 0 (stdin) to point to arg[0] in dup2()
            int result = dup2(in_success, 0);

            if (result == -1) {
                printf("Error occured during dup2 redirection\n");
                exit(1);
            } 
        }


        // if output file was specified by user command
        if (userCommand->output_file != NULL) {
            int out_success = open(userCommand->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);   // open file for reading only
            if (out_success == -1) {
                printf("Error while opening %s\n", userCommand->output_file);
                exit(1);
            }

            // redirects file descriptor 1 (stdout) to point to arg[0] in dup2()
            int result = dup2(out_success, 1); 

            if (result == -1) {
                printf("Error occured during dup2 redirection\n");
                exit(1);
            } 
        } else if (userCommand->run_background == 1) {
            int out_success = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0777);   // open file for reading only
            if (out_success == -1) {
                printf("Error while opening %s\n", userCommand->output_file);
                exit(1);
            }

            // redirects file descriptor 1 (stdout) to point to arg[0] in dup2()
            int result = dup2(out_success, 1); 

            if (result == -1) {
                printf("Error occured during dup2 redirection\n");
                exit(1);
            } 
        }

        execvp(userCommand->command, argv);

        // if execvp returns than the process failed
        printf("%s: command not found\n", userCommand->command);
        exit(1);    // exit child if execvp fails

// the following section of the if-statement handles the parent process
    } else if (fork_result > 0) {
        //printf("This is the parent process");

        // check if process should run in foreground (run_background == 0)
        if (userCommand->run_background == 0) {
            // return status information -- wait until process is done since run in foreground
            waitpid(fork_result, &status, 0); 

            if (WIFSIGNALED(status)) {
                printf("terminated by signal %d\n", WTERMSIG(status));
                fflush(stdout);
            }
            
        } else {
            // return status information on process even if not ended (i.e., continues to run in background) 
            fork_pids[number_forks++] = fork_result; // keep track of child processes pid
            printf("background pid is %d\n", fork_result);
 
        }
    }
}


// function figures out the expected response of the command string
void built_in(struct commandString *userCommand) {
    char *command = userCommand->command;

    if (strcmp(command, "exit") == 0) {
        exitCmd();
    } else if (strcmp(command, "cd") == 0) {
        cdCmd(userCommand);
    } else if (strcmp(command, "status") == 0) {
        statusCmd();
    }
}


// checks command for any comments (# or //)
void determine_action(struct commandString *userCommand) {
    // switch run_mode from background if in foreground only mode
    if (run_mode == 1) {
        userCommand->run_background = 0;
    }

    char *command = userCommand->command;
    int background = userCommand->run_background;

    if (strcmp(command, "exit") == 0 || strcmp(command, "cd") == 0 || strcmp(command, "status") == 0) {
        built_in(userCommand);
        return;
    } else {
        non_built_in(userCommand);
    }
}

// checks command for any comments (# or //)
void checkCmd(char *userCommand) {
    char expandedCommand[3001];
    char pid_string[100];
    sprintf(pid_string, "%d", getpid()); // store smallsh pid in pid_string
    int len_pid = strlen(pid_string);

    int og_index = 0;
    int new_index = 0;

    // check user input command (stored as an global char array) for any comment characters
    while (command[og_index] != '\0' && new_index < 3000) {

        if (command[og_index] == '$' && command[og_index+1] == '$') {
            //replace $$ with PID of smallsh itself
            // getpid() to retrieve the PID of the current process
            int j = 0;
            while (pid_string[j] != '\0' && new_index < 3000) {
                expandedCommand[new_index++] = pid_string[j++];
            }
            og_index += 2;    // skip both $
        } else if (command[og_index] == '#' ||
                    (command[og_index] == '/' && command[og_index+1] == '/'))  {
            commentCommand = 1;
            return;
        } else {
            expandedCommand[new_index++] = command[og_index++];
        }

        expandedCommand[new_index] = '\0';  // end expanded command with null terminator
    }

    if (commentCommand == 1) {
        return;
    } else {
        strncpy(command, expandedCommand, 3000);
        command[3000] = '\0';
    }
    
}


/*
    FUNCTION TO HANDLE SIGNALS
*/

void SIGTSTP_handler(int signal_number) {
    if (run_mode == 0) {
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";

        // STDOUT_FILENO is the file descriptor of standard output (i.e., 1)
        write(STDOUT_FILENO, message, 50);
        run_mode = 1;
        // Sleep for 5 seconds, then terminate handler, unblocking 
        sleep(5);
    } else {
        char* message = "\nExiting foreground-only mode\n";

        // STDOUT_FILENO is the file descriptor of standard output (i.e., 1)
        write(STDOUT_FILENO, message, 30);
        run_mode = 0;
        // Sleep for 5 seconds, then terminate handler, unblocking 
        sleep(5);
    }
}


int main() {
	// ignore SIGINT (CTRL+C) signals
    sigint_action.sa_handler = SIG_IGN;  // if sigint occurs, handle with sig_ign
    sigfillset(&(sigint_action.sa_mask)); // block all other signals during handling
    sigint_action.sa_flags = 0;    // specifies that CTRL+C doesn't need special handling
    sigaction(SIGINT, &sigint_action, NULL);   // catch SIGINT (CTRL+C)

    // handle SIGTSTP (CTRL+Z)
    sigtstp_action.sa_handler = SIGTSTP_handler;    // if sigtstp occurs, handle with sigtstp_handler
    sigfillset(&(sigtstp_action.sa_mask));
    sigtstp_action.sa_flags = SA_RESTART;   // restart functions if interrupted by handler
    sigaction(SIGTSTP, &sigtstp_action, NULL); // catch SIGTSTP (CTRL+Z)


    char builtIn[256] = "";  // stores users 'command' - initialized as empty string
    status = 0; // set status default to 0 before any foreground commands

    // check that user has not supplied argument to exit program
    while(strcmp(builtIn,"exit") != 0) {
        commentCommand = 0;

        check_background();

        // print colon and flush line buffer
        printf(":");
        fflush(stdout);

        // get line from standard input
        char* user_input = NULL;
	    size_t buffer_size = 0;
	    ssize_t line_length = getline(&user_input, &buffer_size, stdin);
        //printf("You typed: %s\n", user_input);  //[DEBUG]

        // check to see if line is an empty return
        if (line_length != -1) {
            if (user_input[0] == '\n' || user_input[0] =='\0') {
                continue;
            }
        } else {
            perror("getline");
        }

        strcpy(command, user_input);    // copy getline data into global variable

        // checks for comments or pid expansion before executing command
        checkCmd(user_input);

        if (commentCommand == 1) {
            continue;
        }

        char command_copy[3001]; // made copy so global var doesnt get changed
        strncpy(command_copy, command, 3000);
        command_copy[3000] = '\0';


        // create new struct to store command read by getline 
        struct commandString *new_command = read_command(command_copy, strlen(command_copy));

        if (new_command == NULL) {
            continue;
        }

        // [DEBUGGING]
        //printf("Command line is: \n");
        //printCommand(new_command);

        // update variable builtIn based on last read user command
        strcpy(builtIn, new_command->command);

        determine_action(new_command);

        check_background();


        // free memory
	    free(user_input);
        free_command(new_command);
    }


    return(0);
}
