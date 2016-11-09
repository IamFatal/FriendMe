#include <time.h>
#include <arpa/inet.h>

#define MAX_NAME 32             // Max username length
#define INPUT_BUFFER_SIZE 256   // Max buffer length

 /*************************Taken from muffinman.c****************************/

typedef struct client {
    char name[MAX_NAME];
    char buf[INPUT_BUFFER_SIZE];
    int inbuf;      // number of bytes currently in buffer
    int room;       // number of bytes available in buffer
    char *after;    // pointer to position after the (valid) data in buf
    int where;      // location of network newline
    int fd;
    struct in_addr ipaddr;
    struct client *next;
} Client;

/*
 * Create a new client and insert it at the head of the client's list.
 */
Client *add_client(int fd, struct in_addr addr);

/*
 * Remove client associated with the given file descriptor from linked list, 
 * free all allocated memory, and close the file descriptor.
 */
void remove_client(int fd);

/*
 * Accept the new connection, create a new client, and ask for a username.
 */
void new_connection(int listenfd);

 /***************************************************************************/

/*
 * Setup socket and return the file descriptor for listening.
 */
int setup();

/*
 * Read and process input from client's fd. Return the next client in list.
 */
Client *get_args(Client *client, User **user_list_ptr);

/*
 * Search the first inbuf characters of buf for a network newline ("\r\n").
 * Return the location of the '\r' if the network newline is found,
 * or -1 otherwise.
 */
int find_network_newline(const char *buf, int inbuf);

/*
 * Tokenize the string stored in cmd.
 * Return the number of tokens, and store the tokens in cmd_argv.
 */
int tokenize(char *cmd, char **cmd_argv);

/* 
 * Read and process commands
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, 
        Client *client, Client **top);

/*
 * Find a client with the given name. Return NULL if no such client exists.
 */
Client *find_client(char *name, Client **top);

/* 
 * Write a formatted error message to fd.
 */
void error(char *msg, int fd);
