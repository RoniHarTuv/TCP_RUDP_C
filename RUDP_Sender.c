#include "RUDP.API.h"

/**
 * This class will represent the sender side for our rudp protocol.
 * The docomention of each part will be inside the class.
 */
int main(int argc, char *argv[])
{
    // Checks that we entered the allowed number of arguments in the terminal.
    if (argc != 5)
    {
        printf("not the right amount of arguments\n");
        return -1;
    }

    // Will set the port acording to the port that is received in the terminal.
    const char *receiver_ip = NULL;
    int receiver_port = 0;
    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-ip") == 0)
            receiver_ip = argv[i + 1];
        else if (strcmp(argv[i], "-p") == 0)
            receiver_port = atoi(argv[i + 1]);
    }

    // Check that the ip is valid and the port is greater then 0.
    if (receiver_ip == NULL || receiver_port <= 0)
    {
        printf("The IP/port is already in use");
        return -1;
    }
    printf("The arguments received!\n");

    // Construct the rudp socket.
    // Will check if there is a problem in the api class.
    int sock = -1;
    sock = rudp_socket();
    printf("RUDP socket created successfully!\n");

    // The variable to store the server's address:
    struct sockaddr_in receiver;
    // Reset the server structure to zeros:
    memset(&receiver, 0, sizeof(receiver));
    // Set the server's address family to AF_INET:
    receiver.sin_family = AF_INET;
    // Set the server's port to the defined port:
    receiver.sin_port = htons(receiver_port);
    // Convert receiver's IP address to binary form:
    if (inet_pton(AF_INET, receiver_ip, &receiver.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    // Using connect function to establish the 3 hand shake connection:
    // as described in the api class.
    if (rudp_connect(sock, &receiver) < 0)
    {
        printf("RUDP connection failed.\n");
        close(sock);
        return 1;
    }

    // Will generate the random data to the file that we want to send.
    char *data = util_generate_random_data(DATA_SIZE);
    if (data == NULL)
    {
        printf("Failed to generate data\n");
        return 1;
    }

    // Will create the file and will give him the write and read promotions.
    FILE *file = fopen("random_data.txt", "wr");
    if (file == NULL)
    {
        printf("Failed to open file for reading\n");
        free(data);
        close(sock);
        return 1;
    }
    // Will write the data to the file we have just created.
    fwrite(data, sizeof(char), DATA_SIZE, file);
    // Will close the file.
    fclose(file);

    // Our main loop for sending the data:
    while (1)
    {
        // Will hold the data that was read from the file.
        FILE *file_read = fopen("random_data.txt", "r");
        if (file_read == NULL)
        {
            printf("Failed to open file for reading\n");
            free(data);
            close(sock);
            return 1;
        }
    
        int bytes_sent = 0;
        int bytes_read = 0;
        //Will determin a second buffer with size of 1500 to send over udp.
        char buffer2[65400];
        //This loop will send this run data. will check if there is more data
        //to read from the file.
        while ((bytes_read = fread(buffer2, sizeof(char), sizeof(buffer2), file_read)) > 0)
        {
            int size = sizeof(Header) + bytes_read;
            // Will set a dynemic size of packet 
            // We donf know how much data will be sent and that the reason for
            //malloc in this part.(not always same size of data).
            Header *packet = (Header *)malloc(size);
            if (packet == NULL)
            {
                printf("malloc for the packet faild.\n");
                break;
            }
            //Will set the arguments of our header:
            packet->length = bytes_read;
            packet->flags = DATA;
            packet->checksum = calculate_checksum(buffer2, bytes_read);
            // The next cell in the memory will be the data that 
            //We want to send (from the file):
            memcpy(packet + 1, buffer2, bytes_read);
            //The sending using rudp packet as we declered.
            bytes_sent += rudp_send(sock, packet, size, &receiver);
            if (bytes_sent < 0)
            {
                perror("send(2)");
                fclose(file);
                free(data);
                close(sock);
                return 1;
            }
        }
        printf("Sent %d bytes to the server!\n", bytes_sent);

        // Will close the file after reading him whole.
        fclose(file_read);
        printf("Data sent successfully!\n");

        // In this part of the code we ask in the terminal if
        //the sender wants to send more data over our rudp protocol.
        //while the sender press y or Y we will return back to the start 
        //of the main loop above.
        char option = 0;
        printf("Send more data? (Y/N): ");
        scanf(" %c", &option);
        if (option != 'Y' && option != 'y')
        {
            break;
        }
    }
    //If the senders press N or anything that is not y we want to close the 
    //connection and end the program. will use the rudp_close function
    //which docomention is in the api class.
    if (rudp_close(sock, &receiver) != 0)
    {
        printf("Cannot close the connection");
    }
    printf("Connection closed!\n");
    return 0;
}

// This function is responsible for generating the
// random data that we are going to send.(As given).
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
