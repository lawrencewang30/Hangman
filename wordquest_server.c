#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_WORDS 15
#define MAX_CLIENTS 3

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

int num_clients = 0;

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void update_guessed_word(char *word, const char *actual_word, char guessed_letter) {
    for (int i = 0; i < strlen(actual_word); ++i) {
        if (tolower(actual_word[i]) == guessed_letter) {
            word[i] = actual_word[i];
        }
    }
}

void *handle_client(void *arg) {
    int newsockfd = *(int *)arg;
    free(arg);

    int n;
    int num_incorrect = 0;

    struct GameMsg gameMsg;
    struct RegularMsg regularMsg;

    int exitFlag = 0;
    int num_words = 0;

    FILE *wordquest_txt;
    char words[MAX_WORDS][1024];
    wordquest_txt = fopen("wordquest_words.txt", "r");
    if (wordquest_txt == NULL) {
        error("ERROR opening file");
    }

    while (fgets(words[num_words], 1024, wordquest_txt) != NULL) {
        num_words++;
        if (num_words >= MAX_WORDS) {
            break;
        }
    }
    fclose(wordquest_txt);

    if (num_words == 0) { // case if no words in txt file
        fprintf(stderr, "No words available from the file.\n");
        close(newsockfd);
        pthread_exit(NULL);
    }

    int random_index = rand() % num_words;
    char *game_word = words[random_index];
    game_word[strlen(game_word) - 1] = '\0';
    char guessed_word[1024];
    memset(guessed_word, '_', strlen(game_word));
    guessed_word[strlen(game_word)] = '\0';

    char incorrect_guesses[1024];
    memset(incorrect_guesses, 0, sizeof(incorrect_guesses));

    while (!exitFlag) {
        gameMsg.msgFlag = 0;
        gameMsg.game_word_length = strlen(game_word);
        gameMsg.numIncorrect = num_incorrect;
        memset(gameMsg.wordquestStatus, '_', gameMsg.game_word_length);
        gameMsg.wordquestStatus[gameMsg.game_word_length] = '\0';
        
        strncpy(gameMsg.incorrectGuesses, incorrect_guesses, sizeof(gameMsg.incorrectGuesses) - 1);
        gameMsg.incorrectGuesses[sizeof(gameMsg.incorrectGuesses) - 1] = '\0';
        
        n = send(newsockfd, &gameMsg, sizeof(gameMsg), 0);
        if (n < 0) {
            error("ERROR writing to socket");
        }

        while (1) {
            n = recv(newsockfd, &regularMsg, sizeof(regularMsg), 0);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            if (n == 0) {
                close(newsockfd);
                num_clients--;
                pthread_exit(NULL);
            }

            regularMsg.message[regularMsg.msgLength] = '\0';
            char letter_to_find = (char)regularMsg.message[0];


            if (strchr(game_word, letter_to_find) != NULL) {
                update_guessed_word(guessed_word, game_word, letter_to_find);
            }
            else {
                incorrect_guesses[num_incorrect] = letter_to_find;
                num_incorrect++;
            }

            gameMsg.msgFlag = 0;
            gameMsg.game_word_length = strlen(game_word);
            gameMsg.numIncorrect = num_incorrect;
            strcpy(gameMsg.wordquestStatus, guessed_word);
            gameMsg.wordquestStatus[gameMsg.game_word_length] = '\0';
            strncpy(gameMsg.incorrectGuesses, incorrect_guesses, sizeof(gameMsg.incorrectGuesses) - 1);
            gameMsg.incorrectGuesses[sizeof(gameMsg.incorrectGuesses) - 1] = '\0';

            if (num_incorrect == 6) {
                exitFlag = 1;
                gameMsg.msgFlag = strlen(game_word);
                strcpy(gameMsg.wordquestStatus, game_word);
                n = send(newsockfd, &gameMsg, sizeof(gameMsg), 0);

                regularMsg.msgLength = strlen("You Lose!");
                strcpy(regularMsg.message, "You Lose!");
                n = send(newsockfd, &regularMsg, sizeof(regularMsg), 0);
                if (n < 0) {
                    error("ERROR writing to socket");
                }

                regularMsg.msgLength = strlen("Game Over!");
                strcpy(regularMsg.message, "Game Over!");
                n = send(newsockfd, &regularMsg, sizeof(regularMsg), 0);
                if (n < 0) {
                    error("ERROR writing to socket");
                }
                break;
            }

            if (strcmp(gameMsg.wordquestStatus, game_word) == 0) {
                exitFlag = 1;
                gameMsg.msgFlag = strlen(game_word);
                strcpy(gameMsg.wordquestStatus, game_word);
                n = send(newsockfd, &gameMsg, sizeof(gameMsg), 0);

                regularMsg.msgLength = strlen("You Win!");
                strcpy(regularMsg.message, "You Win!");
                n = send(newsockfd, &regularMsg, sizeof(regularMsg), 0);
                if (n < 0) {
                    error("ERROR writing to socket");
                }

                regularMsg.msgLength = strlen("Game Over!");
                strcpy(regularMsg.message, "Game Over!");
                n = send(newsockfd, &regularMsg, sizeof(regularMsg), 0);
                if (n < 0) {
                    error("ERROR writing to socket");
                }
                break;
            }

            n = send(newsockfd, &gameMsg, sizeof(gameMsg), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }
        }
        close(newsockfd);
        num_clients--;
        pthread_exit(NULL);
    }
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int game_started = 0;
    //struct GameMsg gameMsg;
    //struct RegularMsg regularMsg;


    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    listen(sockfd, 3);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }

        if (num_clients >= MAX_CLIENTS) {
            int num_clients_n = htonl(num_clients);
            n = send(newsockfd, &num_clients_n, sizeof(num_clients_n), 0);
            if (n < 0) {
                error("ERROR writing to socket");
            }
            
            close(newsockfd);
            continue;
        }
        
        int num_clients_n = htonl(num_clients);
        n = send(newsockfd, &num_clients_n, sizeof(num_clients_n), 0);
        num_clients++;

        if (!game_started) {
            bzero(buffer, sizeof(buffer));
            n = recv(newsockfd, buffer, sizeof(buffer), 0);
            if (n < 0) {
                error("ERROR reading from socket");
            }

            if (strlen(buffer) == 0) {
                game_started = 1;
                pthread_t thread;
                int *newsockfd_ptr = malloc(sizeof(int));
                if (newsockfd_ptr == NULL) {
                    fprintf(stderr, "ERROR: Unable to allocate emory for new client socket\n");
                    close(newsockfd);
                    continue;
                }
                *newsockfd_ptr = newsockfd;
                if (pthread_create(&thread, NULL, handle_client, (void *)newsockfd_ptr) != 0) {
                    fprintf(stderr, "ERRPR: Unable to create thread for new client\n");
                    free(newsockfd_ptr);
                    close(newsockfd);
                    continue;
                }
                game_started = 0;
            }
            else {
                close(newsockfd);
                num_clients--;
                continue;
            }
        }
    }
    close(sockfd);
    return 0;
}