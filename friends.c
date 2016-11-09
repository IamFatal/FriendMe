#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * Create a new user with the given name.  Insert it at the tail of the list 
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME - 1

    for (int i = 0; i < MAX_NAME; i++) {
        new_user->profile_pic[i] = '\0';
    }

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {       // bug fixed 03/04/2016. Now correct on repeat of 1st name
        *user_ptr_add = new_user;
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        return 0;
    }
}


/* 
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
/*    const User *curr = head;
    while (curr != NULL && strcmp(name, curr->name) != 0) {
        curr = curr->next;
    }

    return (User *)curr;
*/
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (User *)head;
}


/*
 * Return a dynamically allocated string containing the usernames of all users
 * in the list, one per line, starting at curr.
 */
char *list_users(const User *curr) {
    int buf_len = 1;
    const User *head = curr;
    
    // calculate sum of the lengths of every name
    while (curr != NULL) {
        buf_len += strlen(curr->name) + 2;  // add 2 for each network newline
        curr = curr->next;
    }
    
    curr = head;                    // go back to the head of the list
    int len = 0;                    // track length of buf
    char *buf = malloc(buf_len);    // allocate buffer of sufficient size
    
    // add all usernames in list to allocated string
    while (curr != NULL) {
        len += snprintf(buf + len, buf_len - len, "%s\r\n", curr->name);
        curr = curr->next;
    }
    
    return buf; // return pointer to string listing all users
}


/* 
 * Make two users friends with each other.  This is symmetric - a pointer to 
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends must be added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * Do not modify either user if the result is a failure.
 * NOTE: If multiple errors apply, return the *largest* error code that applies.
 */
int make_friends(const char *name1, const char *name2, User *head) {
    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);

    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        } 
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}


/* 
 * Return a pointer to a dynamically allocated string holding a user profile.
 */
char *print_user(const User *user) {
    int buf_len = 1;                    // 1 for null terminator
    buf_len += 8 + strlen(user->name);  // "Name: \r\n"     8 characters
    buf_len += 10;                      // "Friends:\r\n"   10 characters
    buf_len += 8;                       // "Posts:\r\n"     8 characters
    buf_len += 44 * 3;                  // dashed break     44 characters
    
    
    // add length of each friend's name
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        buf_len += strlen(user->friends[i]->name) + 2;
    }
    
    // add length of all posts
    const Post *curr = user->first_post;
    while (curr != NULL) {
        // add lengths of author, date, and message
        buf_len += strlen(curr->author) + 8;
        buf_len += strlen(asctime(localtime(curr->date))) + 8;
        buf_len += strlen(curr->contents) + 2;
        curr = curr->next;
        if (curr != NULL) {
            buf_len += strlen("\r\n===\r\n\r\n"); // 9
        }
    }

    int len = 0;                    // track length of buf
    char *buf = malloc(buf_len);    // allocate buffer of sufficient size
    curr = user->first_post;        // go back to head of list
    
    // Add name
    len += snprintf(buf + len, buf_len - len, "Name: %s\r\n", user->name);
    len += snprintf(buf + len, buf_len - len,
                "------------------------------------------\r\n");

    // Add friends list.
    len += snprintf(buf + len, buf_len - len, "Friends:\r\n");
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
        len += snprintf(buf + len, buf_len - len, "%s\r\n", user->friends[i]->name);
    }
    len += snprintf(buf + len, buf_len - len,
                "------------------------------------------\r\n");

    // Add post list.
    len += snprintf(buf + len, buf_len - len, "Posts:\r\n");
    while (curr != NULL) {
        // Add author
        len += snprintf(buf + len, buf_len - len, "From: %s\r\n", curr->author);
    
        // Add date
        char *time = asctime(localtime(curr->date));
        len += snprintf(buf + len, buf_len - len, "Date: %s\r\n", time);

        // Add message
        len += snprintf(buf + len, buf_len - len, "%s\r\n", curr->contents);
        
        curr = curr->next;
        if (curr != NULL) {
            len += snprintf(buf + len, buf_len - len, "\r\n===\r\n\r\n");
        }
    }
    len += snprintf(buf + len, buf_len - len,
                "------------------------------------------\r\n");

    return buf;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Use the 'time' function to store the current time.
 *
 * 'contents' is a pointer to heap-allocated memory - you do not need
 * to allocate more memory to store the contents of the post.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents) {
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}
