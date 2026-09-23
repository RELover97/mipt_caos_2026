#include <unistd.h> // read / write, lseek
#include <fcntl.h>  // open / close
#include <stdio.h>
#include <stdlib.h> // exit

enum {
    WRONG_ARG_NUM = 3,
    CANNOT_OPEN_FILE ,
    CANNOT_GET_FILE_SIZE ,
    CANNOT_READ_CHAR,
    CANNOT_SET_FILE_POS,
    CANNOT_WRITE_CHAR    
};


// Read one byte from file with descriptor fd
// at position pos and write to c
int read_at(int fd, off_t pos, char *c)
{
    // set file position as pos from beginning of file (SEEK_SET)
    if (lseek(fd, pos, SEEK_SET) == (off_t) - 1) {
        printf("Cannot read a char from file\n");
        exit(CANNOT_SET_FILE_POS);
    }

    ssize_t r = read(fd, c, 1);
    if (r == 1) return 1; // success
    if (r == 0) return 0; // EOF
    return -1; // error
}

// Write one byte c to file with descriptor fd
// at position pos
int write_at(int fd, off_t pos, char c)
{
    // set file position as pos from beginning of file (SEEK_SET)
    if (lseek(fd, pos, SEEK_SET) == (off_t) - 1) {
        printf("Cannot set file position\n");
        exit(CANNOT_SET_FILE_POS);
    }

    ssize_t r = write(fd, &c, 1);
    return (r == 1) ? 0 : -1;
}


void reverse_segment(int fd, off_t start, off_t end)
{
    char left, right;
    while (start < end) {
        if (read_at(fd, start, &left) != 1) {
            printf("Cannot read a char on the left\n");
            exit(CANNOT_READ_CHAR);
        }

        if (read_at(fd, end, &right) != 1) {
            printf("Cannot read a char on the right\n");
            exit(CANNOT_READ_CHAR);
        }

        if (write_at(fd, start, right) < 0) {
            printf("Cannot write a char on the left\n");
            exit(CANNOT_WRITE_CHAR);
        }

        if (write_at(fd, end, left) < 0) {
            printf("Cannot write a char on the right\n");
            exit(CANNOT_WRITE_CHAR);
        }

        ++start;
        --end;
    }
}


int main(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i) {
        printf("%s\n", argv[i]);
    }

    if (argc != 2) {
        printf("Prog takes 2 args!\n");
        exit(WRONG_ARG_NUM);
    }

    int fd = open(argv[1], O_RDWR);

    if (fd < 0) {
        printf("Cannot open file %s\n", argv[1]);
        exit(CANNOT_OPEN_FILE);
    }

    // move position from SEEK_END on 0 bytes
    // and return absolute offset from beginning
    // of the file till the current position
    // SEEK_SET, SEEK_CUR, SEEK_END
    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize == (off_t) -1) {
        printf("Cannot get size of file %s\n", argv[1]);
        exit(CANNOT_GET_FILE_SIZE);
    }

    off_t pos = 0;
    off_t line_start = 0;
    char c;
    while (pos < filesize) {
        if (read_at(fd, pos, &c) != 1) {
            printf("Cannot read char from file");
            exit(CANNOT_READ_CHAR);
        }

        if (c == '\n') {
            if (pos > line_start) {
                // reverse non-empty line
                reverse_segment(fd, line_start, pos - 1);
            }
            line_start = pos + 1;
        }

        ++pos;
    }

    // last line does not have '\n'
    if (line_start < filesize) {
        reverse_segment(fd, line_start, filesize - 1);
    }

    close(fd);

    return 0;
}