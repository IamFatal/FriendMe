#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "friends.h"
#include "friends_server.h"

#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \n"


/*
 * Find a client with the given name. Return NULL if no such client exists.
 */
Client *find_client(char *name, Client **top) {
    
    // find client with given fd
    Client **client = top;
    while (*client != NULL) {
        if (strcmp((*client)->name, name) == 0) {   // name matches
            return *client;                         // return client
        }
        client = &(*client)->next;
    }
    
    return NULL;
}


/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv) {
    int cmd_argc = 0;
    char *next_token = strtok(cmd, DELIM);    
    while (next_token != NULL) {
        if (cmd_argc >= INPUT_ARG_MAX_NUM - 1) {
            error("Too many arguments!", STDOUT_FILENO);
            cmd_argc = 0;
            break;
        }
        cmd_argv[cmd_argc] = next_token;
        cmd_argc++;
        next_token = strtok(NULL, DELIM);
    }

    return cmd_argc;
}


/* 
 * Read and process commands
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, 
        Client *client, Client **top) {
    User *user_list = *user_list_ptr;

    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
        return -1;

    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
        char *buf = list_users(user_list);
        write(client->fd, buf, strlen(buf));
        free(buf);

    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
        switch (make_friends(client->name, cmd_argv[1], user_list)) {
            case 0:
            {
                Client *other = find_client(cmd_argv[1], top);
                char buf[100];
                sprintf(buf, "You are now friends with %s.\r\n", cmd_argv[1]);
                write(client->fd, buf, strlen(buf));
                if (other) {
                    sprintf(buf, "%s has added you as a friend.\r\n> ", 
                        client->name);
                    write(other->fd, buf, strlen(buf));
                }
            }    
                break;
            case 1:
                error("you are already friends", client->fd);
                break;
            case 2:
                error("at least one of you has the max number of friends", 
                    client->fd);
                break;
            case 3:
                error("you cannot befriend yourself", client->fd);
                break;
            case 4:
                error("the user you entered does not exist", client->fd);
                break;
        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 3) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 2; i < cmd_argc; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(client->name, user_list);
        User *target = find_user(cmd_argv[1], user_list);
        switch (make_post(author, target, contents)) {
            case 0:
            {
                Client *other = find_client(cmd_argv[1], top);
                if (other) {
                    char buf[INPUT_BUFFER_SIZE];
                    sprintf(buf, "%s says: %s\r\n> ", client->name, contents);
                    write(other->fd, buf, strlen(buf));
                }
            }    
                break;
            case 1:
                error("you are not friends with this user", client->fd);
                break;
            case 2:
                error("the user you entered does not exist", client->fd);
                break;
        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {
        User *user = find_user(cmd_argv[1], user_list);
        if (user == NULL) {
            error("user not found", client->fd);
        } else {
            char *buf = print_user(user);
            write(client->fd, buf, strlen(buf));
            free(buf);
        }
    } else {
        error("Incorrect syntax", client->fd);
    }
    
    return 0;
}