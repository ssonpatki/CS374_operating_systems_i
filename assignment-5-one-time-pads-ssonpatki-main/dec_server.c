// decrypt server
// gcc dec_server.c -std=gnu99 -o dec_server to run

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


int charToIndex(char letter) {
	// if letter is [A, Z], then return letter's corresponding integer
	// i.e., A = 0, B = 1, ... Z = 25
	// so letter - A = letter's index in the alphabet
	if (letter >= 'A' && letter <= 'Z') {
		return letter - 'A';
	} else if (letter == ' ') {
		return 26;	// space is the 26 character
	} else {
		fprintf(stderr, "Invalid character: %c\n", letter);
		exit(1);
	}
}

char indexToChar(int index) {
	// if number is [1, 25], then return integers's corresponding letter
	// i.e., 0 = A, 1 = B, ... 25 = Z
	// so A + index = index's letter in the alphabet
	if (index >= 0 && index <= 25) {
		return 'A' + index;
	} else if (index == 26) {
		char space = ' ';
		return space;
	} else {
		fprintf(stderr, "Invalid index: %d\n", index);
		return(-1);
	}
}

char* decryption(char *cyphertext, char *key) {
	int len = strlen(cyphertext);

	// allocate memory for decrypted message and null terminator
	char* decrypted_text = malloc((len + 1) * sizeof(char));
	
	// cipher array stores sum of #s corresponding to plaintxt and key
	int cipher[len];

	// populate the ciper array
	for(int i = 0; i < len; i++) {
		// first map characters to their corresponding integer value
		int cyphertext_val = charToIndex(cyphertext[i]);
		int key_val = charToIndex(key[i]);

		// sum cyphertext and key integer values and wrap overflow
		cipher[i] = (cyphertext_val - key_val + 27) % 27;

		// map new integer value back to key to determine decrypted message
		decrypted_text[i] = indexToChar(cipher[i]);
	}

	decrypted_text[len] = '\0';
	
	return decrypted_text;
}

// revise fork_pids array when pid is removed
void fix_pid_array(pid_t pid_to_delete, int fork_pids[10], int *number_forks) {
    for (int i = 0; i < *number_forks; i++) {
        // find specific pid in array
        if (fork_pids[i] == pid_to_delete) {
            // shift array to "delete" pid
            for (int j = i; j < *number_forks - 1; j++) {
                fork_pids[j] = fork_pids[j + 1];
            }
            (*number_forks)--;
            break;
        }
    }
}

// function to check termination of child background processes
void check_background(int fork_pids[10], int *number_forks) {
    int i = 0;

    while (i < *number_forks) {
        int status_child;
        // get status of child using waitpid (but dont disrupt other processes)
        pid_t stat_info = waitpid(fork_pids[i], &status_child, WNOHANG);

        // background child process has terminated
        if (stat_info > 0) {
            fix_pid_array(stat_info, fork_pids, number_forks);
        } else {
            i++;
        }
    } 

}

void exitCmd(int fork_pids[10], int number_forks) {
    // send kill signal to process(es)
    // int kill(pid_t pid, int sig);

    for (int i = 0; i < number_forks; i++) {
        kill(fork_pids[i], SIGKILL);
    }
}

