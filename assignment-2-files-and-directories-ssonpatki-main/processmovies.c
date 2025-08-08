// Name: Siya Sonpatki

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements.

// You can, and probably SHOULD, use a lot of your work from assignment 1 to
// get started on this assignment. In particular, when you get to the part
// about processing the selected file, you'll need to load and parse all of the
// data from the selected CSV file into your program. In assignment 1, you
// created structure types, linked lists, and a bunch of code to facilitate
// doing exactly that. Just copy and paste that work into here as a starting
// point :)

// This assignment has no requirements about linked lists, so you can
// use a dynamic array (and expand it with realloc(), for example) instead if
// you would like. But then you'd have to redo a bunch of work.

// NOTE: for DEBUGGING metadata use -g flag 
//    <gcc -g -std=gnu99 processmovies.c -o processmovies>

// Note: can also use gdb ./<executable> for debugging but requires syntax knowledge

// NOTE: to check memory leaks/errors use Valgrind ./<executable_name>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

struct movie {
    char *title;

    // year is 4-digit integer w/ range [1900,2021]
    int year;

    // at most 5 languages w/ 
    char *languages[5];

    // rating can have decimal so use double w/ %1lf specifer
    double rating;
    
    struct movie *next;
};

// Function used to free dynamically allocated memory
void free_movies(struct movie* head, int number_movies) {
	struct movie *current = head;

    while (current != NULL) {
        struct movie *temporary = current;
        current = current->next;

        free(temporary->title);

        // loop needed to array of char pointers
        for (int i = 0; i < 5; i++) {
            free(temporary->languages[i]);
        }
  
        free(temporary);
    }
}


// Function used to populate struct instance with line from data_file
struct movie *read_movie(FILE* data_file) {
	char* line_of_text = NULL;
	size_t buffer_size = 0;
	ssize_t line_length = getline(&line_of_text, &buffer_size, data_file);

	struct movie *m = malloc(sizeof(struct movie));

    // check is line is empty before using it to populate a struct instance
    if (line_length == -1) {
        free(line_of_text);
        free(m);
        return NULL;
    }

    // if the line has data, i.e., the line has some length
	if (line_length > 0) {
        
		line_of_text[line_length - 1] = '\0';
		line_length--;
		
        // tokenize the line and use it to seperate attribute data
		char* title = strtok(line_of_text, ",");
		char* year = strtok(NULL, ",");
        char* languages = strtok(NULL, ",");
        char* rating = strtok(NULL, ",");

		m->title = (char*) malloc(sizeof(char) * (strlen(title) + 1));
		strcpy(m->title, title);
        
        //atoi (stdlib function) converts string representation to int literal
		m->year = atoi(year);   

        int index = 0;
        char *language_token = strtok(languages, ";");
        while (language_token != NULL && index < 5) {
            // remove trailing newline
            language_token[strcspn(language_token, "\r\n")] = 0;

            m->languages[index] = (char*) malloc(sizeof(char) * (strlen(language_token) + 1));
		    strcpy(m->languages[index], language_token); 
            index++;
            language_token = strtok(NULL, ";");
        }
        
        // set the unused elements m->languages elements to null
        // (; index < 5; index++) = increment index from last known value till < 5
        for (; index < 5; index++) {
            m->languages[index] = NULL;
        }

        // stdlib function used to interpret contents of str pointer to a floating point number, 
        //returns value as a double
        m->rating = strtod(rating, NULL);  

        m->next = NULL;
	} 

	free(line_of_text);

	return m;
}

// Function to print linked list - used for debugging
void printList(struct movie *head) {
    struct movie *current = head;

    while (current->title != NULL) {
        printf("%d %.1lf %s [", current->year, current->rating, current->title);

        for (int i = 0; i < 5; i++) {
            if (current->languages[i] != NULL) {
                printf("%s, ", current->languages[i]);
            }
        }

        printf("]\n");
        current = current->next;
    }
}

void allMoviesByYear(struct movie *head, int number_movies, char* dir_name) {

    struct movie *current = head;
    int year_array[number_movies];  // max possible unique year values = number of movies
    int index = 0;

    // create an array of unique movie->year values from list
    while (current != NULL) {
        int already_in_array = 0;
        for (int i = 0; i < index; i++) {
            if(year_array[i] == current->year) {
                //printf("Year %d already in array", current->year);
                already_in_array = 1;
            }
        }

        // if new year value found
        if (already_in_array == 0) {
            year_array[index] = current->year;
            index++;
        }
        
        current = current->next;
    }

    mode_t original_mask = umask(0);    // disable preset permissions but record original umask

    for (int i = 0; i < index; i++) {
        current = head;
        int year = year_array[i];
        char file_path[100];

        snprintf(file_path, sizeof(file_path), "%s/%d.txt", dir_name, year);
        int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0640);

        FILE *new_file = fdopen(fd, "a");

        // only check movies from the same year
        while (current != NULL) {
            if (current->year == year_array[i]) {
                fprintf(new_file, "%s\n", current->title);
                //printf("Year: %d, Movie: %s\n", current->year, current->title);
            }
            current = current->next;
        }

        fclose(new_file);
    }

    umask(original_mask);   // restore original umask

    printf("\n");   //print new line to clean up output visually
}

