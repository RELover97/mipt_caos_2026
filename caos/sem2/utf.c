#include <assert.h>  // assert
#include <limits.h>  // CHAR_BIT
#include <stdint.h>  // int32_t
#include <stdio.h>   // getchar_unlocked, printf

// check value of n-th bit (start from 0-th bit)
#define TEST_BIT(x, n) ((x) & ((1 << (n))))

// least significant bits: task n least bits of x
#define LSB(x, n) ((x) & ((1 << (n)) - 1))

// 4 types of UTF-8 characters:
// 1) 0b0xxxxxxx
// 2) 0b110xxxxx 0b10xxxxxx
// 3) 0b1110xxxx 0b10xxxxxx 0b10xxxxxx
// 4) 0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx

// https://en.wikipedia.org/wiki/Cyrillic_(Unicode_block)
enum { MIN_CYRILLIC_CODEPOINT = 0x400, MAX_CYRILLIC_CODEPOINT = 0x4ff };

int main() 
{
    int total_chars = 0;     // total number of Unicode characters
    int cyrillic_chars = 0;  // number of cyrillic characters

    int ch;
    int chars_left_for_codepoint = 0;
    uint32_t codepoint = 0;

    while ((ch = getchar_unlocked()) != EOF) {
        if (chars_left_for_codepoint == 0) {
            // new wide char
            if (!TEST_BIT(ch, CHAR_BIT - 1)) {  // (ch & (1 << 7)) == 0
                // ASCII char: 0b0xxxxxxx
                ++total_chars;
                continue;
            }

            // Here we have 3 cases: 2, 3, 4

            if (!TEST_BIT(ch, CHAR_BIT - 3)) {  // (ch & (1 << 5)) == 0
                // case 2
                chars_left_for_codepoint = 1;
            } else if (!TEST_BIT(ch, CHAR_BIT - 4)) {  // (ch & (1 << 4)) == 0
                // case 3
                chars_left_for_codepoint = 2;
            } else if (!TEST_BIT(ch, CHAR_BIT - 5)) {  // (ch & (1 << 3)) == 0
                // case 4
                chars_left_for_codepoint = 3;
            } else {
                // unexpected char
                assert(0);
            }

            codepoint = LSB(ch, CHAR_BIT - (chars_left_for_codepoint + 1));
        } else {
            // 0b10xxxxxx
            codepoint = (codepoint << (CHAR_BIT - 2)) + LSB(ch, CHAR_BIT - 2);
            if (--chars_left_for_codepoint == 0) {
                ++total_chars;
                if ((codepoint >= MIN_CYRILLIC_CODEPOINT) &&
                    (codepoint <= MAX_CYRILLIC_CODEPOINT)) {
                    ++cyrillic_chars;
                }
                codepoint = 0;
            }
        }
    }

    printf("%d %d\n", total_chars, cyrillic_chars);

    return 0;
}