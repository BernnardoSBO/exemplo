#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
// hostent, gethostbyname
#include <netdb.h>

// 
#include <netinet/in.h>

// #include <netinet/in.h>

// struct sockaddr_in {
//     short            sin_family;   // e.g. AF_INET
//     unsigned short   sin_port;     // e.g. htons(3490)
//     struct in_addr   sin_addr;     // see struct in_addr, below
//     char             sin_zero[8];  // zero this if you want to
// };

// struct in_addr {
//     unsigned long s_addr;  // load with inet_aton() 32BITS
// };

// struct hostent
// {
//   char *h_name;			/* Official name of host.  */
//   char **h_aliases;		/* Alias list.  */
//   int h_addrtype;		/* Host address type.  */
//   int h_length;			/* Length of address.  */
//   char **h_addr_list;		/* List of addresses from name server.  */
// #ifdef __USE_MISC
// # define	h_addr	h_addr_list[0] /* Address, for backward compatibility.*/
// #endif
// };

int main (int argc, char* argv[]) {
    int socket_file_descriptor, portnumber;
    struct hostent *server;
    struct sockaddr_in serv_addr;

    if (argc < 3) {
        fprintf(stderr, "usage: %s hostname port\n", argv[0]);
        exit(0);
    }

    portnumber = atoi(argv[2]);

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_file_descriptor < 0) {
        perror("ERROR opening socket");
        exit(0);
    }
    
    // gethostbyname returns a hostent struct with data to connect
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    // set memory position to 0
    memset(&serv_addr, 0, sizeof(serv_addr));

    // copy values pointed by 1 to area pointed by 2
    // in this case, it's copying the value of the IPv4 address in server to
    // the IPv4 field in serv_addr
    // obs: serv_addr = sockaddr_internet
    // sin_addr = socket internet address
    // s_addr = 4 byte address (IPv4)
    bcopy((char *) server->h_addr_list[0],
          (char *) &serv_addr.sin_addr.s_addr,
          server->h_length);

    serv_addr.sin_port = htons(portnumber);

    if (connect(socket_file_descriptor, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        perror("ERROR connecting");
        exit(0);
    }

}