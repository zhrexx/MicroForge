#include "xmalloc.h" 
#include <stdio.h>

int main(int argc, char *argv[])
{

    void *a = xmalloc(10);
    int b = 10;
    a = &b;
    printf("%d", *(int *)a);
    return 0;
}
