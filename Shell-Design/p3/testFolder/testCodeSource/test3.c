/**
 * This will print 0-20 with space in each number
*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
int main()
{
    for(int i=0;i<20;i++)
    {
        printf("%d ",i);
    }
    fflush(NULL);
    return 0;
}