int main(int argc, char **argv) {
	// variables to help track forked processes
	int fork_pids[10];
	int number_forks = 0;

	if (argc < 1) {
        printf("Error with command line arguments\n");
        return 1;
    }

    char *listening_port = argv[1];
    //printf("Listening port is: %s \n\n", listening_port);

	int listening_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in address_info = {0};
	address_info.sin_family = AF_INET;
	address_info.sin_addr.s_addr = INADDR_ANY;
	// Port numbers can be between 0 and 65535
	address_info.sin_port = htons(atoi(listening_port));
	
	// associate socket with local address
	int bind_result = bind(listening_socket_fd,
		(const struct sockaddr*) &address_info,
		sizeof(address_info));

	if (bind_result != 0) {
		printf("Failed to bind!\n");
		exit(1);
	}

	// Enable the socket to listen for up to 5 connections at a time
	int listen_result = listen(listening_socket_fd, 5);
	if (listen_result != 0) {
		printf("Failed to listen!\n");
	}

	// infinite loop: will accept messages from clients until terminated
	while (1) {
		struct sockaddr client_addr;
		socklen_t client_addr_len;

		//when connection is made: call accept() to generate socket
		int communication_socket_fd = accept(listening_socket_fd, &client_addr, &client_addr_len);
		if (communication_socket_fd < 0) {
			printf("Failed to accept!\n");
		}

		pid_t fork_result = fork();
		// record fork_pids for later handling
		fork_pids[number_forks++] = fork_result;

		if (fork_result == 0) {
			// I am the child process

			// Receive message from client. Recall that it will end in
			// "@@"
			char message_received[2052] = {0};
			int total_bytes_received = 0;
			int max_bytes_remaining = 2052; // Null terminator is not sent in message
			while (strstr(message_received, "@@") == NULL) { // Until end of message
				// server recieve message from client
				int n_bytes_received = recv(
					communication_socket_fd,
					message_received + total_bytes_received, // ptr arithmetic
					max_bytes_remaining,
					0
				);
				if (n_bytes_received > 0) {
					total_bytes_received += n_bytes_received;
					max_bytes_remaining -= n_bytes_received;
				} else if (n_bytes_received == 0) {
					printf("The writer is terminating "
						"communication (closed socket or "
						"called shutdown())\n");
					return 1;
				} else {
					printf("Error on recv()!\n");
					return 1;
				}
			}
			
			// Strip "@@" off of the end of the message
			size_t len = strlen(message_received);
			message_received[len - 1] = '\0';
			message_received[len - 2] = '\0';
			
			char* client_name = strtok(message_received, "$");
			char* cyphertext = strtok(NULL, "$");
			char* key = strtok(NULL, "$");
			//printf("The client's cyphertext line is: %s\n", cyphertext);
			//printf("The client's key line is: %s\n", key);

			// if server did not recieve the corresponding client, send a rejection message
			if (strcmp(client_name, "dec_client") != 0) {
				char* reject_client = "REJECT@@";
				int total_bytes_sent = 0;
				int bytes_to_send = strlen(reject_client); // Don't send the null terminator
				int bytes_remaining = bytes_to_send;

				// keep track of bytes left to send back to the client
				while (total_bytes_sent < bytes_to_send) {
					int n_bytes_sent = send(
						communication_socket_fd,
						reject_client + total_bytes_sent, // ptr arithmetic
						bytes_remaining,
						0
					);
					if (n_bytes_sent != -1) {
						total_bytes_sent += n_bytes_sent;
						bytes_remaining -= n_bytes_sent;
					} else {
						printf("Error on send()!\n");
					}
				}

				// close communication socket
				close(communication_socket_fd);
				continue;
			}

			/* THIS SECTION EXECUTES IF THE CORRECT CLIENT SENDS A MESSAGE */
			
			// Send response
			char* decrypted_message = decryption(cyphertext, key);
			//printf("decrypted message: %s", decrypted_message);

			char* message_to_send = malloc(strlen(decrypted_message) + 3); // need to add @@ and null terminator
			strcpy(message_to_send, decrypted_message);
			strcat(message_to_send, "@@");
			
			int total_bytes_sent = 0;
			int bytes_to_send = strlen(message_to_send); // Don't send the null terminator
			int bytes_remaining = bytes_to_send;
			// continue sending bytes so long as the full message hasnt been sent
			while (total_bytes_sent < bytes_to_send) {
				// send information from server to client
				int n_bytes_sent = send(
					communication_socket_fd,
					message_to_send + total_bytes_sent, // ptr arithmetic
					bytes_remaining,
					0
				);
				if (n_bytes_sent != -1) {
					total_bytes_sent += n_bytes_sent;
					bytes_remaining -= n_bytes_sent;
				} else {
					printf("Error on send()!\n");
				}
			}

			// use shutdown to stop recieving or writing data into the socket
			shutdown(communication_socket_fd, SHUT_RDWR);

			free(message_to_send);
			exit(0);
		} else {
			// I am the parent process

			// close the communication socket
			close(communication_socket_fd);
			check_background(fork_pids, &number_forks);
		}
	
	}

	exitCmd(fork_pids, number_forks); // kill child processes
	// read, write
	// recv(), send()
}
