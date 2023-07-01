// Copyright: Mohammad Reza Bateni
// https://github.com/bateni1380/Multithread-Programming-In-C-And-CPP/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <sys/stat.h>
#include <regex.h>

#define MAX_LINE_LENGTH 256
#define MAX_THREADS 1000

const char* pattern;
const char* root_directory;
pthread_t threads[MAX_THREADS];
int thread_count = 0;
pthread_mutex_t mutex;
sem_t semaphore;

int isSearchingFile = 1;
int isShowingLine = 1;
int maxSearchDepth = 10;
int maxWorkingThreads = 10;
int isReverseGrep = 0;

void check_folder_name(char* path, char* f_name)
{
    regex_t regex;
    regcomp(&regex, pattern, REG_EXTENDED);
    int notRegxFound = regexec(&regex, f_name, 0, NULL, 0);
    if ((notRegxFound == 0 && isReverseGrep == 0)||(notRegxFound == 1 && isReverseGrep == 1))
        printf("(folder)%s: %s\n", path, f_name);
    regfree(&regex);
}

void* search_file(void* arg) 
{
    sem_wait(&semaphore); // Wait if there is no available worker

    char* path = (char*)arg;
    FILE* file = fopen(path, "r");
    if (file == NULL) 
    {
        pthread_mutex_lock(&mutex);
        printf("Cannot open file: %s\n", path);
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    regex_t regex;
    regcomp(&regex, pattern, REG_EXTENDED);

    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) 
    {
        line_number++;
        int notRegxFound = regexec(&regex, line, 0, NULL, 0);
        if ((notRegxFound == 0 && isReverseGrep == 0)||(notRegxFound == 1 && isReverseGrep == 1))
        {
            pthread_mutex_lock(&mutex);
            if(isReverseGrep == 1)
                printf("(reverse grep)");
            if(isShowingLine==1)
                printf("%s:%d: %s\n", path, line_number, line);
            else
                printf("%s: %s\n", path, line);
            pthread_mutex_unlock(&mutex);
        }
    }

    regfree(&regex);
    fclose(file);
    sem_post(&semaphore); // Release the semaphore once the function has completed
    pthread_exit(NULL);
}

void search_directory(const char* directory, int depth, int max_depth) 
{
    if (depth > max_depth) // Max depth of folders have been reached!
        return;

    DIR* dir = opendir(directory);
    if (dir == NULL) 
    {
        pthread_mutex_lock(&mutex);
        printf("Cannot open directory: %s\n", directory);
        pthread_mutex_unlock(&mutex);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) // Skip current folder and parent folder to avoid loop
            continue;

        char path[MAX_LINE_LENGTH + 1];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        struct stat entry_stat;
        if (stat(path, &entry_stat) == -1) // Get the path status
        {
            pthread_mutex_lock(&mutex);
            printf("Cannot get file status: %s\n", path);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) // It is a folder
        {
            if(isSearchingFile==0)
                check_folder_name(path, entry->d_name);
            search_directory(path, depth+1, max_depth);
        }
        else if (S_ISREG(entry_stat.st_mode) && isSearchingFile==1) // It is a regular file
        {
            char copied_path[MAX_LINE_LENGTH];
            strcpy(copied_path, path);
            pthread_create(&threads[thread_count], NULL, search_file, copied_path);
            thread_count++;
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) 
{
    // Set the root directory
    root_directory = "root";

    if (argc < 2)
    {
        printf("Usage: sudo ./grep pattern ...\n");
        return 1;
    }

    pattern = argv[1];
    if (argc >= 3)
        isSearchingFile = atoi(argv[2]); // If it's 0 then it searchs folder names
    if (argc >= 4)
        isShowingLine = atoi(argv[3]); // If it's 1 then it shows line of found pattern in file
    if (argc >= 5)
        maxSearchDepth = atoi(argv[4]); // The maximum depth to search in folders
    if (argc >= 6)
        maxWorkingThreads = atoi(argv[5]); // The maximum number of threads that can be executed simultaneously
    if (argc >= 7)
        isReverseGrep = atoi(argv[6]); // If it's 1 then it print's the lines that doesn't have the pattern in them

    pthread_mutex_init(&mutex, NULL); // Initialize the mutex that is responsable for preventing printing confussion
    sem_init(&semaphore, 0, maxWorkingThreads); // Initialize the semaphore to control number of working threads (workers)

    search_directory(root_directory, 0, maxSearchDepth);

    for (int i = 0; i < thread_count; i++) 
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&mutex); // Destroy the mutex
    sem_destroy(&semaphore); // Destroy the semaphore

    return 0;
}

