#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 1
#define BUF_SIZE 1024
struct parameters
{
    int server, type, family;
    char *address, *port;
};
void exit_with_perror( char *msg )
{
    perror(msg);
    exit(0);
}
void help_and_exit( char *program )
{
    printf("%s [adres] [port] - nawiązuje połączenie (TCP) z danym adresem na podanym porcie.\n", program );
    printf("-u [port]         - będzie używać UDP.\n");
    printf("-l [port]         - nasłuchuje na podanym porcie.\n");
    printf("-4                - przetworzy adres tylko w wersji 4.\n");
    printf("-6                - przetworzy adres tylko w wersji 6.\n");
    exit(0);
}
int get_socket( struct parameters *prm )
{
    int sockfd, sockopt, yes, err_code;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = prm->family;
    hints.ai_socktype = prm->type;
    if( prm->server )
        hints.ai_flags = AI_PASSIVE;

    if( (err_code = getaddrinfo( prm->address, prm->port, &hints, &servinfo )) != 0 ) // prm->adress NULL dla serwera
    {
        fprintf(stderr, "getaddrinfo: %s %d\n", gai_strerror(err_code), err_code);
        exit(0);
    }

    for( p = servinfo; p != NULL; p = p->ai_next )
    {
        sockfd = socket( p->ai_family, p->ai_socktype, p->ai_protocol );

        if( sockfd == -1 )
        {
            perror("socket()");
            continue;
        }
        if( prm->server )
        {
            if( (sockopt = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) )) == -1 )
                exit_with_perror("setsockopt()");

            if( bind( sockfd, p->ai_addr, p->ai_addrlen ) == -1 )
            {
                close( sockfd );
                perror("bind()");
                continue;
            }
        }
        else
        {
            if( connect( sockfd, p->ai_addr, p->ai_addrlen ) == -1 )
            {
                close(sockfd);
                perror("connect()");
                continue;
            }
        }
        break;
    }

    if( p == NULL )
    {
        printf("Nie udało się połączyć.");
        exit(0);
    }
    freeaddrinfo(servinfo);
    return sockfd;
}
void server_mode( struct parameters *prm )
{
    int serverfd, clientfd, n;
    char txt_buf[BUF_SIZE];
    socklen_t len;
    struct sockaddr_storage theiraddr;
    /* tworzenie gniazda */
    serverfd = get_socket( prm );
    /* komunikacja */
    len = sizeof theiraddr;
    if( prm->type == SOCK_STREAM ) //TCP
    {
        if( listen( serverfd, BACKLOG ) == -1 ) 
            exit_with_perror("listen()");


        if( ( clientfd = accept( serverfd, ( struct sockaddr *) &theiraddr, &len ) ) == -1 ) 
            exit_with_perror("accept()");

        while( ( n = read( clientfd, txt_buf, BUF_SIZE ) ) > 0 )
        {
            if( write( fileno(stdout), txt_buf, n ) == -1 )
                exit_with_perror("write()");
        }

        if( n == -1 )
            exit_with_perror("read()");

        close(clientfd);
    }
    else if( prm->type == SOCK_DGRAM ) //UDP
    {
        while(1)
        {
            if((n = recvfrom( serverfd, txt_buf, BUF_SIZE, 0, (struct sockaddr *) &theiraddr, &len )) == -1 )
                continue;
            write( fileno(stdout), txt_buf, n );
        }
    }
    close(serverfd);
}
void client_mode( struct parameters *prm )
{
    int sockfd, n;
    char txt_buf[BUF_SIZE];
    /* tworzenie i łączenie gniazda */
    sockfd = get_socket( prm );

//    char prompt1[] = "Waiting for input...\n";
//    char prompt2[] = "Stopped waiting for input.\n";
//    char prompt3[] = "Stopped waiting for response.\n";

//    write( fileno(stdout), prompt1, strlen(prompt1));

    /* komunikacja */
    while( ( n = read( fileno(stdin), txt_buf, BUF_SIZE ) ) > 0 )
    {
            if( write( sockfd, txt_buf, n ) == -1 )
                exit_with_perror("write()");

//        write( fileno(stdout), prompt1, strlen(prompt1));

    }

//    write( fileno(stdout), prompt2, strlen(prompt2));

    if( n == -1 )
        exit_with_perror("read()");
    while( ( n = read( sockfd, txt_buf, BUF_SIZE ) ) > 0)
    {
        if( write( fileno( stdout ), txt_buf, n ) == -1 )
            exit_with_perror("write()");
    }
    if( n == -1 )
        exit_with_perror("read()");

//    write( fileno(stdout), prompt3, strlen(prompt3) );

    close(sockfd);
}
int main( int argc, char *argv[] )
{
    struct parameters prm;
    prm.server = 0;
    prm.type = SOCK_STREAM;
    prm.family = AF_UNSPEC;
    prm.address = NULL;
    prm.port = NULL;

    for( int i = 1; i < argc; ++i )
    {
        if( argv[i][0] == '-' )
        {
            if( strcmp( argv[i], "-l") == 0 )
                prm.server = 1;
            else if( strcmp( argv[i], "-u") == 0 )
                prm.type = SOCK_DGRAM;
            else if( strcmp( argv[i], "-6") == 0 )
            {
                if( prm.family == AF_INET )
                    prm.family = AF_UNSPEC;
                else
                    prm.family = AF_INET6;
            }
            else if( strcmp( argv[i], "-4") == 0 )
            {
                if( prm.family == AF_INET6 )
                    prm.family = AF_UNSPEC;
                else
                    prm.family = AF_INET;
            }
            else
                help_and_exit(argv[0]);
        }
    }
    if( prm.server )
    {
        for( int i = 1; i < argc; ++i )
            if( argv[i][0] != '-' )
                prm.port = argv[i];
        if( prm.port == NULL )
            help_and_exit( argv[0] );
        server_mode( &prm );
    }
    else
    {
        for( int i = 1; i < argc; ++i )
        {
            if( argv[i][0] != '-' && prm.address == NULL )
                prm.address = argv[i];
            else if( argv[i][0] != '-' )
                prm.port = argv[i];
        }
        if( prm.address == NULL || prm.port == NULL )
            help_and_exit( argv[0] );
        client_mode( &prm );
    }
    return 0;
}
