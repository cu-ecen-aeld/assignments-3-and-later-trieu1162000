// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <malloc.h>
#include <signal.h>
#include <pthread.h>
#include <sys/queue.h> 
#include <time.h>

#define PORT 9000
#define BACKLOG 10
#define BUFFER_SIZE 1000000
#define SLIST_FOREACH_SAFE(var, head, field, tvar)                           \
    for ((var) = SLIST_FIRST((head));                                        \
            (var) && ((tvar) = SLIST_NEXT((var), field), 1);                 \
            (var) = (tvar))

typedef struct client_data_struct client_data;
struct client_data_struct{
    int client_fd;
    // bool is_thread_complete;
    struct sockaddr_in client_address;
    pthread_t client_thread_id;
    SLIST_ENTRY(client_data_struct) entries;
};

SLIST_HEAD(slisthead, client_data_struct) head_of_thread_list;
int server_fd, new_client_fd;
int file_fd;
pthread_mutex_t locked_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t last_timestamp_update = 0;
pthread_t write_time_to_file_thread;
client_data *new_client_data = NULL;

void signal_handler(int);

void *client_handler(void *);

void *write_time_to_file_handler(void *);

int main(int argc, char const* argv[])
{
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    socklen_t client_addr_len = sizeof(client_addr);
    bool daemon_mode = false;

    // Init the head of linked list
    SLIST_INIT(&head_of_thread_list);

    openlog("SOCKET_SERVER", LOG_CONS | LOG_PID, LOG_USER);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Check process is daemon or not
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    }

    // Run as a daemon if -d argument provided
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "folk() failed");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            exit(0);
        }

        syslog(LOG_INFO, "In daemon mode");

        // Child process continues to run as a daemon
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Creating socket server file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "socket() failed");
        syslog(LOG_ERR, "Create socket server fd - FAIL");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Create socket server fd - PASS");

    // Forcefully attaching socket to the port 9000
    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
        syslog(LOG_ERR, "setsockopt() failed");
        syslog(LOG_ERR, "Attach socket server fd to the PORT - FAIL");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    syslog(LOG_INFO, "Attach socket server fd to the PORT - PASS");

    // Bind the socket server to the PORT
    if (bind(server_fd, (struct sockaddr *)&server_addr,
            sizeof(server_addr))
        < 0) {
        syslog(LOG_ERR, "bind() failed");
        syslog(LOG_ERR, "Bind socket server fd to the PORT - FAIL");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Bind socket server fd to the PORT - PASS");

    // Listen for connections
    if (listen(server_fd, BACKLOG) < 0) {
        syslog(LOG_ERR, "listen() failed");
        syslog(LOG_ERR, "Listen for connections - FAIL");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Listen for connections - PASS");

    // Create the timer thread
    if (pthread_create(&write_time_to_file_thread, NULL, write_time_to_file_handler, NULL)) {
        syslog(LOG_ERR, "pthread_create() failed");
        syslog(LOG_ERR, "Create thread for write time to file - FAIL");
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Create thread for write time to file - PASS");

    // Insert write time thread into the list
    client_data *write_time_data = (client_data *)malloc(sizeof(client_data));
    write_time_data->client_thread_id = write_time_to_file_thread;
    SLIST_INSERT_HEAD(&head_of_thread_list, write_time_data, entries);

    while(1){
        int new_client_fd;
        pthread_t new_thread_id;
        // client_data *thread_to_exit_data = NULL;

        // Accept new connection
        new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *) &client_addr_len);
        if (new_client_fd < 0)
        {
            syslog(LOG_ERR, "accept() failed");
            syslog(LOG_ERR, "Accept new connection - FAIL");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "Accept new connection - PASS");


        // Allocate struct client data
        new_client_data = (client_data *)malloc(sizeof(client_data));
        if (new_client_data == NULL) {
            syslog(LOG_ERR, "Allocate struct client data - FAIL");
            exit(EXIT_FAILURE);
        }
        new_client_data->client_fd = new_client_fd;
        new_client_data->client_address = client_addr;
        // new_client_data->is_thread_complete = false;

        // Spawn thread to handle new client
        if (pthread_create(&new_thread_id, NULL, client_handler, (void *)new_client_data) < 0) {
            syslog(LOG_ERR, "pthread_create() failed");
            syslog(LOG_ERR, "Spawn new thread - FAIL");
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "Spawn new thread - PASS");

        new_client_data->client_thread_id = new_thread_id;

        // Add new spawned thread to list
        SLIST_INSERT_HEAD(&head_of_thread_list, new_client_data, entries);
        syslog(LOG_INFO, "Add new thread to list");

        // For each thread in list. Check complete flag
        // SLIST_FOREACH(new_client_data, &head_of_thread_list, entries) {
            // if(new_client_data->is_thread_complete)
            // {
                // free(new_client_data);
            // }
            // // continue;
        // }
    }
    
    return 0;
}

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");

        // Signal all threads to exit gracefully
        client_data *thread_to_exit_data, *temp_thread_data;

        SLIST_FOREACH_SAFE(thread_to_exit_data, &head_of_thread_list, entries, temp_thread_data) {
            // Request each thread to exit gracefully
            pthread_cancel(thread_to_exit_data->client_thread_id);
        }

        SLIST_FOREACH_SAFE(thread_to_exit_data, &head_of_thread_list, entries, temp_thread_data) {
            // Wait for all threads to complete execution
            pthread_join(thread_to_exit_data->client_thread_id, NULL);
        }

        // Free linked list
        while (!SLIST_EMPTY(&head_of_thread_list)) {
            thread_to_exit_data = SLIST_FIRST(&head_of_thread_list);
            SLIST_REMOVE_HEAD(&head_of_thread_list, entries);
            free(thread_to_exit_data);
        }

        // Close the socket and other
        shutdown(server_fd, SHUT_RDWR);
        close(file_fd);
        close(server_fd);
        close(new_client_fd);
        closelog();

        // Free mutex
        pthread_mutex_destroy(&locked_mutex);

        // Delete the file
        remove("/var/tmp/aesdsocketdata");

        syslog(LOG_INFO, "Process exitted");
        exit(EXIT_SUCCESS);
    }
}

