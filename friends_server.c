/*
 * FriendMe (Network Version)
 *   
 * Taken/Modified from Alan J Rosenthal's server, muffinman.c:
 *      - methods add_client, remove_client, new_connection
 *      - linked list of clients (added name member)
 *
 * Shray Sharma, Saman Motamed, April 2016
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>

#include "friends.h"
#include "friends_server.h"

#define INPUT_BUFFER_SIZE 256
#define INPUT_ARG_MAX_NUM 12
#define DELIM " \n"

#ifndef PORT
  #define PORT 50472
#endif


// create the head of the empty client linked list
Client *top = NULL;
int num_clients = 0;

char prompt[] = 
    "\r\nWelcome to FriendMe!"
    "\r\n------------------------------"
    "\r\nPlease enter your username: ";

    
/*
 * Setup socket and return the file descriptor for listening.
 */
int setup() {
    int on = 1, status;
    struct sockaddr_in self;
  
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the server terminates.
    status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                      (const char *) &on, sizeof(on));
    if(status == -1) {
        perror("setsockopt -- REUSEADDR");
    }

    self.sin_family = AF_INET;
    self.sin_addr.s_addr = INADDR_ANY;
    self.sin_port = htons(PORT);
    memset(&self.sin_zero, 0, sizeof(self.sin_zero)); // Initialize sin_zero to 0

    if (bind(listenfd, (struct sockaddr *)&self, sizeof(self)) == -1) {
        perror("bind"); // probably means port is in use
        exit(1);
    }

    printf("Server started: Listening on port %d\n", PORT);
    
    if (listen(listenfd, 5) == -1) {
        perror("listen");
        exit(1);
    }
  
    return listenfd;
}


int main() {
    // Create the head of the empty user linked list
    User *user_list = NULL;
    
    Client *client;
    int listenfd = setup(); // setup socket and get listenfd
    
    while (1) {
        
        // initialize fd set, add listen fd
        fd_set fdlist;
        int maxfd = listenfd;
        FD_ZERO(&fdlist);
        FD_SET(listenfd, &fdlist);
        
        // add fd's of all clients to set, find max fd
        client = top;
        while (client) {
            FD_SET(client->fd, &fdlist);
            if (client->fd > maxfd)
                maxfd = client->fd;
            client = client->next;
        }
        
        if (select(maxfd + 1, &fdlist, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }
        
        // check fds of clients, read if set
        client = top;
        while (client) {
            if (FD_ISSET(client->fd, &fdlist)) {
                client = get_args(client, &user_list);
            } else {
                client = client->next;
            }
        }
            
        // if listenfd is set, accept new connection
        if (FD_ISSET(listenfd, &fdlist))
            new_connection(listenfd);
    }
    
    return 0;
}


/*
 * Search the first inbuf characters of buf for a network newline ("\r\n").
 * Return the location of the '\r' if the network newline is found,
 * or -1 otherwise.
 */
int find_network_newline(const char *buf, int inbuf) {
    for (int i = 0; i < inbuf; i++) {
        if (buf[i] == '\r') {
            if (buf[i + 1] == '\n') {
                return i;   // return the location of '\r' if found
            }
        }
        else if (buf[i] == '\n') {
            return i;
        }
    }

    return -1;  // network newline not found
}


/*
 * Accept the new connection, create a new client, and ask for a username.
 */
void new_connection(int listenfd) {
    int fd;
    struct sockaddr_in peer;
    socklen_t socklen = sizeof(peer);

    if ((fd = accept(listenfd, (struct sockaddr *)&peer, &socklen)) < 0) {
        perror("accept");
        exit(1);
    } else {
        printf("Accepting connection from %s\n", inet_ntoa(peer.sin_addr));
        Client *client = add_client(fd, peer.sin_addr);
        write(client->fd, prompt, sizeof(prompt) - 1);
    }
}


/*
 * Create a new client and insert it at the head of the client's list.
 */
Client *add_client(int fd, struct in_addr addr) {
    Client *new_client = malloc(sizeof(Client));
    if (!new_client) {
        perror("malloc");
        exit(1);
    }
    
    printf("Connection established with %s\n", inet_ntoa(addr));
    fflush(stdout);
    
    // initialize name as empty
    for (int i = 0; i < MAX_NAME; i++) {
        new_client->name[i] = '\0';
    }
    
    // initialize buffer as empty
    for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
        new_client->buf[i] = '\0';
    }
    
    new_client->fd = fd;
    new_client->inbuf = 0;
    new_client->room = 0;
    new_client->after = new_client->buf;
    new_client->where = 0;
    new_client->ipaddr = addr;
    new_client->next = top;
    top = new_client;
    num_clients++;
    
    return new_client;
}


