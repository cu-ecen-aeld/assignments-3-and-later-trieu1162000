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

#define PORT 9000
#define BACKLOG 5
#define BUFFER_SIZE 100000

int server_fd, new_client_fd;
int file_fd;

void signal_handler(int);

void client_handler(int, struct sockaddr_in);

int main(int argc, char const* argv[])
{
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    socklen_t client_addr_len = sizeof(client_addr);
    bool daemon_mode = false;

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
            perror("Error forking");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            exit(0);
        }

        // Child process continues to run as a daemon
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 9000
    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 9000
    if (bind(server_fd, (struct sockaddr *)&server_addr,
            sizeof(server_addr))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    
    while(1){
        if ((new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *) &client_addr_len)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Handle the new client
        client_handler(new_client_fd, client_addr);

    }
    
    return 0;
}

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");

        // Close the socket and other
        close(file_fd);
        close(server_fd);
        close(new_client_fd);
        closelog();

        // Delete the file
        remove("/var/tmp/aesdsocketdata");
        
        exit(0);
    }
}

void client_handler(int client_fd, struct sockaddr_in client_address)
{

    char client_ip_string[INET_ADDRSTRLEN];
    ssize_t received_bytes = 0;
    ssize_t total_received_bytes = 0;
    char* buffer = NULL;
    
    // Allocate memory for buffer
    buffer = malloc(sizeof(char)*BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);

    inet_ntop( AF_INET, &client_address.sin_addr, client_ip_string, INET_ADDRSTRLEN );
    syslog(LOG_INFO, "Accepted connection from %s", client_ip_string);
    
    /*
    *****************************************************************
    ** Receive data from socket to file
    *****************************************************************
    */
    
    // Open file descriptor for writing, create if it doesnot exist
    file_fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_APPEND | O_CREAT, 0666);
    
    // Handle if open error
    if (file_fd == -1)
    {
        perror("open");
        close(client_fd);
        free(buffer);
        return;
    }
    
    while((received_bytes = read(client_fd, buffer, BUFFER_SIZE)) > 0)
    {
        // Check buffer exceeds the heap memory
        if (total_received_bytes > mallinfo().fordblks)
        {
            syslog(LOG_INFO, "Buffer exceeds heap memory. Close the socket instantly");
            
            // Free resources
            close(file_fd);
            close(client_fd);
            free(buffer);
            
            return;
        }
        
        // Append the buffer to the file
        write(file_fd, buffer, received_bytes);
        
        // Check new line character
        if (strchr(buffer, '\n') != NULL)
            break;
        total_received_bytes += received_bytes;
    }
    
    close(file_fd);
    /*
    *****************************************************************
    */
    
    /*
    *****************************************************************
    ** Write back file to the socket
    *****************************************************************
    */
    
    // Open file descriptor for reading
    file_fd = open("/var/tmp/aesdsocketdata", O_RDONLY, 0666);
    
    // Read file and store whole data to the buffer
    while((received_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        // Send back buffer to the client
        send(client_fd, buffer, received_bytes, 0);
    }
    /*
    *****************************************************************
    */
    
    // Free resources 
    close(file_fd);
    free(buffer);
    
    // Close client socket
    close(client_fd);
    syslog(LOG_INFO, "Closed connection from %s", client_ip_string);
    
}