void *client_handler(void *arg)
{
    // Block signals that the thread is not terminated prematurely by these signals
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    client_data *handler_client_data = (client_data *) arg;
    char client_ip_string[INET_ADDRSTRLEN];
    ssize_t received_bytes = 0;
    ssize_t total_received_bytes = 0;
    char* buffer = NULL;
    int rc;
    
    // Allocate memory for buffer
    buffer = malloc(sizeof(char)*BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);

    inet_ntop( AF_INET, &((handler_client_data->client_address).sin_addr), client_ip_string, INET_ADDRSTRLEN );
    syslog(LOG_INFO, "Accepted connection from %s", client_ip_string);

    /*
    *****************************************************************
    ** Receive data from socket to file
    *****************************************************************
    */

    // Obtain mutex
    rc = pthread_mutex_lock(&locked_mutex);
    if (rc != 0){
        syslog(LOG_ERR, "pthread_mutex_lock() failed");
        syslog(LOG_ERR, "Obtain the mutex - FAIL");
        close(handler_client_data->client_fd);
        free(buffer);
        return NULL;
    }
    syslog(LOG_INFO, "Obtain the mutex - PASS");

    // Open file descriptor for writing, create if it doesnot exist
    file_fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_APPEND | O_CREAT, 0666);
    if (file_fd == -1)
    {
        syslog(LOG_ERR, "open() failed");
        syslog(LOG_ERR, "Open file to read - FAIL");
        close(handler_client_data->client_fd);
        free(buffer);
        return NULL;
    }
    syslog(LOG_INFO, "Open file to read - PASS");

    while((received_bytes = read(handler_client_data->client_fd, buffer, BUFFER_SIZE)) > 0)
    {
        // Check buffer exceeds the heap memory
        if (total_received_bytes > mallinfo().fordblks)
        {
            syslog(LOG_ERR, "Buffer exceeds heap memory. Close the socket instantly");
            // Free resources
            close(file_fd);
            close(handler_client_data->client_fd);
            free(buffer);
            
            return NULL;
        }

        syslog(LOG_INFO, "Write the buffer to file");
        // Append the buffer to the file
        write(file_fd, buffer, received_bytes);
        
        // Check new line character
        if (strchr(buffer, '\n') != NULL)
            break;
        total_received_bytes += received_bytes;
    }

    /*
    *****************************************************************
    */

    // Release the mutex to allow another thread to write
    rc = pthread_mutex_unlock(&locked_mutex);
    if (rc != 0){
        syslog(LOG_ERR, "pthread_mutex_unlock() failed");
        syslog(LOG_ERR, "Release the mutex - FAIL");
        close(file_fd);
        close(handler_client_data->client_fd);
        free(buffer);
        return NULL;
    }
    syslog(LOG_INFO, "Release the mutex - PASS");

    /*
    *****************************************************************
    ** Write back file to the socket
    *****************************************************************
    */
    // Read file and store whole data to the buffer
    lseek(file_fd, 0, SEEK_SET);
    while((received_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        syslog(LOG_INFO, "Send back data to the socket");
        // Send back buffer to the client
        send(handler_client_data->client_fd, buffer, received_bytes, 0);
    }
    /*
    *****************************************************************
    */

    // Set flag complete to true
    // handler_client_data->is_thread_complete = true;

    // Free resources 
    close(file_fd);
    close(handler_client_data->client_fd);
    free(buffer);

    syslog(LOG_INFO, "Closed connection from %s", client_ip_string);
}

void *write_time_to_file_handler(void *arg) {
    time_t current_time;
    struct tm *time_info;
    char timestamp[128]; // Adjust the size as needed
    int fd, rc;

    while (1) {
        // Obtain mutex
        rc = pthread_mutex_lock(&locked_mutex);
        if (rc != 0){
            syslog(LOG_ERR, "pthread_mutex_lock() failed");
            syslog(LOG_ERR, "Obtain the mutex - FAIL");
            return NULL;
        }
        syslog(LOG_INFO, "Obtain the mutex - PASS");

        // Get the current time
        time(&current_time);
        time_info = localtime(&current_time);

        // Format the timestamp using RFC 2822 compliant strftime
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", time_info);

        // Open the file for appending
        fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (fd == -1) {
            syslog(LOG_ERR, "open() failed");
            syslog(LOG_ERR, "Open file to write time - FAIL");
            continue;
        }
        syslog(LOG_INFO, "Open file to write time - PASS");

        // Append the timestamp
        write(fd, timestamp, strlen(timestamp));

        // Close the file
        close(fd);

        // Release the mutex to allow another thread to write
        rc = pthread_mutex_unlock(&locked_mutex);
        if (rc != 0){
            syslog(LOG_ERR, "pthread_mutex_unlock() failed");
            syslog(LOG_ERR, "Release the mutex - FAIL");
            return NULL;
        }
        syslog(LOG_INFO, "Release the mutex - PASS");

        syslog(LOG_INFO, "Sleep 10s for every time after writing timestamp");
        // Sleep for 10 seconds before appending the next timestamp
        sleep(10);
    }
}
