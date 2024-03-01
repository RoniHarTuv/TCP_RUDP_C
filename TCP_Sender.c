#include <stdio.h>       
#include <arpa/inet.h>   
#include <sys/time.h>    
#include <time.h>        
#include <sys/socket.h>  
#include <unistd.h>      
#include <string.h>      
#include <stdlib.h>      
#include <netinet/tcp.h> 
#include <signal.h>

//a helper function you gave us to generate random data
char *util_generate_random_data(unsigned int size);

//receiver IP address
#define SERVER_IP "127.0.0.1"
//receiver port
#define SERVER_PORT 8888
//The size of the data (2 MB bytes)
#define DATA_SIZE 2 * 1024 * 1024

/*
 *  ***** TCP Sender main function ******
 */
int main(int argc, char *argv[])
{
    // Retrieve information from the user:
    const char *algo = NULL;
    int receiverPORT = 0;
    const char *receiverIP = NULL;

    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-ip") == 0)
            receiverIP = argv[i + 1];
        else if (strcmp(argv[i], "-p") == 0)
            receiverPORT = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "-algo") == 0)
            algo = argv[i + 1];
    }

    // Check if the IP or port is already in use:
    if (receiverIP == NULL || receiverPORT <= 0)
    {
        printf("The IP/port is already in use");
        return -1;
    }
    printf("The arguments received!\n");

    /**
     * ****** Create the TCP socket: ******
     */

    int sock = -1;
    // Create the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }
    printf("The sender's socket was created!\n");

    // Set Cubic\Reno algo
    int a = setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo));
    if (a != 0)
    {
        perror("setsockopt(2)");
        close(sock);
        return 1;
    }
    printf("The algorithm was chosen!\n");

    //The server's addres
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    printf("Connecting to [%s:%d]...\n", SERVER_IP, SERVER_PORT);

    // Try to connect to the server using the socket and the server structure.
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect(2)");
        close(sock);
        return 1;
    }
    printf("Successfully connected to the server with [%s:%d] using algo %s \n", receiverIP, receiverPORT, algo);

    //Generate random data to be sent to the receiver
    char *data = util_generate_random_data(DATA_SIZE);
    if (data == NULL)
    {
        printf("Failed to generate data\n");
        return 1;
    }

    // Write it to file (if needed)
    // Open the file to read data and send over TCP
    FILE *file = fopen("random_data.txt", "wr");
    if (file == NULL)
    {
        printf("Failed to open file for reading\n");
        free(data);
        close(sock);
        return 1;
    }
    fwrite(data, sizeof(char), DATA_SIZE, file);
    fclose(file);
    char *buffer = (char *)malloc(DATA_SIZE);
    if (buffer == NULL)
    {
        perror("malloc");
        free(data);
        close(sock);
        return 1;
    }
    while (1)
    {
        // Read data from file and send over TCP
        FILE *file_read = fopen("random_data.txt", "r");
        if (file_read == NULL)
        {
            printf("Failed to open file for reading\n");
            free(data);
            close(sock);
            return 1;
        }
        int bytesSent = 0;
        size_t bytesRead;
        while ((bytesRead = fread(buffer, sizeof(char), DATA_SIZE, file_read)) > 0)
        {
            bytesSent += send(sock, buffer, bytesRead, 0);
            if (bytesSent < 0)
            {
                perror("send(2)");
                fclose(file);
                free(data);
                close(sock);
                return 1;
            }
        }
        printf("Sent %d bytes to the server!\n", bytesSent);
        fclose(file_read);
        printf("The data was sent!\n");
        // Ask the user:
        char option = 0;
        printf("Send more data? [Y/N]: ");
        scanf(" %c", &option);
        if (option != 'Y' && option != 'y')
        {
            break;
        }
    }
    //Exit
    int exit = send(sock, "EXIT", 4, 0);
    if (exit < 0)
    {
        printf("The exit message faild");
    }
    else
    {
        printf("Sender sending exit message...\n");
    }
    // Free allocated data and close socket
    free(data);
    close(sock);
    printf("->Connection closed!<-\n");

    return 0;
}
// function that generate random data* 
char *util_generate_random_data(unsigned int size)
{
    char *buffer = NULL;
    if (size == 0)
        return NULL;
    buffer = (char *)calloc(size, sizeof(char));
    if (buffer == NULL)
        return NULL;
    srand(time(NULL)); 
    for (unsigned int i = 0; i < size; i++)
    {
        *(buffer + i) = ((unsigned int)rand() % 256);
    }
    return buffer;
}