// Function used to determine number of movies (struct instances) read from file
int tallyMovies(struct movie *head) {
    struct movie *current = head;
    int movieTotal = 0;

    while (current != NULL) {
        current = current->next;
        movieTotal++;
    }

    return movieTotal;
}


void populateList(char *csv_file, char* dir_name) {
    
    //printf("File name: %s \n\n", csv_file);

	FILE* data_file = fopen(csv_file, "r"); 
	if (!data_file) { 
		printf("Error opening file %s\n", csv_file);
	}

    struct movie *head = NULL;
	int number_movies = 0;

	char* dummy_line = NULL;
	size_t dummy_buffer_size;
	getline(&dummy_line, &dummy_buffer_size, data_file);
	free(dummy_line);

	while (!feof(data_file)) {
		struct movie *new_movie = read_movie(data_file);

        if (new_movie == NULL) {
            continue;
        }

        if (head == NULL) {
            head = new_movie;
        } else {
            struct movie *current = head;
            while (current->next != NULL) {
                current = current->next;
            }

            current->next = new_movie;
        }
	}

    number_movies = tallyMovies(head);
	
    // file data
    // printf("Processed file %s and parsed data for %d movies\n\n", csv_file, number_movies);
    
    /*
        DISPLAY OPTIONS FOR USER FUNCTIONALITIES
    */

    allMoviesByYear(head, number_movies, dir_name);

	free_movies(head, number_movies);
	head = NULL; 

	fclose(data_file); 
}


void processFile(char* file_name, int user_choice) {
    // create a new directory inside the current working directory
        // directory name should be <ONID>.movies.<RANDOM NUMBER>
        // note: for random number use: rand() % 100000
        // IMPORTANT: make sure directory permissions are set to rwxr-x--- (octal code 0750)
    
    int random_num = rand() % 100000;
    char dir_name[100];

    snprintf(dir_name, sizeof(dir_name), "sonpatks.movies.%d", random_num);

    mode_t original_mask = umask(0);    // disable preset permissions but record original umask

    int success = mkdir(dir_name, 0750);

    umask(original_mask);   // restore original umask

    if (success == 0) {
        printf("Created directory with name %s\n", dir_name);
    } else {
        printf("Error creating directory\n");
    }

    // Process the movies inside the specified file
    populateList(file_name, dir_name);
    
    // create a new file in the new directory for each year that 1+ movie was released
        // file name should be <YYYY>.txt
        // each file should contain a list of titles of all movies released in the 
            // specific year each on their own line
        // IMPORTANT: make sure file permissions are set to rw-r-----

}


void largestFile() {
    DIR* directory_stream = opendir("./");
    if (!directory_stream) {
        printf("Error: filed to open current directory\n");
    }

    const char *prefix = "movies_";
    const char *extension = ".csv";

    struct dirent* entry;

    int first = 1;
    char name_of_latest_file[256];
    int largest_file_bytes = 0;

   
    do {
        entry = readdir(directory_stream);
        if (entry) {
            //printf("%s\n", entry->d_name);    //[DEBUG]

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // check that file starts with movies_ (check first n = strlen(prefix) char of the file name)
            if (strncmp(entry->d_name, prefix, strlen(prefix)) != 0) {
                continue;
            }

            // check that file ends with .csv (check last n = strlen(extension) char of the file name)
            
            int len_file_name = strlen(entry->d_name);
            int min_len_name = strlen(prefix) + strlen(extension);
            int len_ext = strlen(extension);
            if (len_file_name <= min_len_name) {
                printf("File name <= to minimum length of name with prefix and extension\n");
                continue;
            }

            // [finish] check last 4 letters of file name are .csv otherwise skip
            if (strcmp(entry->d_name + len_file_name - len_ext, extension) != 0) {
                printf("Last 4 characters of file name are not .csv");
                continue;
            }

            char entry_path[2 + 256];

            strcpy(entry_path, entry->d_name);
            //strcat(entry_path, entry->d_name);

            struct stat entry_info;
            int result = lstat(entry_path, &entry_info);

            if (first || entry_info.st_size > largest_file_bytes) {
                first = 0;
                largest_file_bytes = entry_info.st_size;
                strcpy(name_of_latest_file, entry->d_name);
            }
        }
    } while (entry);
    
    printf("Now processing the chosen file named %s\n", name_of_latest_file);

    processFile(name_of_latest_file, 1);

    closedir(directory_stream);
}


