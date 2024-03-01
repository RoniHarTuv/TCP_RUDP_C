#include "RUDP.API.h"

/*Here we create our rudp socket.
the socket constructor will get the arguments:
1)AF_INET - for ipv4 in our rudp version.
2)SOCK_DGRAM - to set the protocol be udp with our adding.
3) 0 to mation the protocol that we are using.
*/
int rudp_socket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket(2)");
        return -1;
    }
    return sock;
}

/**
 * This method will make the 3 way handshake as heppens in tcp protocol.
 * Will firstly send the Syn from the sender to start the connection.
 * Then, if a syn-ack is received from the receiver to the sender we will send ack
 * back to the receiver as the last step in the 3way handshake.
 * If the 3way handshake is done the sender starts to send the data from the file.
 */
int rudp_connect(int sock, struct sockaddr_in *addr)
{
    Header syn = {0};
    syn.flags = SYN;
    syn.checksum = calculate_checksum(&syn, sizeof(syn));
    if (sendto(sock, &syn, sizeof(syn), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        printf("Syn messege wasnt sent!\n");
        return -1;
    }
    // chatgpt was used in lines 51-58.
    fd_set read_fds;
    struct timeval timeout = {3, 0};
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    struct sockaddr_in syn_ack_add;
    struct sockaddr_in syn_ack_from;
    socklen_t from_len = sizeof(syn_ack_from);
    int s = select(sock + 1, &read_fds, NULL, NULL, &timeout);
    printf("SYN messege sent, wating for SYN-ACK\n");
    Header syn_ack;
    if (s > 0)
    {
        if (recvfrom(sock, &syn_ack, sizeof(syn_ack), 0, (struct sockaddr *)&syn_ack_add, &from_len) < 0)
        {
            printf("The SYN-ACK wasnt sent.");
            return -1;
        }
        if (syn_ack.flags != SYNACK)
        {
            printf("received not a SYN-ACK packet");
            return -1;
        }
        printf("The SYN-ACK answer was received!\n");
        Header ack = {0};
        ack.flags = LASTACKIN3;
        ack.checksum = calculate_checksum(&ack, sizeof(ack));
        if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0)
        {
            printf("The ACK messege wasn't sent\n");
            return -1;
        }
        printf("The ACK was sent!\n");
    }
}

/**
 * This function will be responsible to send the data over the socket.
 * We want to allow more then one attemp of sending the data if someting went wrong.
 * if the data is sent properlly we expect to get ack from the receiver.
 * After the ack is sent and received in the sender syde we want to retun the
 * number of bytes that have sent.
 */
int rudp_send(int sock, Header *packet, size_t packet_size, const struct sockaddr_in *dest_addr)
{
    int atp = 0;
    Header ack = {0};
    socklen_t from_len = sizeof(struct sockaddr_in);
    struct sockaddr_in from_addr;
    while (atp < MAX_ATTEMPTS)
    {
        int send_bty = sendto(sock, packet, packet_size, 0, (const struct sockaddr *)dest_addr, sizeof(*dest_addr));
        if (send_bty < 0)
        {
            printf("Failed to send the packet!\n");
            return -1;
        }
        // chatgpt was used in lines 108-112.
        fd_set read_fds;
        struct timeval timeout = {4, 0};
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        int s = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        if (s>0 && recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&from_addr, &from_len) >= 0 && ack.flags == ACK)
        {
            return send_bty;
        }
        atp++;
    }
    printf("The send function failed, reached to max attemps or timeout!\n");
    return -1;
}

/**
 * This function will be responsible to sending the ack for the data
 * that have received correctly.
 * Will construct a new heder and will set the header flag to be
 * ack. then will dent it over the socket with sendto method of udp protocol.
 */
