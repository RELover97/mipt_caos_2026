#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int read_byte(int fd, off_t offset, char *ch)
{
    for (;;) {
        // read no more than 1 bytes to ch from fd
        // at offset
        ssize_t n = pread(fd, ch, 1, offset);

        if (n == 1)
            return 0;

        if (n == -1) {
            perror("pread");
        } else {
            fprintf(stderr, "Unexpected EOF\n");
        }

        return -1;
    }
}

static int write_byte(int fd, off_t offset, char ch)
{
    for (;;) {
        ssize_t n = pwrite(fd, &ch, 1, offset);

        if (n == 1)
            return 0;

        if (n == -1) {
            perror("pwrite");
        } else {
            fprintf(stderr, "write failed\n");
        }

        return -1;
    }
}

static int reverse_range(int fd, off_t begin, off_t end)
{
    // Reverse range [begin, end).
    off_t left = begin;
    off_t right = end;

    while (left < right) {
        --right;

        if (left >= right)
            break;

        char left_byte;
        char right_byte;

        if (read_byte(fd, left, &left_byte) == -1)
            return -1;

        if (read_byte(fd, right, &right_byte) == -1)
            return -1;

        if (write_byte(fd, left, right_byte) == -1)
            return -1;

        if (write_byte(fd, right, left_byte) == -1)
            return -1;

        ++left;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDWR);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Not a regular file\n"); // perror(...)
        close(fd);
        return EXIT_FAILURE;
    }

    off_t file_size = st.st_size;
    off_t line_start = 0;
    for (off_t pos = 0; pos < file_size; ++pos) {
        char ch;

        if (read_byte(fd, pos, &ch) == -1) {
            close(fd);
            return EXIT_FAILURE;
        }

        if (ch != '\n')
            continue;

        //  Line has range [line_start, pos)
        // '\n' is not reversed
        if (reverse_range(fd, line_start, pos) == -1) {
            close(fd);
            return EXIT_FAILURE;
        }
        line_start = pos + 1; // skip '\n'
    }

    // The last line cannot have '\n'
    if (line_start < file_size) {
        if (reverse_range(fd, line_start, file_size) == -1) {
            close(fd);
            return EXIT_FAILURE;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}