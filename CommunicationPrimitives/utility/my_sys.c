/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "my_sys.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>

//#include "myUtils.h"

#include "my_string.h"
#include "Yggdrasil/core/utils/hashfunctions.h"

int run_command(char* cmd, char* out, int size) {

    char* argv[] = {"sh", "-c", cmd, NULL};

    int fd[2];

    if(pipe(fd) == -1) {
        fprintf(stderr, "Pipe Failed" );
        exit(-1);
    }

    pid_t pid = fork();

    // child
    if (pid == 0) {
        close(fd[0]); // close reading end of the pipe
        dup2(fd[1], 1); // Redirect stdout to pipe

        execvp(argv[0], argv);

        // if execv fails
        exit(-1);
   }

   // parent
   else {
       // close writting end of the pipe
       close(fd[1]);

       waitpid(pid, 0, 0); // wait for child to exit

       // Read output of child
       int n = 0;
       if(out) {
           memset(out, 0, size);
           n = read(fd[0], out, size);
       }

       close(fd[0]);

       return n;
   }
}

char* build_path(char* file_path, char* folder, char* file_name) {
	bzero(file_path, PATH_MAX);

	strcpy(file_path, folder);

    int len = strlen(file_path);

	if(file_path[len-1] != '/') {
        file_path[len++] = '/';
    }
	memcpy(file_path + len, file_name, strlen(file_name)+1);

	return file_path;
}

hash_table* parse_configs(const char* file_path) {
    return parse_configs_order(file_path, NULL);
}

hash_table* parse_configs_order(const char* file_path, list** order) {

    FILE* file = fopen(file_path , "r");

    size_t max_len = 1000;
    char* line = malloc(max_len);
    int read = 0;

    if (file) {
        hash_table* table = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);

        //while (fscanf(file, "%s", line) != EOF) {
        while((read = getline(&line, &max_len, file)) != -1) {

            // If line is a comment or is empty, ignore
            if( line[0] == '\0' || isspace(line[0]) || line[0] == '#' /*|| (line[0] == '/' && line[1] == '/')*/ ) {
                continue;
            } else {
                char key[max_len];
                char value[max_len];

                char *ptr = strtok(line, "=");

                // Key
                if(ptr) {
                    str_trim(key, ptr);

                    ptr = strtok(NULL, "=");
                } else {
                    // error
                    continue;
                }

                // Value
                if(ptr) {
                    char *x = strchr(ptr, '#');
                    if(x) {
                        //int index = x - ptr;
                        *x = '\0';
                    }

                    str_trim(value, ptr);
                } else {
                    // error
                    continue;
                }

                int k_len = strlen(key)+1;
                char* k = malloc(k_len);
                memcpy(k, key, k_len);

                int v_len = strlen(value)+1;
                char* v = malloc(v_len);
                memcpy(v, value, v_len);

                hash_table_insert(table, k, v);

                if(order != NULL) {
                    char* k2 = malloc(k_len);
                    memcpy(k2, key, k_len);
                    list_add_item_to_tail(*order, k2);
                }
            }
        }
        free(line);
        fclose(file);

        return table;
    } else
        return NULL;
}

int r_mkdir(const char* path) {
    char tmp[PATH_MAX] = {0};
    size_t len = 0;
    struct stat st = {0};

    mode_t mode = 0777;

    strcpy(tmp, path);
    len = strlen(tmp);

    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for(char* ptr = tmp + 1; *ptr; ptr++) {
        if(*ptr == '/') {
            *ptr = 0;

            if (stat(tmp, &st) == -1) {
                errno = 0;
                if(mkdir(tmp, mode) != 0) {
                    //fprintf(stderr, "Could not create directory %s (errno = %s)\n", log_path, strerror(errno));
                    //exit(-1);
                    return errno;
                } else {
                    //printf("Creating dir %s ...\n", log_path);
                }
            }

            *ptr = '/';
        }
    }
    
    if (stat(tmp, &st) == -1) {
        errno = 0;
        if(mkdir(tmp, mode) != 0) {
            //fprintf(stderr, "Could not create directory %s (errno = %s)\n", log_path, strerror(errno));
            //exit(-1);
            return errno;
        } else {
            //printf("Creating dir %s ...\n", log_path);
        }
    }

    return 0;
}
