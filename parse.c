#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

int fsize = 0;

int main(void)
{
    FILE *fp;
    fp = fopen("in.txt", "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (fp < 0)
    {
        cerr << "Error opening file" << endl;
    }

    while (read = getline(&line, &len, fp) != -1)
    {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        cout << "length " << read << endl;
        printf("%s\n", line);
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(fp);
    if (line)
    {
        free(line);
    }
    return 0;
}