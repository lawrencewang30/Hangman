#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    struct GameMsg {
        int8_t msgFlag;
        int8_t game_word_length;
        int8_t numIncorrect;
        char wordquestStatus[1024];
        char incorrectGuesses[6];
    };

    struct RegularMsg {
        int8_t msgLength;
        char message[1024];
    };

    char buffer[1024];
    int received_num_clients;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    

    struct GameMsg gameMsg;
    struct RegularMsg regularMsg;

    n = recv(sockfd, &received_num_clients, sizeof(received_num_clients), 0);
    int num_clients = ntohl(received_num_clients);
    if (num_clients >= 3) {
        printf(">>>Sorry, the server is currently at max capacity! Please try again later :(\n");
        close(sockfd);
        exit(0);
    }

    printf(">>>Ready to start game? (y/n): ");

    bzero(buffer, 1024);
    if (fgets(buffer, 1023, stdin) == NULL) { // fgets encountered EOF (Ctrl + D)
        printf("\n");
        close(sockfd);
        exit(0);
}

    // Remove the newline character
    buffer[strcspn(buffer, "\n")] = 0;

    // Check if user input is valid
    if (strlen(buffer) == 0 || (buffer[0] != 'y' && buffer[0] != 'n')) {
        printf("Invalid input. Please enter 'y' or 'n'.\n");
        n = send(sockfd, "invalid input", strlen("invalid input"), 0);
        close(sockfd);
        printf("Connection terminated.\n");
        exit(0);
    }

    if (buffer[0] == 'y') {
        n = send(sockfd, "", sizeof(""), 0);
    }
    if (buffer[0] == 'n') {
        char letter[2] = {buffer[0], '\0'};
        n = send(sockfd, letter, strlen(letter), 0);

        close(sockfd);
        printf("Connection terminated.\n");
        exit(0);
    }

    n = recv(sockfd, &gameMsg, sizeof(gameMsg), 0);
    if (n < 0) {
        error("ERROR: did not receive message");
    }

    int length = 1024;
    gameMsg.wordquestStatus[gameMsg.game_word_length] = '\0';
    printf(">>>");
    for (int i = 0; i < length; ++i) {
        if (gameMsg.wordquestStatus[i] == '\0') {
            break;
        }
        printf("%c", gameMsg.wordquestStatus[i]);
        if (i < length - 1 && gameMsg.wordquestStatus[i + 1] != '\0') {
            printf(" ");
        }
    }
    printf("\n");

    printf(">>>Incorrect Guesses: ");
    for (int i = 0; i < length; ++i) {
        if (gameMsg.incorrectGuesses[i] == '\0') {
            break;
        }
        printf("%c", gameMsg.incorrectGuesses[i]);
        if (i < length - 1 && gameMsg.incorrectGuesses[i + 1] != '\0') {
            printf(" "); // Add space if it's not the last non-null character
        }
    }
    printf("\n>>>");

    while (1) {
        printf("\n>>>Letter to guess: ");
        bzero(buffer, 1024);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) { // fgets encountered EOF (Ctrl + D)
            printf("\n");
            break;
        }

        if (strlen(buffer) != 2 || !isalpha(buffer[0])) {
            printf(">>>Error! Please guess one letter.");
            continue;
        }

        regularMsg.msgLength = strlen(buffer) - 1;
        regularMsg.message[regularMsg.msgLength] = '\0';
        strcpy(regularMsg.message, buffer);

        n = send(sockfd, &regularMsg, sizeof(regularMsg), 0);
        if (n < 0) {
            error("ERROR writing to socket");
        }

        n = recv(sockfd, &gameMsg, sizeof(gameMsg), 0);
        if (n < 0) {
            error("ERROR reading from socket");
        }

        if (gameMsg.msgFlag != 0) {
            printf(">>>The word was %s\n", gameMsg.wordquestStatus);

            n = recv(sockfd, &regularMsg, sizeof(regularMsg), 0);
            regularMsg.message[regularMsg.msgLength] = '\0';
            printf(">>>%s\n", regularMsg.message);

            n = recv(sockfd, &regularMsg, sizeof(regularMsg), 0);
            regularMsg.message[regularMsg.msgLength] = '\0';
            printf(">>>%s\n", regularMsg.message);

            break;
        }
        int length = 1024;
        printf(">>>");
        for (int i = 0; i < length; ++i) {
            if (gameMsg.wordquestStatus[i] == '\0') {
                break;
            }
            printf("%c", gameMsg.wordquestStatus[i]);
            if (i < length - 1 && gameMsg.wordquestStatus[i + 1] != '\0') {
                printf(" "); // Add space if it's not the last non-null character
            }
        }
        printf("\n");
        printf(">>>Incorrect Guesses: ");
        for (int i = 0; i < length; ++i) {
            if (gameMsg.incorrectGuesses[i] == '\0') {
                break;
            }
            printf("%c", gameMsg.incorrectGuesses[i]);
            if (i < length && gameMsg.incorrectGuesses[i + 1] != '\0') {
                printf(" "); // Add space if it's not the last non-null character
            }
        }
        printf("\n>>>");
    }

    close(sockfd);
    return 0;
}