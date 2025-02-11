#include <cstring>
#include <cstdio>

int main(int argc, char* argv[]) {
    while (argc > 1) {
        // Find smallest argument
        int smallest = 1;

        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[smallest], argv[i]) > 0) {
                smallest = i;
            }
        } 

        fprintf(stdout, "%s\n", argv[smallest]);

        // Replace smallest with topmost element    
        argv[smallest] = argv[argc - 1];
        --argc;
    }
}