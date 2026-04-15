# include "string.h"
# include "type.h"

int strcmp(const char *s1, const char *s2){
    while(*s1 && (*s1 == *s2)){
        s1++; s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, int n){
    while(n > 0 && *s1 && (*s1 == *s2)){
        s1++;
        s2++;
        n--;
    }
    if(n == 0) 
        return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strlen(const char *s){
    int len = 0;
    while (*s++) 
        len++;
    return len;
}
int hextoi(const char* s, int n) {
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= 0)
            r += *s++ - '0';
    }
    return r;
}
int align(int n, int byte) {
    return (n + byte - 1) & ~(byte - 1);
}