void smallestFile() {
    DIR* directory_stream = opendir("./");
    if (!directory_stream) {
        printf("Error: filed to open current directory\n");
    }

    const char *prefix = "movies_";
    const char *extension = ".csv";

    struct dirent* entry;

    int first = 1;
    char name_of_latest_file[256];
    int smallest_file_bytes = 0;

   
    do {
        entry = readdir(directory_stream);
        if (entry) {
            // printf("%s\n", entry->d_name);    //[DEBUG]

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // check that file starts with movies_ (check first n = strlen(prefix) char of the file name)
            if (strncmp(entry->d_name, prefix, strlen(prefix)) != 0) {
                continue;
            }

            // check that file ends with .csv (check last n = strlen(extension) char of the file name)
            
            int len_file_name = strlen(entry->d_name);
            int min_len_name = strlen(prefix) + strlen(extension);
            int len_ext = strlen(extension);
            if (len_file_name <= min_len_name) {
                printf("File name <= to minimum length of name with prefix and extension\n");
                continue;
            }

            // [finish] check last 4 letters of file name are .csv otherwise skip
            if (strcmp(entry->d_name + len_file_name - len_ext, extension) != 0) {
                printf("Last 4 characters of file name are not .csv");
                continue;
            }

            char entry_path[2 + 256];

            strcpy(entry_path, entry->d_name);
            //strcat(entry_path, entry->d_name);

            struct stat entry_info;
            int result = lstat(entry_path, &entry_info);

            if (first || entry_info.st_size < smallest_file_bytes) {
                first = 0;
                smallest_file_bytes = entry_info.st_size;
                strcpy(name_of_latest_file, entry->d_name);
            }
        }
    } while (entry);
    
    printf("Now processing the chosen file named %s\n", name_of_latest_file);

    processFile(name_of_latest_file, 2);

    closedir(directory_stream);
}


void nameFile() {
    char file_name[256];
    printf("Enter the complete file name: ");
    scanf("%255s", &file_name);

    DIR* directory_stream = opendir("./");
    if (!directory_stream) {
        printf("Error: filed to open current directory\n");
    }

    struct dirent* entry;

    int first = 1;
    char name_of_latest_file[256];
    int file_found = 0;
   
    do {
        entry = readdir(directory_stream);
        if (entry) {
            //printf("%s\n", entry->d_name);    //[DEBUG]

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // check that file starts with movies_ (check first n = strlen(prefix) char of the file name)
            if (strcmp(entry->d_name, file_name) != 0) {
                continue;
            }

            // check that file ends with .csv (check last n = strlen(extension) char of the file name)
            
            strcpy(name_of_latest_file, entry->d_name);
            file_found += 1;
        }
    } while (entry);
    
    if (file_found != 0) {
        printf("Now processing the chosen file named %s\n", name_of_latest_file);
        processFile(name_of_latest_file, 1);
    } else {
        printf("The file %s was not found\n\n", file_name);
    }

    closedir(directory_stream);
}


void selectFile() {
    int user_choice;

    printf("What file do you want to process: \n");
    printf("1. Enter 1 to pick the largest file \n");
    printf("2. Enter 2 to pick the smallest file \n");
    printf("3. Enter 3 to specify the name of a file \n\n");

    printf("Enter a choice from 1 to 3: ");
    scanf("%d", &user_choice);

    while ((user_choice < 1) || (user_choice > 3)) {
        printf("Invalid choice \n");
        printf("Enter a choice from 1 to 3: ");
        scanf("%d", &user_choice);
    }

    switch(user_choice) {
        case 1:
            largestFile();
            break;
        case 2:
            smallestFile();
            break;
        case 3:
            nameFile();
            break;
        default:
            break;
    }

}


int main() {
    srand(time(NULL));  // use rand() to call random function

    int user_choice;

    do {
        printf("Search Options: \n");
        printf("1. Select file to process \n");
        printf("2. Exit the program \n\n");

        printf("Enter a choice from 1 or 2: ");
        scanf("%d", &user_choice);
        printf("\n");

        while ((user_choice < 1) || (user_choice > 2)) {
            printf("Invalid choice \n");
            printf("Enter a choice from 1 or 2: ");
            scanf("%d", &user_choice);
        }

        switch(user_choice) {
            case 1:
                selectFile();
                break;
            case 2:
                break;
            default:
                break;
        }

    } while (user_choice != 2);

	return 0;
}