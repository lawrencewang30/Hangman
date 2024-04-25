# WordQuest
This program is a Hangman game implemented in C that allows up to 3 clients simultaneuously to connect with the game server.  
The server allows connections from any IP address, and all clients will connect with the same port number specified by the server.  
Each client has up to 6 incorrect guesses (one letter at a time) before they lose the game; if the word is fully guessed beforehand they win.  
The program accounts for any sudden disconnections made by the client, which thus frees up a space for other potential clients to connect.  
The words chosen by the server is based on the hangman_words.txt file. This can be adjusted according to the user's preferred words.  
If the server is already full and a 4th client tries to join, they will be greeted with a server overloaded message and instantly disconnected.

Executing the program (executables):
make all

To run wordquest_server.c: 
./wordquest_server <port_number>

To run wordquest_client.c (for each corresponding client):
./wordquest_client <IP_address> <port_number>