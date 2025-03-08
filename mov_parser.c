#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE (1024 * 1024 * 64)

uint32_t read_u32_be(char *buf) {
    return ((uint8_t)buf[0] << (8*3)) |
           ((uint8_t)buf[1] << (8*2)) |
           ((uint8_t)buf[2] << (8*1)) |
           ((uint8_t)buf[3] << (8*0)) ;
}

uint64_t read_u64_be(char *buf) {
    return ((uint64_t)(uint8_t)buf[0] << (8*7)) |
           ((uint64_t)(uint8_t)buf[1] << (8*6)) |
           ((uint64_t)(uint8_t)buf[2] << (8*5)) |
           ((uint64_t)(uint8_t)buf[3] << (8*4)) |
           ((uint64_t)(uint8_t)buf[4] << (8*3)) |
           ((uint64_t)(uint8_t)buf[5] << (8*2)) |
           ((uint64_t)(uint8_t)buf[6] << (8*1)) |
           ((uint64_t)(uint8_t)buf[7] << (8*0)) ;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("USAGE: %s <file>\n", argv[0]);
        return 1;
    }

    char *buf = malloc(BUFFER_SIZE);
    if (!buf) {
        perror("malloc failed");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        free(buf);
        return 1;
    }

    ssize_t bytes_read = read(fd, buf, BUFFER_SIZE);
    if (bytes_read <= 0) {
        perror("read failed");
        close(fd);
        free(buf);
        return 1;
    }

    printf("bytes_read = %zd\n", bytes_read);

    size_t i = 0;
    while (i + 8 <= (size_t)bytes_read) {
        uint32_t atom_size = read_u32_be(buf + i);
        char atom_type[5] = {0};
        atom_type[0] = buf[i+4];
        atom_type[1] = buf[i+5];
        atom_type[2] = buf[i+6];
        atom_type[3] = buf[i+7];
        atom_type[4] = '\0';

        if (atom_size == 1) {
            if (i + 16 > (size_t)bytes_read) {
                printf("  Not enough data for extended size, stopping.\n");
                break;
            }
            atom_size = read_u64_be(buf + i + 8);
            printf("Found atom of type %s with extended size: %llu\n", atom_type, (unsigned long long)atom_size);
        } else {
            printf("Found atom of type %s with size: %u\n", atom_type, atom_size);
        }

        if (atom_size < 8) {
            printf("Invalid atom size (%u), stopping parsing.\n", atom_size);
            break;
        }

        if (atom_size > (size_t)bytes_read - i) {
            printf("Atom size %u exceeds remaining buffer (%zu), stopping.\n", atom_size, bytes_read - i);
            break;
        }

        i += atom_size;
    }

    close(fd);
    free(buf);
    return 0;
}
