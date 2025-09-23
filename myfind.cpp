#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>


void print_usage(char *programm_name)
{
    printf("Usage: %s [-h] [-v] [-f dateiname]\n\n", programm_name);
    return;
}


int main(int argc, char *argv[])
{
    int c;
    char *dateiname;
    char *programm_name;

    programm_name = argv[0];

    while ((c = getopt(argc, argv, "Ri")) != EOF)
    {
        switch (c)
        {
            case '?':
                fprintf(stderr, "%s error: Unknown option.\n", programm_name);
                print_usage(programm_name);
                exit(1);
                /* Das break ist nach exit() eigentlich nicht mehr notwendig. */
                break;
            case 'R':
                fprintf(stdout, "Option -R detected.\n");
                break;
            case 'i':
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
        printf("Usage: %s [-R] [-i <value>] <required_string1> <required_string2> [optional_strings...]\n", argv[0]);
        return 1;
    }

    printf("Filepath string %d: %s\n", 3, argv[3]);
    printf("File string %d: %s\n", 4, argv[4]);
    for (int i = optind + 2; i < argc; i++) {
        printf("Optional string %d: %s\n", i - optind + - 1, argv[i]);
    }

    return 0;



}