#include <signal.h>             /* signal */
#include <stdio.h>              /* perror */
#include <stdlib.h>             /* exit */
#include <string.h>             /* memset */
#include <unistd.h>             /* read, close */
#include <sys/socket.h>         /* socket, setsockopt */
#include <sys/stat.h>           /* stat */
#include <fcntl.h>              /* open */
#include <netinet/in.h>         /* struct sockaddr_in */
#include <arpa/inet.h>          /* inet_ntoa */

#define PORT 80
#define BACKLOG 128

void process_connection(int socket_fd, struct sockaddr_in *client_addr);
void send_response(int socket_fd, unsigned char *buffer, int chars_to_send);
void get_request(int socket_fd, unsigned char *buffer);

int main(int argc, char **argv)
{
    int reuseaddr = 1;
    socklen_t sin_size;
    int listening_socket_fd, accepting_socket_fd;
    struct sockaddr_in server_addr, client_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), 0, 8);

    if((listening_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Cannot create socket");
        exit(1);
    }

    /* SOL_REUSEADDR allows you to bind to an address in TIME_WAIT */
    if(setsockopt(listening_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
    {
        perror("Cannot setsocketopt");
        exit(1);
    }

    if(bind(listening_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Cannot bind");
        exit(1);
    }

    /*  
     *  backlog just set to /proc/sys/net/core/somaxconn
     *  i.e. default value for max in connection queue
     */
    if(listen(listening_socket_fd, BACKLOG) == -1)
    {
        perror("Cannot listen");
        exit(1);
    }

    /* 
     *  Ignore "broken pipe" errors
     *  These can occur if the client (e.g. the browser) closes the connection
     *  with the server, but the server is not aware and attempts to send data.
     */
    signal(SIGPIPE, SIG_IGN);

    printf("Accepting connection on port %d\n", PORT);

    while(1)
    {
        sin_size = sizeof(client_addr);
        if((accepting_socket_fd = accept(listening_socket_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1)
        {
            perror("Cannot accept connection");
            exit(1);
        }

        process_connection(accepting_socket_fd, &client_addr);
    }

    return 0;
}

void process_connection(int socket_fd, struct sockaddr_in *client_addr)
{
    /* 
     *  Shitty Web Server does not care about your request. You get index.html
     *  no matter what you want.
     */
    unsigned char request[500];
    unsigned const char *response_filename = "./index.html";
    unsigned const char *response_header = "HTTP/1.1 200 OK\r\nServer: Shitty Web Server\r\n\r\n";
    unsigned char *response_data;
    int response_fd, response_file_size;
    int header_size = strlen(response_header);
    struct stat file_stat;

    /* 
     *   If I was to inline get_request here then the local variables (cr and request_ptr) would probably
     *   get overwritten when the stack is overflowed, completely fucking with the getting of the request.
     */
    get_request(socket_fd, request);

    printf("%s\t%s:%d\n", request, inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

    /* If the request is anything other than "GET /" then fuck off. */
    if(strncmp(request, "GET / ", 6) == 0)
    {
        if((response_fd = open(response_filename, O_RDONLY, 0)) == -1)
        {
            perror("Cannot open file");
            exit(1);
        }

        if((fstat(response_fd, &file_stat)) == -1)
        {
            perror("Cannot stat file");
            exit(1);
        }

        response_file_size = file_stat.st_size;

        if((response_data = (unsigned char *)malloc(response_file_size + header_size)) == NULL)
        {
            perror("Cannot allocate data for file");
            exit(1);
        }

        if(read(response_fd, response_data + header_size, response_file_size) == -1)
        {
            perror("Cannot read");
            exit(1);
        }

        memcpy(response_data, response_header, header_size);

        send_response(socket_fd, response_data, response_file_size + header_size);
        free(response_data);
        close(response_fd);
    }
    else
    {
        unsigned char *error_response = "HTTP/1.1 404 NOT FOUND\r\nServer: Shitty Web Server\r\n\r\nGTFO\r\n";
        send_response(socket_fd, error_response, strlen(error_response));
    }

    shutdown(socket_fd, SHUT_RDWR);
}

void get_request(int socket_fd, unsigned char *buffer)
{
    int cr = 0;
    while(recv(socket_fd, buffer, 1, 0) == 1)
    {
        if(*buffer == '\r')
        {
            cr = 1;
        }
        else if((cr == 1) && (*buffer == '\n'))
        {
            /* go back to \r and terminate the string */
            *(buffer - 1) = '\0';
            break;
        }

        buffer++;
    }
}

void send_response(int socket_fd, unsigned char *buffer, int chars_to_send)
{
    int chars_sent;

    while(chars_to_send > 0)
    {
        if((chars_sent = send(socket_fd, buffer, chars_to_send, 0)) == -1)
        {
            perror("Cannot send");
            break;
        }

        chars_to_send -= chars_sent;
        buffer += chars_sent;
    }
}
