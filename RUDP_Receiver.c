#include "RUDP.API.h"
/**
 * This class will represent the receiver side for our rudp protocol.
 * The docomention of each part will be inside the class.
 */

// Function to get current time in milliseconds
double current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);                                             // Get current time
    return ((double)te.tv_sec * 1000.0) + ((double)te.tv_usec / 1000.0); // Convert to milliseconds
}
int main(int argc, char *argv[])
{
    // Checks that we entered the allowed number of arguments in the terminal.
    if (argc != 3)
    {
        return 1;
    }
    // Defult port
    int port = RECEIVER_PORT;
    // Will set the port to the port that is writen in the termianl.
    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-p") == 0)
            port = atoi(argv[i + 1]);
    }
    // will check that the port is greater then 0.
    if (port <= 0)
    {
        printf("port number is invalid\n");
        return 1;
    }
    printf("Arguments received successfully.port number is %d\n", port);

    // will create the rudp socket in the receiver side.
    // if fails then will fail at the api inse the rudp_socket.
    int sock = -1;
    sock = rudp_socket();
    printf("socket was opened!\n");

    // The variable to store the receiver's address.
    struct sockaddr_in receiver;
    // The variable to store the sender's address.
    struct sockaddr_in sender;
    // A variable that stores the sender's structure length
    socklen_t sender_len = sizeof(sender);
    // Reset the receiver and sender structures to zeros
    memset(&receiver, 0, sizeof(receiver));
    memset(&sender, 0, sender_len);
    // Set the receiver address family to AF_INET
    receiver.sin_family = AF_INET;
    // Set the receiver address 0.0.0.0
    receiver.sin_addr.s_addr = htonl(INADDR_ANY);
    // Set the receiver's port to the port
    receiver.sin_port = htons(port);

    // Bind:
    if (bind(sock, (struct sockaddr *)&receiver, sizeof(receiver)) < 0)
    {
        perror("bind(2)");
        close(sock);
        return 1;
    }
    printf("receiver binds successfully!\n");
    int numberOfRun = 0;
    double totaltime = 0;
    double totalAllByts = 0;
    double *bandwidth = (double *)malloc(sizeof(double) * MAX_REQ);
    double *times = (double *)malloc(sizeof(double) * MAX_REQ);
    double timeTakeninMiliSec = 0;
    // double start = 0;
    //  the main loop of receiving the data in the receiver side.
    int isRunning = 1;
    while (isRunning)
    {
        double totalB = 0;
        // the header that will receive to.
        Header recv_packet;
        // the buffer that will receive the data to.
        char buffer[DATA_SIZE + (16 * (DATA_SIZE / 65400))];
        memset(buffer, 0, DATA_SIZE + (16 * (DATA_SIZE / 65400)));
        double timeTakeninSec = 0;
        // this loop will run until we get all the data from the file
        // that the sender is tring to send.
        double startTime = current_timestamp();
        while (totalB < DATA_SIZE + (16 * (DATA_SIZE / 65400)))
        {
            // time strat.
            // will use the rudp_recv function to receive the data to the buffer.
            int this_run_bytes = rudp_recv(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender, &sender_len);
            if (this_run_bytes == -1)
            {
                perror("recv(2)");
                close(sock);
                break;
            }
            Header *recv = (Header *)buffer;
            // printf("the flag is: %d\n",recv->flags);
            //  if the header that we receive is Fin we want to send
            //   ack(in the api file) and close the connection.
            if (recv->flags == 'F')
            {
                close(sock);
                isRunning = 0;
                break;
            }
            else if (this_run_bytes == 0)
            {
                printf("Client %s:%d disconnected\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port));
                close(sock);
                continue;
            }
            totalB += this_run_bytes;
        }
        double endTime = current_timestamp();
        if (startTime != 0)
        {
            timeTakeninMiliSec = endTime - startTime;
            totalAllByts += totalB;
            times[numberOfRun] = timeTakeninMiliSec;
            bandwidth[numberOfRun] = totalB / (timeTakeninMiliSec * 1000);
            numberOfRun++;
        }
    }
    bandwidth[numberOfRun - 1] = DATA_SIZE / (timeTakeninMiliSec * 1000);
    for (int i = 1; i < numberOfRun; i++)
    {
        printf("**********************************************\n");
        printf("              #%d run statistics:                 \n", (i));
        printf("*Time: %.3lf ms.\n", times[i]);
        printf("*Bandwidth: %.3lf MB/s.\n", bandwidth[i]);
        totaltime  += times[i];
    }
    printf("**********************************************\n");
    printf("              Total statistics:                 \n");
    if (numberOfRun > 0)
    {
        printf("* The AVG time of sending was: %.3lf ms.\n", totaltime / (numberOfRun - 1));
        double speed = 0.0;
        if (totaltime != 0)
        {
            double TotalDataMB = totalAllByts / (1024.0 * 1024.0); // Convert bytes to MB
            speed = TotalDataMB / (totaltime / 1000.0);            // Convert totaltime to seconds
        }
        printf("* The AVG bandwidth was: %.3lf MB/s.\n", speed);
    }
    printf("Connection closed!\n");
    free(bandwidth);
    free(times);
    return 0;
}