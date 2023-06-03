#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// sockadrr, bind, listen, send, recv
#include <sys/socket.h>

// hostent, gethostbyname
#include <netdb.h>

// sockadrr_in
#include <netinet/in.h>

#include <sys/select.h>

/*
file descriptors for I/O
STDIN_FILENO    0
STDOUT_FILENO   1
STDERR_FILENO   2
*/ 
#include <unistd.h>

#include <arpa/inet.h>

#define VERBOSE 1
#define MAX_CLIENTS 15
#define REQADDPEER "Hello, how are you"

int is_peering = 0;
int ID, peer_ID;


void printverb(char* output) {
    if (VERBOSE) 
        printf("%s", output);
}

int peering (int peer_port, char* peer_addr_string) {
    int peer_socket_fd;

    // SOCKET
    peer_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (peer_socket_fd < 0){
        perror("ERROR:\ncouldn't open peering socket");
        exit(1);
    }

    if (setsockopt(peer_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed\n");
        exit(1);
    }

    // ADDRESS
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    
    struct hostent* host_server;
    host_server = gethostbyname(peer_addr_string);

    bcopy((char*) host_server->h_addr_list[0],
          (char*)& peer_addr.sin_addr.s_addr,
          host_server->h_length);

    // CONNECTING
    if (connect(peer_socket_fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) >= 0) {
        
        // send: REQADD_PEER
        is_peering = 1;

        int n;
        char REQADD_PEER[] = "01.00.00.00";
        
        n = send(peer_socket_fd, REQADD_PEER, strlen(REQADD_PEER), 0);
        printf("Sent reqadd_peer\n");

        // recv: ERROR(06) OR RES_ADD
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        n = recv(peer_socket_fd, buffer, 50, 0);

        // Message ID
        char msgId[3];
        strncpy(msgId, buffer, 2);
        msgId[2] = '\0';
        int numMsgId = atoi(msgId);

        // Payload
        int payload_length = n - 9;
        char * payload = (char *) malloc(((n-9)+1)*sizeof(char));
        strncpy(payload, buffer+9, 2);
        payload[payload_length] = '\0';
        int numPayload = atoi(payload);
    
                    // char msgOriginId[3];
                    // strncpy(msgOriginId, buffer+3, 2);
                    // msgOriginId[2] = '\0';
                    // int numMsgOriginId = atoi(msgOriginId);

                    // char msgDestinationId[3];
                    // strncpy(msgDestinationId, buffer+6, 2);
                    // msgDestinationId[2] = '\0';
                    // int numMsgDestinationId = atoi(msgDestinationId);


       
            
                
        if (numMsgId == 11) {                       // if error(6)
            printf("Peer limit exceeded\n");            // print error(6) message: "Peer limit exceeded"
            is_peering = 0;
        } 
  
        else if (numMsgId == 7) {                   // elif res_add(PiD)

            ID = numPayload; 
            
            // print: "New Peer ID"
            printf("New Peering ID: %d\n", ID);

            peer_ID = rand() % 10;
            
            while (ID == peer_ID) {
                peer_ID = rand() % 10;
            }

            // send: res_add(PiD)
            char RES_ADD[12] = "07.00.00.0";
            n = sprintf(RES_ADD+10, "%d", peer_ID);
            n = send(peer_socket_fd, RES_ADD, strlen(RES_ADD), 0);

            // print: "Peer PiDSj connected"
            printf("Peer %d connected\n", peer_ID);

            // TODO recv: res_list(Id1,Id2)
        }
        printverb("Active Connection Performed\n");

        free(payload);
    } 

    // OR BINDING/LISTENING
    if (!is_peering) {
        printverb("No peer found, starting to listen\n");
        if (bind(peer_socket_fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
            perror("ERROR on binding\n");
            exit(1);
        }
        if (listen(peer_socket_fd,10) != 0) {
            perror("ERROR listening to peer\n");
            exit(1);
        }
    }
    return peer_socket_fd;    
}

int serving (int server_port) {
    int server_socket_fd;

    // SOCKET
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_fd < 0) {
        perror("ERROR\ncouldn't open serving socket\n");
        exit(1);
    }

    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed\n");
        exit(1);
    }

    // ADDRESS
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // BINDING TO ADDRESS(PORT/IP)
    if (bind(server_socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR binding to serving socket\n");
        exit(1);
    }

    // LISTENING
    if (listen(server_socket_fd, 16) != 0){
        perror("ERROR listening to clients");
        exit(1);
    }

    // RETURNING SOCKET
    return server_socket_fd;
}

void handle_peering_connection(int peer_socket_fd) {

    // declaring variable to receive peer address
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    socklen_t peer_len = sizeof(peer_addr);

    // accept
    int new_peer_socket = accept(peer_socket_fd, (struct sockadrr *)&peer_addr, &peer_len);
    printf("Connected\n");
    if (new_peer_socket < 0) {
        perror("accept()");
        exit(1);
    }

    // Receive REQADD_PEER
    int n;
    char buffer[256];
    // memset(&buffer, 0, sizeof(buffer));
    printf("\n(1)\n");

    n = recv(new_peer_socket, buffer, 255, 0);
    
    printf("message received: %s\n", buffer);
    printf("String size: %ld\n", sizeof(buffer));
    printf("Received size: %d\n", n);

    // check message arrival format
    printf("(2)\n");
         
    if (is_peering) {
        char ERROR06[] = "11.00.00.06";                         // building ERROR(06) package to send
        
        printf("\nMessage about to be sent: %s\n", ERROR06);

        n = send(new_peer_socket, ERROR06, strlen(ERROR06), 0); //send:  ERROR(06)

        close(new_peer_socket);                                 //close: connection to peer
    }

    else {                                                  // else:
        char RES_ADD[12] = "07.00.00.0";                    // building RES_ADD(PID) package to send
        int peerOtherId = rand() % 10;
        n = sprintf(RES_ADD+10, "%d", peerOtherId);

        printf("\nMessage about to be sent: %s\n", RES_ADD);

        n = send(new_peer_socket, RES_ADD, strlen(RES_ADD), 0); //send:  RES_ADD(PID) sent
                
        // n = recv(new_peer_socket, buffer, 255, 0);              //recv:  res_add our PiD number

        // TODO: GET ID FROM MESSAGE END
            // CHECK IF MESSAGE IS RES_ADD
            // IF IT IS, GET PID NUMBER FROM THE BACK OF RES_ADD
            // SEND LIST OF CONNECTED CLIENTS

        is_peering = 1;

        //TODO send:  res_list (list of current connected clients)

    }

}

int main(int argc, char* argv[]) {
    // CHECKING INPUT
    if (argc != 4) {
        fprintf(stderr, "ERROR\nusage: %s <ipv4address> <peerport> <serverport>\n", argv[0]);
        exit(1);
    }
    
    // PEERING
    int peer_socket_fd, peer_port;
    peer_port = atoi(argv[2]);
    peer_socket_fd = peering(peer_port, argv[1]);
   

   // SERVING
   int server_socket_fd, server_port;
   server_port = atoi(argv[3]);
   server_socket_fd = serving(server_port);

   
    // FILE DESCRIPTOR SETS
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    /* 
    create sets to listen
    backup sets
    select from sets -> alters 
    */


    int highSocket = 17;
    

    while (1) {
        fd_set read_fds_copy = read_fds;

        int socketCount = select(highSocket+10, &read_fds_copy, NULL, NULL, NULL);

        if (FD_ISSET(peer_socket_fd, &read_fds_copy)){
            handle_peering_connection(peer_socket_fd);
        }
    }
    // highSocket = serverSocketfd;

    // while (1) {
    
    // // established highest socket fd id
    // // create fd_sets

    // int res = select(highSocket, &read_fds, NULL, NULL, NULL);

    // switch (res) {
    //     case -1:
    //         perror("ERROR on select(), returning -1\n");
    //         exit(1);
    //     case 0:
    //         perror("ERROR on select(), returning 0\n");
    //         exit(1);
    //     default:
    //         // if stdinput fd is in read set
    //         if (FD_ISSET(STDIN_FILENO, &read_fds)) {
    //             // handle read
    //         }

    //         // if peer is trying to connect
    //         if (FD_ISSET(peerSocketfd, &read_fds)) {
    //             //handle peer connection
    //         }

    //         // if client is trying to connect
    //         if (FD_ISSET(serverSocketfd, &read_fds)) {
    //             //handle new client connection
    //         }

    //         // if stdinput fd is in read set
    //         if (FD_ISSET(STDIN_FILENO, &except_fds)) {
    //             // handle read
    //         }

    //         // if peer is trying to connect
    //         if (FD_ISSET(peerSocketfd, &except_fds)) {
    //             //handle peer connection
    //         }

    //         // if client is trying to connect
    //         if (FD_ISSET(serverSocketfd, &except_fds)) {
    //             //handle new client connection
    //         }

    //         for (i=0; i < max_clients; ++i) {
    //             if (socketList[i] != NO_SOCKET && FD_ISSET(socketList[i], &read_fds)) {
    //                 receivemsg
    //                 if receivemsg = fail 
    //                     close_connection
    //             }

    //             if (socketList[i] != NO_SOCKET && FD_ISSET(socketList[i], &write_fds)) {
    //                 sendMsgToPeer
    //                 if sendMsgToPeer = fail
    //                     close_connection
    //             }

    //             if (socketList[i] != NO_SOCKET && FD_ISSET(socketList[i], &except_fds)) {
    //                 close_connection
    //             }
    //         }


    //     }
    // /*
    // keep track of highest socket to iterate over

    // build fd_sets
    
    // select between them

    // check for input read
    // check for peering Socket read
    
    // check for client socket read
    
    // */


    // }
    /*
    1st) try connecting to another peer
    2nd) listening to another peer and clients
    
    */
    
    // int peerSocketfd, peerPort;

    // if (argc < 3)

    // int socket_file_descriptor, portnumber, clientlength;
    // struct sockaddr_in serv_addr;

    // if (argc < 2) {
    //     fprintf(stderr, "ERROR no port provided");
    //     exit(1);
    // }

    // socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    
    // if (socket_file_descriptor < 0) {
    //     perror("ERROR opening socket");
    // }

    // bind(socket_file_descriptor, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
}