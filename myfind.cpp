#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <cstring>
#include <limits.h> 
#include <sys/stat.h>
#include <string>
#include <sys/wait.h>
#include <algorithm>

#define FILE_FOUND_EXIT_CODE 42

// Funktion für einen case-insensitive String Vergleich
int case_insensitive_cmp(const char *s1, const char *s2) {
    //solange beide strings noch zeichen enthalten
    while (*s1 && *s2) {
        //Buchstaben werden verglichen
        if (tolower(*s1) != tolower(*s2)) {
            return 0; //Falls string nicht gleich sind return Wert 0 
        }
        s1++;
        s2++;
    }
    return *s1 == *s2; //Beide Strings sollten gleich enden
}

//Funiktion für die Suche 
int find_file_recursive(const char *current_path, const char *target_filename, bool recursive, bool case_insensitive) {
    DIR *dirp;
    struct dirent *direntp;
    
    // Attempt to open the current directory
    if ((dirp = opendir(current_path)) == NULL) {
        // Suppress print to stderr for non-critical errors like permission denied
        return 0; 
    }

    // Loop through all entries in the directory
    while ((direntp = readdir(dirp)) != NULL) {
        // Skip "." and ".."
        if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
            continue;
        }

        // Check if the current entry's name matches the target file name
        if ((case_insensitive_cmp((direntp->d_name), target_filename) && case_insensitive) || strcmp(direntp->d_name, target_filename) == 0) {
            // File found!
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_path, direntp->d_name);
            printf("✅ FOUND: '%s' at %s\n", target_filename, full_path);
            
            // Close the directory before returning
            while ((closedir(dirp) == -1) && (errno == EINTR));
            return 1; // Signal that the file was found
        }

        // Construct the full path
        char full_path[1024]; 
        if ((size_t)snprintf(full_path, sizeof(full_path), "%s/%s", current_path, direntp->d_name) >= sizeof(full_path)) {
            continue;
        }

        // Check if the current entry is a directory and if we should recurse
        struct stat statbuf;
        if (stat(full_path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) && recursive) {
            // Recursively call the function for the subdirectory
            if (find_file_recursive(full_path, target_filename, recursive, case_insensitive) == 1) {
                // If found in a subdirectory, stop searching and return up the call stack
                while ((closedir(dirp) == -1) && (errno == EINTR));
                return 1;
            }
        }
    }
    
    // Close the directory
    while ((closedir(dirp) == -1) && (errno == EINTR));

    return 0; // File not found in this path
}

int main(int argc, char *argv[])
{
    int c;
    char *programm_name;
    unsigned short Counter_Option_R = 0;
    unsigned short Counter_Option_i = 0;
    programm_name = argv[0];

    while ((c = getopt(argc, argv, "Ri")) != EOF)
    {
        switch (c)
        {
            case '?':
                fprintf(stderr, "%s error: Unknown option.\n", programm_name);
                exit(1);
                /* Das break ist nach exit() eigentlich nicht mehr notwendig. */
                break;
            case 'R':
                Counter_Option_R++;
                fprintf(stdout, "Option -R detected.\n");
                break;
            case 'i':
                Counter_Option_i++;
                fprintf(stdout, "Option -i detected.\n");
                break;
            default:
                fprintf(stderr, "Usage: %s [-R] [-i] searchpath filename1 [filename2...]\n", argv[0]);
                exit(-1);
                assert(0);
        }
    }
    if (argc - optind < 2) {
        printf("Error: At least two required strings must be provided.\n");
        printf("Usage: %s [-R] [-i <value>] <required_path> <required_file> [optional_strings...]\n", argv[0]);
        return 1;
    }

    for (int i = optind + 2; i < argc; i++) {
        printf("Optional string %d: %s\n", i - optind + - 1, argv[i]);
    }


    int i;
    for (i =  2 + Counter_Option_i + Counter_Option_R; i < argc; i++) {
        const char *target_filename = argv[i];
        const char *search_path = argv[1 + Counter_Option_i + Counter_Option_R];
        pid_t pid = fork();
        if (pid < 0) {
            // Fork failed
            perror("fork");
            // If fork fails, we continue with the remaining files, but the parallel search is compromised
            continue; 
        } else if (pid == 0) {
            // --- Child Process ---
            printf("    [PID %d] Searching for: '%s'\n", getpid(), target_filename);
            
            int found = find_file_recursive(search_path, target_filename, Counter_Option_R > 0, Counter_Option_i > 0);
            
            if (found) {
                // If found, print the PID and exit with a special code
                printf("    ⭐ MATCH FOUND by PID: %d for file '%s'\n", getpid(), target_filename);
                exit(FILE_FOUND_EXIT_CODE);
            } else {
                // If not found, exit with success (0)
                printf("    [PID %d] Finished search for '%s' (Not Found).\n", getpid(), target_filename);
                exit(0);
            }

        }
        // --- Parent Process continues the loop to fork the next child ---
    }

    // --- Parent Process: Wait for all child processes to finish ---
    int status;
    pid_t wpid;
    int files_found_count = 0;
    
    // We wait for all children created (argc - 2 of them)
    int num_children = argc - 2 - Counter_Option_i - Counter_Option_R;
    for (i = 0; i < num_children; i++) {
        wpid = wait(&status);
        if (wpid > 0) {
            // Check if the child exited with our special "File Found" code
            if (WIFEXITED(status) && WEXITSTATUS(status) == FILE_FOUND_EXIT_CODE) {
                files_found_count++;
            }
        } else if (wpid == -1 && errno == ECHILD) {
             // No more children to wait for
             break;
        } else if (wpid == -1) {
            perror("wait");
        }
    }

    printf("----------------------------------------\n");
    printf("Summary: %d out of %d files were found.\n", files_found_count, num_children);
    printf("--- All parallel searches complete. ---\n");
    return 0;
}
