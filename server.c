#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

const unsigned short MAX_BYTES = 1024;
const unsigned short PORTNUM = 2019;

void sum_line(char buf[], unsigned int buflen, int newsockfd)
{
        unsigned short i = 0;
        unsigned int sum = 0;
        unsigned int digit;
        bool error = false;

        if( buflen > MAX_BYTES)
                error = true;

        if ( !( buf[0] >= '0' && buf[0] <= '9' )|| !( buf[buflen-3] >= '0' && buf[buflen-3] <= '9' )
             || buf[buflen-2] != '\r' || buf[buflen-1] != '\n' )
                error = true;

        while(i < buflen - 2 && error == false)
        {
                digit = 0;

                if( buf[i] == ' ' )
                {
                        error = true;
                        break;
                }

                while( i < buflen - 2 && buf[i] != ' ' )
                {
                        if( buf[i] < '0' || buf[i] > '9' )
                        {
                                error = true;
                                break;
                        }

                        if( digit > INT_MAX/10 + INT_MAX%10 )
                        {
                                error = true;
                                break;
                        }

                        digit *= 10;

                        if( digit > INT_MAX  - (buf[i] - '0') )
                        {
                                error = true;
                                break;
                        }

                        digit += (buf[i] - '0');

                        i++;

                }

                if( sum > INT_MAX - digit )
                {
                        error = true;
                        break;
                }

                sum += digit;
                i++;
        }

        if(error)
        {
                if( send( newsockfd, "ERROR\r\n", 7, 0 ) < 0 )
                {
                        perror("send");
                        exit(EXIT_FAILURE);
                }
        }
        else
        {
                int length = snprintf( NULL, 0, "%d", sum );
                char* str = malloc( length + 3 );
                snprintf( str, length + 3, "%d\r\n", sum );
                if( send( newsockfd, str, length + 3, 0 ) < 0 )
                {
                        perror("send");
                        exit(EXIT_FAILURE);
                }
        }
}

int main(int argc, char *argv[])
{
        int sockfd, newsockfd;
        char buf[MAX_BYTES + 1];
        unsigned int clilen;
        struct sockaddr_in serv_addr, cli_addr;

        newsockfd = 0;
        clilen = sizeof(cli_addr);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if ( sockfd < 0 )
        {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTNUM);
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        if ( bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 )
        {
                perror("bind\n");
                exit(EXIT_FAILURE);
        }

        if ( listen(sockfd, 3) < 0 )
        {
                perror("listen\n");
                exit(EXIT_FAILURE);
        }

        while( true )
        {
                int error = 0;
                socklen_t len = sizeof (error);
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

                if (newsockfd < 0)
                {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                while( true )
                {
                        // sprawdzenie czy polaczenie z klientem jest utrzymane.
                        // zrodlo pomyslu: https://stackoverflow.com/questions/4142012/how-to-find-the-socket-connection-state-in-c
                        int retval = getsockopt (newsockfd, SOL_SOCKET, SO_ERROR, &error, &len);

                        if (retval != 0)
                        {
                                fprintf(stderr, "error getting socket \n error code: %s\n", strerror(retval));
                                break;
                        }

                        if (error != 0)
                        {
                                fprintf(stderr, "socket error: %s\n", strerror(error));
                        }

                        memset(buf, 0x00, MAX_BYTES * sizeof(char));

                        // odebranie danych, zapisanie ich do bufora i przeslanie bufora do funkcji sumujacej
                        int buflen = read(newsockfd, buf, MAX_BYTES );

                        if( buflen == 0 )
                                break;

                        if( buflen < 0 )
                        {
                                perror("read");
                                exit(EXIT_FAILURE);
                        }

                        sum_line(buf, buflen, newsockfd);
                }

                printf("Client disconnected\n");
                close(newsockfd);
        }
        return 0;
}
