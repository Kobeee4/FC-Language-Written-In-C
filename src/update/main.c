#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    puts("FCL (FuseCore Language) - stub runtime v0.1");

    if (argc < 2) {
        printf("Usage: %s <source.fcl>\n", argv[0]);
        return 0;
    }

    const char *path = argv[1];
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open file");
        return 1;
    }

    printf("Simulating execution of %s\n---\n", path);
    int c;
    while ((c = fgetc(f)) != EOF) putchar(c);
    printf("\n---\nExecution finished (simulation).\n");
    fclose(f);
    return 0;
}