/*
 * Remove client associated with the given file descriptor from linked list, 
 * free all allocated memory, and close the file descriptor.
 */
void remove_client(int fd) {
    Client **client;
    
    // find client with given fd
    for (client = &top; *client && (*client)->fd != fd;
        client = &(*client)->next);

    // if fd was found, remove client from list, free memory, and close fd
    if (*client) {
        Client *temp = (*client)->next;
        if ((close(fd)) == -1) {
            perror("close");
        }
        free(*client);
        *client = temp;
        num_clients--;
    } else {
        fprintf(stderr, "Error: client with fd %d not found\n", fd);
        fflush(stderr);
    }
}


/*
 * Read and process input from client's fd. Return the next client in list.
 */
Client *get_args(Client *client, User **user_list_ptr) {
    int nbytes;
    Client *next = client->next;
    
    nbytes = read(client->fd, client->after, client->room);
    if (nbytes == -1) {
        perror("read");
        exit(1);
    }

    // update inbuf with nbytes, look for network newline
    client->inbuf += nbytes;
    client->where = find_network_newline(client->buf, client->inbuf);
        
    if (client->where >= 0) {   // detected a new line 
            
        // null terminate the line
        client->buf[client->where] = '\0';
        client->buf[client->where + 1] = '\0';
        
        // if client is already logged in, process commands
        if (client->name[0] != '\0') {
            printf("Message received from %s: %s\r\n", client->name,
                client->buf);
            fflush(stdout);
            
            // tokenize input into arguments
            char *cmd_argv[INPUT_ARG_MAX_NUM];
            int cmd_argc = tokenize(client->buf, cmd_argv);

            // process commands
            if (cmd_argc > 0 && process_args(cmd_argc, cmd_argv, user_list_ptr,
                    client, &top) == -1) {
                char buf[80];
                printf("Client %s disconnected\n", inet_ntoa(client->ipaddr));
                fflush(stdout);
                sprintf(buf, "Logging you out, %s...\r\n", client->name);
                write(client->fd, buf, strlen(buf));
                remove_client(client->fd);
                return next; // can only reach if quit command was entered
            } else if (cmd_argc == 0) {
                error("your message was too long.", client->fd);
            }
                
            write(client->fd, "\r\n> ", 4);

        } else { // new client, create new user or log into existing one
            char temp_name[MAX_NAME];
            if (strlen(client->buf) >= MAX_NAME) {
                strncpy(temp_name, client->buf, MAX_NAME - 1);
                temp_name[MAX_NAME] = '\0';
            } else {
                strcpy(temp_name, client->buf);
            }
            switch (create_user(temp_name, user_list_ptr)) {
                case 0: // new user successfully created
                {
                    strcpy(client->name, temp_name);
                    int len = 43 + strlen(client->name);
                    char out[len];
                    snprintf(out, len,
                    "\r\nGreetings, %s!\r\nPlease type a command:\r\n> ",
                    client->name);
                    write(client->fd, out, len);
                }
                    break;
                case 1: // user exists, client is a returning user
                {
                    strcpy(client->name, temp_name);
                    int len = 46 + strlen(client->name);
                    char out[len];
                    snprintf(out, len,
                    "\r\nWelcome back, %s!\r\nPlease type a command:\r\n> ",
                    client->name);
                    write(client->fd, out, len);
                }
                    break;
                case 2: // given name is too long
                    error("username is too long", client->fd);
                    write(client->fd, "\r\n> ", 4);
                    break;
            }
        }
          
        // update inbuf and remove the full line from buf
        client->inbuf -= (client->where + 2);
        for (int i = 0; i < client->where; i++) {
            client->buf[i] = '\0';
        }
          
        // move content after the full line to beginning of buf
        memmove(&client->buf[0], &client->buf[client->where +2], client->inbuf);
    }
    // update room and after, in preparation for the next read
    client->room  = sizeof(client->buf) - client->inbuf;
    client->after = &client->buf[client->inbuf];
    
    return next;
}


/* 
 * Write a formatted error message to fd.
 */
void error(char *msg, int fd) {
    int len = 10 + strlen(msg);
    char out[len];
    snprintf(out, len, "Error: %s\r\n", msg);
    write(fd, out, len);
}
