// TODO Name: Siya Sonpatki

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements. Refer to the example-directory/ contents for some example
// code to get you started.

// !!! USE VALGRIND TO CHECK MEMORY LEAKS !!!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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

/*

    FOLLOWING FUNCTIONS ARE FOR MAIN USER FUNCTIONALITIES

*/

// Function shows movies released in the specified year
void moviesByRelease(struct movie *head) {
    //printf("Inside moviesByRelease function \n"); // [DEBUG]

    int user_choice;
    printf("Enter the year for which you want to see movies [1900-2021]: ");
    scanf("%d", &user_choice);

    while ((user_choice <= 1900) || (user_choice >= 2021)) {
        printf("Invalid choice \n");
        printf("Enter a year from 1900 to 2021: ");
        scanf("%d", &user_choice);
    }

    struct movie *current = head;
    int movies_from_year = 0;

    while (current != NULL) {
        if (current->year == user_choice) {
            printf("%s\n", current->title);

            movies_from_year++;
        }
        current = current->next;
    }

    if (movies_from_year == 0) {
        printf("No data about movies released in the year %d\n", user_choice);
    }

    printf("\n");   //print new line to clean up output visually
}

// Function shows highest rated movie for each year
void moviesByRating(struct movie *head, int number_movies) {
    //printf("Inside moviesByRating function \n");  // [DEBUG]
    
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

    // find highest rated movie in a given year
    for (int i = 0; i < index; i++) {
        // keep track of highest rating
        double highest_rating = 0;
        // keep track of movie with highest rating
        struct movie *highest_movie = NULL;
        current = head;

        // only check movies from the same year
        while (current != NULL) {

            if (current->year == year_array[i]) {
                // compare current->rating with current->next->rating
                if (current->rating > highest_rating) {
                    highest_rating = current->rating;
                    highest_movie = current; 
                } 
                //printf("Highest Rating: %.1lf, Current Rating: %.1lf", highest_rating, current->rating);
            }
            
            current = current->next;
        }
        // highest = highest rated movie in the given year
        if (highest_movie != NULL) {
            printf("%d %.1lf %s\n", highest_movie->year, highest_movie->rating, highest_movie->title);
        }
    }

    printf("\n");   //print new line to clean up output visually
}


// Function removes extra non-alphabetic characters from original string to new clean string
// Note: will be used for cmp user_input to movies->languages[i] string for retrieve movies by language

void cleanString(char *clean, char *original) {
    int index = 0;
    // for all characters in original string - loop until reach null terminator
    for (int i = 0; original[i] != '\0'; i++) {

        // check if the character is in the alphabet (between [A,Z] or [a,z])
        if ((original[i] >= 'A' && original[i] <= 'Z') ||
        (original[i] >= 'a' && original[i] <= 'z')) {
            clean[index++] = original[i];
        }
    }
    clean[index] = '\0'; // end string with null terminator
}

// Function shows the title and year of release of all movies in a specific language
void moviesByLanguage(struct movie *head) {
    //printf("Inside moviesByLanguage function \n");    // [DEBUG]

    char user_choice[20];
    printf("Enter the language for which you want to see movies: ");
    scanf("%s", user_choice);

    struct movie *current = head;
    int movies_found = 0; 

    //printf("User's input: %s\n", user_choice);    // [DEBUG]

    while (current != NULL) {

        user_choice[strcspn(user_choice, "\n")] = 0;    // remove trailing newline
        
        // struct declaration of char *languages[5] so max 5 possible languages
        for (int i = 0; i < 5; i++) {   

            if (current->languages[i] != NULL) {
                char cleaned_string[20];
                cleanString(cleaned_string, current->languages[i]);
                //printf("User: %s, Cleaned: %s\n", user_choice, cleaned_string);  // [DEBUG]
                
                int cmp = strcmp(cleaned_string, user_choice);
                //printf("Cmp value: %d\n", cmp);  // [DEBUG]

                if (cmp == 0) {
                    printf("%d %s \n", current->year, current->title);
                    movies_found++;
                }
            }
        }

        current = current->next;
    }
    
    if (movies_found == 0) {
        printf("No data about movies released in %s\n", user_choice);
    }

    printf("\n");   //print new line to clean up output visually
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


int main(int argc, char** argv) {

    /*
        POPULATE THE LINKED LIST OF MOVIES FROM FILE
    */
	    if (argc < 2) {
        printf("Error with command line arguments\n");
        return 1;
    }

    char *csv_file = argv[1];

    /*
	char csv_file[50];
    printf("Enter the name of the CSV file you would like to use: ");
    scanf("%s", csv_file);
    printf("File name: %s \n\n", csv_file);
    */

    //printf("File name: %s \n\n", csv_file);

	FILE* data_file = fopen(csv_file, "r"); 
	if (!data_file) { 
		printf("Error opening file %s\n", argv[1]);
		return 1;
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
    printf("Processed file %s and parsed data for %d movies\n\n", csv_file, number_movies);
    

    /*
        DISPLAY OPTIONS FOR USER FUNCTIONALITIES
    */

    int user_choice;

    do {
        printf("Search Options: \n");
        printf("1. Show movies released in a specified year \n");
        printf("2. Show highest rated movie for each year \n");
        printf("3. Show the title and year of release of all movies in a specific language \n");
        printf("4. Exit from the program \n\n");

        printf("Enter a choice from 1 to 4: ");
        scanf("%d", &user_choice);

        while ((user_choice < 1) || (user_choice > 4)) {
            printf("Invalid choice \n");
            printf("Enter a choice from 1 to 4: ");
            scanf("%d", &user_choice);
        }

        switch(user_choice) {
            case 1:
                moviesByRelease(head);
                break;
            case 2:
                moviesByRating(head, number_movies);
                break;
            case 3:
                moviesByLanguage(head);
                break;
            case 4:
                //printList(head)   // [DEBUG] Used to check linked list (causes memory errors)
                break;
            default:
                break;
        }

    } while (user_choice != 4);

	free_movies(head, number_movies);
	head = NULL; 

	fclose(data_file); 

	return 0;
}
