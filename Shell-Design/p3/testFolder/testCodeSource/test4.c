/**
 * This will take input stream, output them
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
int main()
{
    char c;
    while(read(STDIN_FILENO,&c,1))
    {
        printf("%c",c);
        if(c==' ')
        {
            printf("\n");
        }
    }
    return 0;
}