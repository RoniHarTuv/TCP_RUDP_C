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

// TCP PORT
#define PORT 8888
// 2MB in bytes for the buffer
#define BUFFER_SIZE 2 * 1024 * 1024
// The maximum number of clients that the server can handle.
#define CLIENTS_IN_QUEUE 1
// The maximum request
#define MAX_REQ 100

// Function to get current time in milliseconds
//chatgpt was used in lines 23-28.
double current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);                                             // Get current time
    return ((double)te.tv_sec * 1000.0) + ((double)te.tv_usec / 1000.0); // Convert to milliseconds
}

// WE USU IPV4 ALL OVER THE ASSIGNMENT//
/*
 *  ***** TCP Receiver main function ******
 */
int main(int argc, char *argv[])
{
    // first, we get the input fron the user.
    if (argc != 5) // if the input is illegal
    {
        return 1;
    }
    // define the algorithm
    const char *algo = NULL;

    // the port number
    int port = PORT;

    // gets information from the user:
    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-p") == 0)
            port = atoi(argv[i + 1]);
        else if (strcmp(argv[i], "-algo") == 0)
            algo = argv[i + 1];
    }
    if (port <= 0)
    {
        printf("port number is invalid\n");
        return 1;
    }
    printf("The Arguments received successfully.port number is: %d\n", port);
    /**
     * ****** create the TCP socket: ******
     */
    int sock = -1;
    // Create the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }
    printf("The receiver's socket was created!\n");

    // Set Cubic\Reno algo.
    if (algo != NULL)
    {
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0)
        {
            perror("Faild to set the socket\n");
            close(sock); // close the socket
            return 1;
        }
    }

    // The receiver's address
    struct sockaddr_in receiver;

    // The sender's address
    struct sockaddr_in sender;

    // The sender's structure length
    socklen_t sender_length = sizeof(sender);

    memset(&receiver, 0, sizeof(receiver));
    memset(&sender, 0, sender_length);

    receiver.sin_family = AF_INET;
    receiver.sin_addr.s_addr = INADDR_ANY;

    // Set the receiver's port to the user port
    receiver.sin_port = htons(port);

    // Bind
    if (bind(sock, (struct sockaddr *)&receiver, sizeof(receiver)) < 0)
    {
        perror("bind(2)");
        close(sock);
        return 1;
    }
    printf("receiver binds successfully!\n");

    // The receiver listen....
    if (listen(sock, CLIENTS_IN_QUEUE) < 0)
    {
        perror("listen(2)");
        close(sock);
        return 1;
    }
    printf("Sender connected. beginning to receive file....\n");
    // Accept
    int sender_sock = accept(sock, (struct sockaddr *)&sender, &sender_length);
    if (sender_sock < 0)
    {
        perror("accept(2)");
        close(sock);
        return 1;
    }
    // using a variables in order to print the statistics after the program finished
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int numberOfRun = 0;
    double totaltime = 0;
    double totalAllByts = 0;
    double *bandwidth = (double *)malloc(sizeof(double) * MAX_REQ);
    double *times = (double *)malloc(sizeof(double) * MAX_REQ);
    // while the sender continue to send a files
    while (1)
    {
        double start = current_timestamp(); //start the time
        double totalB = 0.0;
        double msec = 0;
        while (totalB != BUFFER_SIZE)
        {
            bytes_received = recv(sender_sock, buffer, BUFFER_SIZE, 0);
            if (bytes_received < 0)
            {
                perror("recv(2)");
                close(sender_sock);
                close(sock);
                return 1;
            }
            if (strncmp(buffer, "EXIT", 4) == 0)
            {
                break;
            }
            // If the amount of received bytes is 0, the client has disconnected.
            // Close the client's socket and continue to the next iteration.
            else if (bytes_received == 0)
            {
                fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
                close(sender_sock);
                continue;
            }
            totalB += bytes_received;
        }

        totalAllByts += totalB;
        double end = current_timestamp(); //stop the time
        msec = end - start; // Time elapsed in milliseconds
        totaltime += msec;
        times[numberOfRun] = msec;
        // calculate the bandwidth for each run
        bandwidth[numberOfRun] = ((totalB / 1024 / 1024) / (times[numberOfRun] / 1000.0));
        numberOfRun++;
        // if the sender finish
        if (strncmp(buffer, "EXIT", 4) == 0)
        {
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }
    /**
     * ****** Prints the statistics ******
     */
    for (int i = 0; i < numberOfRun - 1; i++) // each run
    {
        printf("**********************************************\n");
        printf("              #%d run statistics:                 \n", (i + 1));
        printf("*Time: %.3lf ms.\n", times[i]);
        printf("*Bandwidth: %.3lf MB/s.\n", bandwidth[i]);
    }

    printf("**********************************************\n");
    printf("              Total statistics:                 \n");
    if (numberOfRun > 0)
    {
        printf("* The AVG time of sending was: %.3lf ms.\n", totaltime / numberOfRun);
        double speed = 0.0;
        if (totaltime != 0)
        {
            double TotalDataMB = totalAllByts / (1024.0 * 1024.0); // Convert bytes to MB
            speed = TotalDataMB / (totaltime / 1000.0);            // Convert totaltime to seconds
        }
        printf("* The AVG bandwidth was: %.3lf MB/s.\n", speed);
    }
    else
    {
        printf("* No data received.\n");
    }
    printf("**********************************************\n");
    printf("->the connection closed .program finished<-\n");
    // free the dynamic array
    free(bandwidth);
    free(times);
    close(sock);
    close(sender_sock);
    return 0;
}