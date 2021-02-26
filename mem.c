#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
using namespace std;
int nums[5] = {0, 1, 2, 3, 4};
int main(void)
{
    const int SIZE = 80;
    char *history[SIZE];
    *history = (char *)malloc(sizeof(char *) * SIZE);
    int i = 0;
    while (i < SIZE)
    {
        char *copy = ;
        history[i] = copy;
        cout << history[i] << endl;
        i++;
    }

    return 0;
}