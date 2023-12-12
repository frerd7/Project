/* decompress.cpp
 * Copyright (C) 2023  Frederick Valdez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <zip.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "url.h"

int desscompress(const char *input) 
{
    struct zip *archive;
    archive = zip_open(input, 0, NULL);
    if (archive == NULL) {
        fprintf(stderr, "Error opening ZIP file: %s\n", input);
        return 1;
    }

    int num_entries = zip_get_num_entries(archive, 0);
    printf("Number of files in the ZIP archive: %d\n", num_entries);

    const size_t chunk_size = 4 * 1024; // 4 KB (adjust as needed)
    char buffer[chunk_size];

    for (int i = 0; i < num_entries; i++) {
        struct zip_stat file_stat;
        zip_stat_init(&file_stat);

        if (zip_stat_index(archive, i, 0, &file_stat) != 0) {
            fprintf(stderr, "Error reading file stat for file %d\n", i);
            break;
        }
              
        const char* entry_name = zip_get_name(archive, i, 0);
        if (entry_name[strlen(entry_name) - 1] == '/') {
           mkdir(entry_name, 0777);
	   }

        struct zip_file *file = zip_fopen_index(archive, i, 0);
        if (file == NULL) {
            fprintf(stderr, "Error opening file %d\n", i);
            continue;
        }
        
         FILE *output_file = fopen(entry_name, "wb");
           if (output_file == NULL) {
            zip_fclose(file);
            continue;
          }

        zip_int64_t bytes_remaining = file_stat.size;

        while (bytes_remaining > 0) {
            size_t bytes_to_read = (size_t)(bytes_remaining < chunk_size ? bytes_remaining : chunk_size);
            zip_int64_t bytes_read = zip_fread(file, buffer, bytes_to_read);

            if (bytes_read <= 0) {
                fprintf(stderr, "Error reading from ZIP file\n");
                break;
            }

            if (fwrite(buffer, 1, (size_t)bytes_read, output_file) != (size_t)bytes_read) {
                fprintf(stderr, "Error writing to output file\n");
                break;
            }

            bytes_remaining -= bytes_read;
        }
        
        fclose(output_file);
        zip_fclose(file);
    }
    
    zip_close(archive);

    printf("Decompression complete.\n");
    remove(input);
    return 0;
}
