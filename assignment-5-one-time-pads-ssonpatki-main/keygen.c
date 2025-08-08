/* 
    Purpose: randomly generate a one-time pad key of the specified length and print it to 
standard output.

    Note: one-time pad key should only contain characters from the set of 27 allowable
characters (capital letters and spaces)

    Note: Only the generated key should be printed to standard output. Any error messages 
printed by keygen should be printed to standard error, not standard output.

    Shell command: ./keygen <keylength> > <output_file> (keylength <= 1024)

*/

// to run: gcc keygen.c -std=gnu99 -o keygen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>


int main(int argc, char **argv) {
    srand(time(NULL));  // use rand() to call random function

	if (argc < 1) {
        fprintf(stderr, "Error due to command line arguments\n");
        return 1;
    }

    // convert ASCII char to integer using atoi
    int keylength = atoi(argv[1]);

    if (keylength == 0) {
        fprintf(stderr, "Keylength is too short%d", keylength);
        return 1;
    }

    // 1025 characters to account for null terminator
    char one_time_pad[1025] = {0};
    char allowable_chars[27] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', ' '
    };

    // create a key (one-time pad) of requested length by mapping
    // random generated integer to char in char array
    for (int i = 0; i < keylength; i++) {
        int index = rand() % 27;
        one_time_pad[i] = allowable_chars[index];
    }
    
    one_time_pad[keylength] = '\0';

    // print one-time pad to standard out
    fprintf(stdout, "%s\n", one_time_pad);
	
    return 0;
}
