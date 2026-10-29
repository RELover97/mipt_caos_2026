/*
Запустить фоном процесс и увидеть, что он zombie:

gcc zombie.c -o zombie
./zombie &

ps aux | grep Z (или defunct)

Как избавиться от зомби:
kill -9 ppid
*/

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    pid_t child_id = fork();

    if (child_id > 0) {
        // Родительский процесс спит 60 секунд, не вызывая wait()
        sleep(60);
    } else {
        // Дочерний процесс завершается мгновенно
        exit(0);
    }

    return 0;
}
