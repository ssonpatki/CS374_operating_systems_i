// decrypt client
// gcc dec_client.c -std=gnu99 -o dec_client to run

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

int invalidChar(char* line_of_text) {
	int len = strlen(line_of_text);

	// array of valid characters
	char allowable_chars[27] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', ' '
    };


	// loop for each character of the text
	for (int i = 0; i < len; i++) {
		int invalid_char = 1;
		// loop for each valid character 
		for (int j = 0; j < 27; j++) {
			// check to see if this character is an allowable char
			if (line_of_text[i] == allowable_chars[j]) {
				invalid_char = 0;
				break;
			}
		}
		if (invalid_char == 1) {
			//fprintf(stderr, "invalid char %c\n", line_of_text[i]);
			return 1;
		}
	}

	return 0;
}


int main(int argc, char **argv) {
// shell command used to execute client:
// $ ./enc_client <ciphertext> <key> <port>

	if (argc < 4) {
        fprintf(stderr, "Error with command line arguments\n");
        return 1;
    }

	// store commpand line args as variables
    char *ciphertext_file = argv[1];
    char *key_file = argv[2];
    char *port = argv[3];

    //printf("ciphertext is: %s \n\n", ciphertext_file);
    //printf("Key is: %s \n\n", key_file);
    //printf("Port is: %s \n\n", port);

// read ciphertext from given file
    FILE* data_file = fopen(ciphertext_file, "r"); 
	if (!data_file) { 
		fprintf(stderr, "Error opening file %s\n", ciphertext_file);
		return 1;
	}
    
	// read text from ciphertext file
	char* ciphertext_line = NULL;
	size_t ciphertext_buffer_size;
	ssize_t line_length = getline(&ciphertext_line, &ciphertext_buffer_size, data_file);

	// check is line is empty before using it to populate a struct instance
    if (line_length == -1) {
        free(ciphertext_line);
        fprintf(stderr, "Line is empty\n");
        exit(1);
    }

    //printf("[%s] is contained in file\n", ciphertext_line);
    fclose(data_file); 

// read key from given file
    data_file = fopen(key_file, "r"); 
	if (!data_file) { 
		fprintf(stderr, "Error opening file %s\n", key_file);
		return 1;
	}
    
	char* key_line = NULL;
	size_t key_buffer_size;
	line_length = getline(&key_line, &key_buffer_size, data_file);

	// check is line is empty before using it to populate a struct instance
    if (line_length == -1) {
        free(key_line);
        fprintf(stderr, "Line is empty\n");
        exit(1);
    }

    //printf("[%s] is contained in file\n", key_line);
    fclose(data_file);


// create client socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		fprintf(stderr, "Failed to create socket!\n");
		return 1;
	}
	
	// Get linked list of addrinfo structures for all TCP/IP server sockets
	// listening on localhost (this machine), port 51728
	struct addrinfo* server_addr_list = NULL;
	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	// use getaddr info if don't know the server's ip but you know the domain name
	int info_result = getaddrinfo(
		"localhost",
		port,
		&hints,
		&server_addr_list
	);
	if (info_result != 0) {
		fprintf(stderr, "Error on getaddrinfo!\n");
		return 1;
	}
	
	// Keep trying to connect until we succeed or until we've tried all of
	// the addresses in the list. In practice, if there's a TCP/IP server
	// running on port 51728 on this machine, the list will have a single
	// addrinfo structure in it referencing that server's listening socket.
	// Else, the list will be empty (in which case we probably would have
	// already printed an error and exited in the previous step)
	int connect_result = -1;
	struct addrinfo* itr = server_addr_list;
	while (itr != NULL && connect_result == -1) {
		connect_result = connect(
			socket_fd,
			itr->ai_addr,
			itr->ai_addrlen
		);
		itr = itr->ai_next;
	}

	// If connect_result is still -1, then we failed to connect
	if (connect_result == -1) {
		fprintf(stderr, "Error on connect!\n");
		exit(2);
	}

	// remove trailing newline char
	ciphertext_line[strcspn(ciphertext_line, "\r\n")] = '\0';
	key_line[strcspn(key_line, "\r\n")] = '\0';

	// check for any invalid characters in ciphertext or key
	int ciphertext_invalid = invalidChar(ciphertext_line);
	int key_invalid = invalidChar(key_line);
	if (ciphertext_invalid == 1) {
		fprintf(stderr, "Found invalid character in ciphertext file\n");
		exit(1);
	} else if (key_invalid == 1) {
		fprintf(stderr, "Found invalid character in key file\n");
		exit(1);
	}

	// get length of each string
	size_t ciphertext_len = strlen(ciphertext_line);
	size_t key_len = strlen(key_line);

	// exit if the key length is shorter than length of ciphertext
	if (key_len < ciphertext_len) {
		fprintf(stderr, "Error: key is shorter than ciphertext\n");
		exit(1);
	}

	// Message can be at most 2063 characters, including client name, 2-$, @@, and null terminator
	char* client_name = "dec_client";
	char message_to_send[2063] = {0};
	// snprintf - redirects output of printf() function onto buffer
	// i.e., creates a new string of text with the same format if printf() was used instead
	snprintf(message_to_send, sizeof(message_to_send), "%s$%s$%s@@", client_name, ciphertext_line, key_line);

	//printf("message to send: %s", message_to_send);

	// Send the message
	int total_bytes_sent = 0;
	int bytes_to_send = strlen(message_to_send); // Don't send the null terminator
	int bytes_remaining = bytes_to_send;
	// continue sending bytes so long as the full message hasnt been sent
	while (total_bytes_sent < bytes_to_send) {
		// send information from client to server
		int n_bytes_sent = send(
			socket_fd,
			message_to_send + total_bytes_sent, // ptr arithmetic
			bytes_remaining,
			0
		);
		if (n_bytes_sent != -1) {
			total_bytes_sent += n_bytes_sent;
			bytes_remaining -= n_bytes_sent;
		} else {
			fprintf(stderr, "Error on send()!\n");
		}
	}

	// Receive response. Suppose it has the same requirements (same max
	// size, ends in @@)
	char message_received[2052] = {0};
	// temporary buffer to recv message in parts - helps with concurrency
	char temporary_buffer[512] = {0};
	int total_bytes_received = 0;
	int max_bytes_remaining = 2052; // Null terminator is not sent in message
	while (strstr(message_received, "@@") == NULL) { // Until end of message
		// client recieve message from server
		int n_bytes_received = recv(
			socket_fd,
			temporary_buffer, // ptr arithmetic
			sizeof(temporary_buffer) - 1,
			0
		);
		if (n_bytes_received > 0) {
			// append n bytes from temporary_buffer to message_recieved
			strncat(message_received, temporary_buffer, n_bytes_received);
			total_bytes_received += n_bytes_received;
			//max_bytes_remaining -= n_bytes_received;
		} else if (n_bytes_received == 0) {
			fprintf(stderr, "The writer is terminating communication (closed"
				"socket or called shutdown())\n");
			return 1;
		} else {
			fprintf(stderr, "Error on recv()!\n");
			return 1;
		}
	}
	// Strip "@@" off of the end of the message
	size_t len = strlen(message_received);
	message_received[len - 1] = '\0';
	message_received[len - 2] = '\0';

	if(strcmp(message_received, "REJECT") == 0) {
		fprintf(stderr, "Error: server has rejected the client");
		exit(2);
	}

	//response recieved from server
	fprintf(stdout, "%s\n", message_received);
	free(ciphertext_line);
	free(key_line);

	shutdown(socket_fd, SHUT_RDWR);
	close(socket_fd);
	freeaddrinfo(server_addr_list);

	exit(0);
}
