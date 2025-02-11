#include <cstdlib>
#include <cstdio>

int main() {
    unsigned long numBytes = 0;

    while (fgetc(stdin) != EOF) {
        numBytes++;
    }

    fprintf(stdout, "%8lu\n", numBytes);

    exit(0);
}