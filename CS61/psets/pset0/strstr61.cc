#include <cstring>
#include <cassert>
#include <cstdio>

// char* mystrstr(const char* s1, const char* s2) {
//     // special case in the event s2 is emtpy
//     if (!s2[0]) {
//         return (char*) s1;
//     }

//     // loop over s1
//     for (size_t i = 0; s1[i] != 0; ++i) {
//         // loop over s2 until they dont match
//         size_t j = 0;

//         while (s2[j] != 0 && s1[i + j] == s2[j]) {
//             ++j;
//         }

//         // check if we've reached the end of s2
//         if (!s2[j]) {
//             return (char*) &s1[i];
//         }
//     }

//     return nullptr;
// }

char* mystrstr(const char* s1, const char* s2) {
    if (!*s2) {
        return (char*) s1;
    }

    while (*s1) {
        const char* s1check = s1;
        const char* s2check = s2;

        while (*s2check && *s1check == *s2check) {
            ++s1check;
            ++s2check;
        }

        if (!*s2check) {
            return (char*) s1;
        }

        ++s1;
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    assert(argc == 3);
    printf("strstr(\"%s\", \"%s\") = %p\n", argv[1], argv[2], strstr(argv[1], argv[2]));
    printf("mystrstr(\"%s\", \"%s\") = %p\n", argv[1], argv[2], mystrstr(argv[1], argv[2]));
    assert(strstr(argv[1], argv[2]) == mystrstr(argv[1], argv[2]));
}