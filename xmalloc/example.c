#include "xmalloc.h" 
#include <stdio.h>

int main(int argc, char *argv[])
{

    int *a = xmalloc(10);
    *a = 10;
    printf("%d\n", *(int *)a);
    xfree(a);
    printf("%d\n", *(int *)a);
    return 0;
}
