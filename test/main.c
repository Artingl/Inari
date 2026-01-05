#include <typedefs.h>
#include <sys.h>
#include <errno.h>

int main()
{
    debug("Hello world from init!");

    handle_t file;
    char buf[8];
    int res = open(&file, "/test.txt", IO_READ);

    if (res != 0)
        debug("unable to open file!");
    
    read(file, &buf[0], 7, NULL);
    buf[7] = '\0';
    debug(buf);

    if (close(file) != 0)
        debug("unable to close file!");
    usleep(1000000);
    debug("dying!");
    return 0;
}
