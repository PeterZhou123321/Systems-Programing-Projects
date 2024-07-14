#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
int main()
{
    char s[4];
    read(STDIN_FILENO,s,3);
    s[3]=0;
    if(strcmp("abc",s)==0)
    {
        printf("Received\n");
    }
    return 0;
}