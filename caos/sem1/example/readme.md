Commits:

1. gcc

`g++ -std=c++20 main.cpp math.cpp -o app`

Посмотреть, какие команды gcc запускает под капотом:

`g++ -std=c++20 -v main.cpp math.cpp -o app`

`g++ -std=c++20 -### main.cpp math.cpp -o app`

2. Preprocessor

Добавим директивы в `main.cpp`

`g++ -E main.cpp -o main.i`

Посчитать число строк в файле: 

`wc -l main.cpp`

`wc -l main.i`