void rudp_sendack(int sock, struct sockaddr_in *addr)
{
    Header ack_packet = {0};
    ack_packet.flags = ACK;
    ack_packet.checksum = 0;
    ack_packet.checksum = calculate_checksum(&ack_packet, sizeof(ack_packet));
    if (sendto(sock, &ack_packet, sizeof(ack_packet), 0, (const struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        printf("Failed to send the ACK\n");
        return;
    }
}

/**
 * This function will be responsible for receiving the data over our
 * rudp protocol.
 * In this part of the code we want to check the parametrs that we add to
 * the header of each packet that is sent.
 * This method will cllasify the flags and will determine how to
 * act with each type od packet that is received based on the
 * flags field in the header of the packet.
 * will also check that the checksum and the length of the data that is
 * received is the ont the we sent in the sender side when sending the packet.
 * In the end this method will return the number of bytes that is received.
 */
int rudp_recv(int sock, void *buffer, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    int recv_bytes = recvfrom(sock, buffer, len, flags, src_addr, addrlen);
    if (recv_bytes < 0)
    {
        printf("Receive 0 bytes-error!\n");
        return -1;
    }
    Header *packet = (Header *)buffer;
    unsigned short int senders_side_checksum = packet->checksum;
    int senders_side_length = packet->length;
    packet->checksum = 0;
    unsigned short int received_side_checksum = calculate_checksum(packet, recv_bytes);
    if (packet->flags == 'D')
    {
        received_side_checksum = calculate_checksum(packet + 1, recv_bytes - HEADER_SIZE);
    }
    if (senders_side_checksum != received_side_checksum || senders_side_length != recv_bytes - HEADER_SIZE)
    {
        printf("Checksum error or length error");
        return -1;
    }
    switch (packet->flags)
    {
    case 'D': // data
        rudp_sendack(sock, (struct sockaddr_in *)src_addr);
        break;

    case 'S': // syn
        printf("SYN packet received from the sender!\n");
        Header syn_ack_packet = {0};
        syn_ack_packet.flags = SYNACK;
        syn_ack_packet.checksum = 0;
        syn_ack_packet.checksum = calculate_checksum(&syn_ack_packet, sizeof(syn_ack_packet));
        sendto(sock, &syn_ack_packet, sizeof(syn_ack_packet), 0, src_addr, sizeof(struct sockaddr_in));
        printf("SYN-ACK sent!\n");
        break;

    case 'F': // fin
        printf("FIN packet received from the sender!\n");
        printf("Sending Ack to fin!\n");
        rudp_sendack(sock, (struct sockaddr_in *)src_addr);
        break;

    case 'A': // ack
        printf("ACK packet received!\n");
        break;

    case 'L': // last ack for 3 way handshake
        printf("ack for syn-ack received\n");
        printf("The 3 handshake was completed, waiting for data!\n");
        break;

    default: // if none of the above
        printf("Received an unknown type\n");
        return -1;
    }
    return recv_bytes;
}

/**
 * This function will be resbonsible to close the rudp connection.
 * The function will send the fin messege from the sender to the receiver.
 * Then will expect to get an ack for the fin from the receiver in the
 * sender side. after ack is sent and received we close the connection
 * with close method from the udp protocol.
 */

int rudp_close(int sock, const struct sockaddr_in *server_addr)
{
    Header fin = {0};
    fin.flags = FIN;
    fin.checksum = 0;
    fin.checksum = calculate_checksum(&fin, sizeof(fin));
    if (sendto(sock, &fin, sizeof(fin), 0, (const struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        printf("Failed to send fin packet");
        return -1;
    }
    printf("FIN packet sent!\n");
    Header ack = {0};
    if (recvfrom(sock, &ack, sizeof(ack), 0, NULL, NULL) < 0)
    {
        printf("faild to receive the ack packet (fin)\n");
        return -1;
    }
    else
    {
        if (ack.flags != ACK)
        {
            printf("not an ack packet\n");
            return -1;
        }
        printf("ack for FIN received!\n");
        return close(sock);
    }

    return 0;
}

/*
 * @brief A checksum function that returns 16 bit checksum for data.
 * @param data The data to do the checksum for.
 * @param bytes The length of the data in bytes.
 * @return The checksum itself as 16 bit unsigned number.
 * @note This function is taken from RFC1071, can be found here:
 * @note https://tools.ietf.org/html/rfc1071
 */
unsigned short int calculate_checksum(void *data, unsigned int bytes)
{
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1)
    {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0)
        total_sum += *((unsigned char *)data_pointer);
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    return (~((unsigned short int)total_sum));
}
