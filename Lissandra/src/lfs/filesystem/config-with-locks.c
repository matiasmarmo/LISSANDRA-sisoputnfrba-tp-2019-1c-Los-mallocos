#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/string.h>

#include "manejo-datos.h"

t_config *lfs_config_create_from_file(char *path, FILE *file) {
    fseek(file, 0, SEEK_SET);
    struct stat stat_file;
    stat(path, &stat_file);

    t_config *config = malloc(sizeof(t_config));

    config->path = strdup(path);
    config->properties = dictionary_create();

    char* buffer = calloc(1, stat_file.st_size + 1);
    fread(buffer, stat_file.st_size, 1, file);

    if (strstr(buffer, "\r\n")) {
        printf("\n\nconfig_create - WARNING: the file %s contains a \\r\\n sequence "
         "- the Windows new line sequence. The \\r characters will remain as part "
         "of the value, as Unix newlines consist of a single \\n. You can install "
         "and use `dos2unix` program to convert your files to Unix style.\n\n", path);
    }

    char** lines = string_split(buffer, "\n");

    void add_cofiguration(char *line) {
        if (!string_starts_with(line, "#")) {
            char** keyAndValue = string_n_split(line, 2, "=");
            dictionary_put(config->properties, keyAndValue[0], keyAndValue[1]);
            free(keyAndValue[0]);
            free(keyAndValue);
        }
    }
    string_iterate_lines(lines, add_cofiguration);
    string_iterate_lines(lines, (void*) free);

    free(lines);
    free(buffer);

    return config;
}

int lfs_config_save_in_file(t_config *self, FILE *file) {

    if(fseek(file, 0, SEEK_SET) < 0) {
        return -1;
    }
    int fd = fileno(file);
    if(fd < 0) {
        return -1;
    }
    if(ftruncate(fd, 0) < 0) {
        return -1;
    }
    
    char* lines = string_new();
    void add_line(char* key, void* value) {
        if(value == NULL) {
            return;
        }
        string_append_with_format(&lines, "%s=%s\n", key, (char*)value);
    }

    dictionary_iterator(self->properties, add_line);
    int result = fwrite(lines, strlen(lines), 1, file);
    free(lines);
    return result;
}