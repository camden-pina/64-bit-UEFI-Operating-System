#include <stdio.h>
 #include <stdlib.h>
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        perror(argv[1]);
        exit(1);
    }
    unsigned char buffer[16];
    int i, j;
    while (!feof(fp)) {
        i = fread(buffer, 1, 16, fp);
        printf("%08x: ", (int)(ftell(fp) - i));
        for (j = 0; j < i; j++) {
            printf("%02x ", buffer[j]); if (j == 7)
            printf(" ");
        }
      
        printf("|");
        
        for (j = 0; j < i; j++) {
            if (buffer[j] >= ' ' && buffer[j] <= '~')
                printf("%c", buffer[j]);
            else
                printf(".");
        }
        printf("|\n");
    }
    fclose(fp);
    return 0;
}