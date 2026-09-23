#include <dirent.h>      // opendir(), readdir(), closedir()
#include <errno.h>       // errno
#include <inttypes.h>    // PRIuMAX
#include <limits.h>      // PATH_MAX
#include <stdint.h>      // SIZE_MAX
#include <stdio.h>       // printf(), perror()
#include <stdlib.h>      // malloc(), free(), EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>      // strlen(), strcpy(), strcat()
#include <sys/stat.h>    // stat(), lstat(), struct stat, S_ISREG(...)
#include <sys/types.h>   // mode_t, off_t
#include <time.h>        // localtime_r(), strftime()
#include <unistd.h>      // access-related POSIX definitions

typedef struct {
    uintmax_t regular_files;   // количество обычных файлов
    uintmax_t directories;     // количество каталогов
    uintmax_t symlinks;        // количество символических ссылок
    uintmax_t other;           // остальные типы файлов
    uintmax_t total_size;      // суммарный размер обычных файлов
    uintmax_t errors;          // количество ошибок доступа
} Stats;

// Return string file type
const char *file_type(mode_t mode)
{
    if (S_ISREG(mode)) {
        return "FILE";
    }

    if (S_ISDIR(mode)) {
        return "DIR ";
    }

    if (S_ISLNK(mode)) {
        return "LINK";
    }

    return "OTHER";
}


// Transform access rights to:
// rwxr-xr--
void format_permissions(mode_t mode, char permissions[10])
{
    permissions[0] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[1] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[2] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[3] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[4] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[5] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[6] = (mode & S_IROTH) ? 'r' : '-';
    permissions[7] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[8] = (mode & S_IXOTH) ? 'x' : '-';

    permissions[9] = '\0';
}



// Return last file modification time
void print_time(time_t time_value)
{
    struct tm tm_value;
    char buffer[32];

    if (localtime_r(&time_value, &tm_value) == NULL) {
        printf("unknown");
        return;
    }

    if (strftime(buffer, sizeof(buffer),
                 "%Y-%m-%d %H:%M:%S",
                 &tm_value) == 0) {
        printf("unknown");
        return;
    }

    printf("%s", buffer);
}


// Create path to file:
// directory + "/" + filename
char *join_path(const char *directory, const char *name)
{
    size_t dir_len = strlen(directory);
    size_t name_len = strlen(name);

    // dir_len + '/' (if needed) + name_len + '\0'
    // Check size_t overflow
    int need_slash = (dir_len > 0 && directory[dir_len - 1] != '/');

    if (dir_len > SIZE_MAX - name_len - (size_t) need_slash - 1) {
        return NULL;
    }

    size_t total = dir_len + (size_t) need_slash + name_len + 1;

    char *result = malloc(total);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, directory, dir_len);

    size_t pos = dir_len;

    if (need_slash) {
        result[pos++] = '/';
    }

    memcpy(result + pos, name, name_len);
    result[pos + name_len] = '\0';

    return result;
}


// Print file info
void print_file_info(const char *path, const struct stat *st)
{
    char permissions[10];
    format_permissions(st->st_mode, permissions);

    printf("%-5s  %-9s  links=%-3ju  uid=%-5ju  gid=%-5ju  "
           "size=%-10ju  mtime=",
           file_type(st->st_mode),
           permissions,
           (uintmax_t) st->st_nlink,
           (uintmax_t) st->st_uid,
           (uintmax_t) st->st_gid,
           (uintmax_t) st->st_size);

    print_time(st->st_mtime);

    printf("  %s\n", path);
}


