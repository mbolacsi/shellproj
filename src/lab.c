// Include statments go brrrrrrr
#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <limits.h>

// Print the version of the shell... because we might need to know in case we accidentally update this thing
void print_version_and_exit() {
    printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
    exit(0);  // Exit the program.... We gotta get outta here
}

// Fetch the magical prompt string which will stare at you with its commanding presence until you type something.
char *get_prompt(const char *env) {
    // Lets check if we've got a fancy custom prompt in the environment... something like "type input here loser"
    char *prompt = getenv(env);
    // If we find it we gonna snatch it otherwise we're stuck with default shell. Womp Womp
    return prompt ? strdup(prompt) : strdup("shell>");
}

// We're going on a trip in our favorite rocket ship 🚀
int change_dir(char **dir) {
    // Tell me where were going or I swear I'll turn this car around!
    const char *target = dir[1] ? dir[1] : getenv("HOME");

    // How dare you tell me to go somewhere that doesn't exist!! Im gonna find out where u live and pay u a visit.
    if (!target) {
        struct passwd *pw = getpwuid(getuid());
        target = pw ? pw->pw_dir : NULL;
    }

    // Ohh, no home directory... Guess you're living on the streets, huh?
    if (!target) {
        fprintf(stderr, "cd: Cannot determine home directory\n");
        return -1;
    }

    // We know where we going! Lets gooooo!
    if (chdir(target) != 0) {
        perror("cd");  // Wait, what do you mean that didn't work? 😡
        return -1;
    }

    // YAY!! We made it to... where exactly? 🤔
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Moved to: %s\n", cwd);
    } else {
        perror("getcwd");  // not again :(
    }

    return 0; 
}

// Break this line into tokens this is my stupid code 🎸
char **cmd_parse(const char *line) {
    // The maximum number of arguments we can have before the system breaks up with us
    int max_args = sysconf(_SC_ARG_MAX);

    // Why must you remember every argument we have :(
    char **args = malloc(max_args * sizeof(char *));
    if (!args) {
        perror("malloc failed");  // Too many arguments... divorce moment
        exit(EXIT_FAILURE);  
    }

    // Why are u copying me???
    int arg_count = 0;
    char *line_copy = strdup(line);
    // Tokenize by splitting it at spaces, tabs, and newlines
    char *token = strtok(line_copy, " \t\n");

    // Just keep splitting just keep splitting what do we do we split split split
    while (token && arg_count < max_args - 1) {
        args[arg_count++] = strdup(token);  // Save each token as an argument
        token = strtok(NULL, " \t\n");  // Get the next token
    }

    args[arg_count] = NULL;  // Null-terminate the argument list
    free(line_copy); 

    return args;  
}

// Set the arguments free! They must escape
void cmd_free(char **line) {
    if (line) {
        for (int i = 0; line[i]; i++) {
            free(line[i]);  // Free each individual argument
        }
        free(line);  // Free the array of arguments
    }
}

// We got whitespace... Ain't nobody got time for that.
char *trim_white(char *line) {
    // Don't be leading me on with this whitespace.
    while (isspace((unsigned char)*line)) {
        line++;
    }

    // Find the last non-whitespace character... We must kill everything after 🔪
    char *end = line + strlen(line) - 1;
    // Move backward to remove any trailing whitespace
    while (end > line && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';  // Terminate the string at the last non-whitespace character
    return line;  
}

// Executes built-in commands like exit, cd, and history
bool do_builtin(struct shell *sh, char **argv) {
    // No command? Not sure what we're doing here then 🤔... Just say false i guess
    if (!argv || !argv[0]) return false;

    // Goodbye cruel shell
    if (strcmp(argv[0], "exit") == 0) {
        sh_destroy(sh);  
        exit(0); 
    }
    // To Narnia!
    else if (strcmp(argv[0], "cd") == 0) {
        return change_dir(argv) == 0;
    }
     // Let’s look at all our mistakes 
    else if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            // Print each entry in the list
            for (int i = 0; hist_list[i]; i++) {
                printf("%d  %s\n", i + 1, hist_list[i]->line);
            }
        } else {
            printf("No history available.\n");   // Wait, we got no past? 😱
        }
        return true;
    } else {
        // Look at all the 3 things you can do!!!!
        printf("Unknown command: %s\n", argv[0]);
        printf("Available commands:\n");
        printf("  exit    - Exit the shell\n");
        printf("  cd      - Change the current directory\n");
        printf("  clear      - Clear the terminal\n");
        printf("  history - Display the command history\n");
        return false;
    }
}

// Initialize the shell: set prompt, configure terminal settings, and handle signals
void sh_init(struct shell *sh) {
    // Lets check them environment variables to see if u get cool custom prompt or lame shell
    sh->prompt = get_prompt("MY_PROMPT");
    if (!sh->prompt) sh->prompt = get_prompt("PS1");
    if (!sh->prompt) sh->prompt = strdup("shell>");  // Default 

    sh->shell_terminal = STDIN_FILENO;  // Use standard input as the terminal
    sh->shell_pgid = getpid();  // Set the shell’s process group ID to its PID

    // Set the shell’s process group to allow it to handle signals
    setpgid(sh->shell_pgid, sh->shell_pgid);
    tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

    // Ignore all these annoying signals
    signal(SIGINT, SIG_IGN);  // Ctrl+C? Nah, we're too cool for that
    signal(SIGQUIT, SIG_IGN); // Quit? Nah, we never quit
    signal(SIGTSTP, SIG_IGN); // Stop? More like "keep going" 😤
    signal(SIGTTIN, SIG_IGN); // Who needs TTY input? Not us
    signal(SIGTTOU, SIG_IGN); // TTY output? Who even does that anymore?
}

// Break up with the shell. It's not us, it's you... 😢
void sh_destroy(struct shell *sh) {
    // If shell structure exists and has a prompt, free the allocated memory and set prompt to null.
    if (sh && sh->prompt) {
        free(sh->prompt);
        sh->prompt = NULL; 
    }
}

// Parse command-line arguments... All 1 of them!!! YAY!
void parse_args(int argc, char **argv) {
    int opt;
    // I sure hope they only wanna know the version 🤞
    while ((opt = getopt(argc, argv, "v")) != -1) {
        if (opt == 'v') {
            print_version_and_exit();  
        } else {
            // Wait, what’s this? You gave us a mystery option? We don’t do mysteries here.
            fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
            exit(EXIT_FAILURE);  // epic fail
        }
    }
}
