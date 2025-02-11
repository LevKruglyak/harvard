#include <cstdlib>
#include <cstdio>
#include <cctype>

int main() {
    unsigned long numberCharacters = 0, numberWords = 0, numberLines = 0;

    char currentChar;
    bool wasLastCharacrterSpace = true;

    while (true) {
        currentChar = fgetc(stdin);

        if (currentChar == EOF) {
            break;
        }

        // Increment character counter
        numberCharacters++;

        // Check if we need to increment line counter
        if (currentChar == '\n') {
            numberLines++;
        }

        // Check if we just entered a word
        if (isspace((unsigned char) currentChar)) {
            wasLastCharacrterSpace = true;
        } else if (wasLastCharacrterSpace) {
            wasLastCharacrterSpace = false;
            numberWords++;
        }
    }

    fprintf(stdout, "%8lu %7lu %7lu\n", numberLines, numberWords, numberCharacters);

    exit(0);
}