// Traverse directory recursively
void traverse_directory(const char *directory_path,
                               Stats *stats,
                               unsigned depth)
{
    /*
     * opendir() открывает каталог.
     *
     * В Linux каталог представлен файловым дескриптором,
     * но opendir() возвращает более удобный объект DIR*,
     * предназначенный для работы с readdir().
     */
    DIR *dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Cannot open directory '%s': %s\n",
                directory_path, strerror(errno));

        ++stats->errors;
        return;
    }

    /*
     * readdir() возвращает следующий элемент каталога.
     *
     * После последнего элемента возвращается NULL.
     */
    errno = 0;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /*
         * Каталог всегда содержит специальные записи:
         *
         * "."  — текущий каталог
         * ".." — родительский каталог
         *
         * Их не надо обрабатывать, иначе рекурсия не закончится.
         */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /*
         * Формируем полный путь.
         */
        char *path = join_path(directory_path, entry->d_name);

        if (path == NULL) {
            fprintf(stderr,
                    "Cannot allocate memory for path '%s/%s'\n",
                    directory_path,
                    entry->d_name);

            ++stats->errors;
            continue;
        }

        /*
         * lstat(), в отличие от stat(), НЕ переходит по символической
         * ссылке.
         *
         * Это важно:
         *
         *     link -> /some/other/directory
         *
         * lstat() скажет, что это LINK.
         *
         * stat() сообщил бы информацию о целевом объекте.
         *
         * Использование lstat() позволяет не заходить внутрь
         * символических ссылок и не получить циклическую рекурсию.
         */
        struct stat st;

        if (lstat(path, &st) == -1) {
            fprintf(stderr,
                    "Cannot stat '%s': %s\n",
                    path,
                    strerror(errno));

            ++stats->errors;
            free(path);
            continue;
        }

        /*
         * Отступ показывает уровень вложенности.
         */
        for (unsigned i = 0; i < depth; ++i) {
            printf("    ");
        }

        /*
         * Выводим информацию о текущем объекте.
         */
        print_file_info(path, &st);

        /*
         * Обновляем статистику.
         */
        if (S_ISREG(st.st_mode)) {
            /*
             * Обычный файл.
             *
             * st_size — его размер в байтах.
             */
            ++stats->regular_files;
            stats->total_size += (uintmax_t)st.st_size;
        }
        else if (S_ISDIR(st.st_mode)) {
            /*
             * Каталог.
             */
            ++stats->directories;

            /*
             * Рекурсивно заходим внутрь каталога.
             */
            traverse_directory(path, stats, depth + 1);
        }
        else if (S_ISLNK(st.st_mode)) {
            /*
             * Символическая ссылка.
             */
            ++stats->symlinks;
        }
        else {
            /*
             * Например:
             *
             *   FIFO
             *   Unix socket
             *   device file
             */
            ++stats->other;
        }

        /*
         * path был создан через malloc().
         */
        free(path);
    }

    /*
     * Если цикл закончился не потому, что readdir() дошёл до конца,
     * а из-за ошибки, errno будет ненулевым.
     */
    if (errno != 0) {
        fprintf(stderr,
                "Error while reading directory '%s': %s\n",
                directory_path,
                strerror(errno));

        ++stats->errors;
    }

    /*
     * Закрываем каталог.
     */
    if (closedir(dir) == -1) {
        fprintf(stderr,
                "Cannot close directory '%s': %s\n",
                directory_path,
                strerror(errno));

        ++stats->errors;
    }
}


int main(int argc, char *argv[])
{
    /*
     * Ожидаем:
     *
     *     ./file_stat /path/to/directory
     *
     * argc == 2
     */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *directory_path = argv[1];

    /*
     * Проверяем, что указанный путь действительно существует
     * и является каталогом.
     */
    struct stat root_stat;

    if (lstat(directory_path, &root_stat) == -1) {
        fprintf(stderr,
                "Cannot access '%s': %s\n",
                directory_path,
                strerror(errno));

        return EXIT_FAILURE;
    }

    if (!S_ISDIR(root_stat.st_mode)) {
        fprintf(stderr,
                "'%s' is not a directory\n",
                directory_path);

        return EXIT_FAILURE;
    }

    /*
     * Инициализируем статистику нулями.
     */
    Stats stats = {0};

    printf("Scanning directory: %s\n\n", directory_path);

    /*
     * Запускаем рекурсивный обход.
     */
    traverse_directory(directory_path, &stats, 0);

    /*
     * Итоговая статистика.
     */
    printf("\n");
    printf("========== SUMMARY ==========\n");
    printf("Regular files : %" PRIuMAX "\n", stats.regular_files);
    printf("Directories   : %" PRIuMAX "\n", stats.directories);
    printf("Symlinks      : %" PRIuMAX "\n", stats.symlinks);
    printf("Other objects : %" PRIuMAX "\n", stats.other);
    printf("Total size    : %" PRIuMAX " bytes\n", stats.total_size);
    printf("Errors        : %" PRIuMAX "\n", stats.errors);

    return (stats.errors == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}