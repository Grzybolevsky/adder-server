#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

const unsigned short MAX_BYTES = 1024;
const unsigned short PORTNUM = 2019;

void sum_line(char* buf, unsigned int buflen, int newsockfd)
{
        unsigned i = 0, sum = 0, digit;
        bool succesful = true;

        if(buflen > MAX_BYTES || !( buf[i] >= '0' && buf[i] <= '9' ) ) succesful = false;

        while( i < buflen )
        {
                digit = 0;

                while( buf[i] >= '0' && buf[i] <= '9'  )
                {
                        if(i > MAX_BYTES || digit > INT_MAX/10 + INT_MAX%10 ) succesful = false;

                        digit *= 10;

                        if( digit > INT_MAX  - (buf[i] - '0') ) succesful = false;

                        digit += (buf[i] - '0');

                        i++;
                }

                if( sum > INT_MAX - digit ) succesful = false;

                sum += digit;
                if(buf[i] == '\r' && buf[i + 1] == '\n' )
                {
                        if(succesful)
                        {
                                int length = snprintf( NULL, 0, "%d", sum );
                                char str[length + 2];
                                sprintf( str, "%d\r\n", sum );
                                if( send( newsockfd, str, length + 2, 0 ) < 0 )
                                {
                                        perror("send");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        else
                        {
                                if( send( newsockfd, "ERROR\r\n", 7, 0 ) < 0 )
                                {
                                        perror("send");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        if(i + 2 < buflen)
                                sum_line( (buf+i+2), buflen-(i+2), newsockfd );
                        return;
                }
                else if(buf[i] == ' ')
                {
                        i++;
                        if( buf[i] < '0' || buf[i] > '9') succesful = false;
                }
                else
                {
                        if( send( newsockfd, "ERROR\r\n", 7, 0 ) < 0 )
                        {
                                perror("send");
                                exit(EXIT_FAILURE);
                        }
                        while(buf[i] != '\r' && (buf[i+1] != '\n' && i + 2 < buflen) ) i++;
                        if(i + 2 < buflen) sum_line( (buf+i+2), buflen-(i+2), newsockfd );
                        return;
                }
        }
}

int main(int argc, char *argv[])
{
        int sockfd, newsockfd;
        char buf[MAX_BYTES];
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
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

                if (newsockfd < 0)
                {
                        perror("accept");
                        exit(EXIT_FAILURE);
                }

                while( true )
                {
                        memset(buf, 0x00, MAX_BYTES * sizeof(char));

                        int buflen = read(newsockfd, buf, MAX_BYTES );
                        if( buflen == 0 ) break;

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
