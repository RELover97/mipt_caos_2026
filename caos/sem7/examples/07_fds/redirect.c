#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    /* Стандартные дескрипторы: 0=stdin, 1=stdout, 2=stderr.
       dup(1) создаёт ещё один номер для того же открытого описания,
       что используется текущим stdout (терминал, pipe и т. п.). */
    int original_stdout = dup(STDOUT_FILENO);
    if (original_stdout == -1) {
        perror("dup");
        return EXIT_FAILURE;
    }

    /* Открываем новый файл, O_TRUNC обнуляет содержимое при каждом запуске.
       Права 0644 для нового файла могут дополнительно ограничиваться umask. */
    int file = open("build/redirected.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        perror("open");
        close(original_stdout);
        return EXIT_FAILURE;
    }

    /* До изменения fd 1 сбрасываем буфер stdio. Иначе ранее напечатанные,
       но ещё не записанные байты могут оказаться в другом назначении. */
    fflush(stdout);
    if (dup2(file, STDOUT_FILENO) == -1) {
        perror("dup2 redirect");
        close(file);
        close(original_stdout);
        return EXIT_FAILURE;
    }

    /* Теперь stdout (fd 1) указывает на то же открытое описание, что file.
       Закрытие исходного номера не закроет файл, пока открыт fd 1. */
    close(file);
    puts("written to file");
    fflush(stdout); /* Гарантируем запись этой строки именно в redirected.txt. */

    /* Восстанавливаем fd 1, заменив его сохранённым дескриптором stdout. */
    if (dup2(original_stdout, STDOUT_FILENO) == -1) {
        perror("dup2 restore");
        close(original_stdout);
        return EXIT_FAILURE;
    }
    close(original_stdout);
    puts("stdout restored"); /* Снова выводим в исходный терминал или pipe. */
    return EXIT_SUCCESS;
}
