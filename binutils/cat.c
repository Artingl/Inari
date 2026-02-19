#include <io.h>
#include <sys.h>
#include <errno.h>
#include <alloc.h>

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s [file]\n", argv[0]);
        return -1;
    }

    handle_t hndl;
    int res = 0;
    size_t sz;

    if ((res = open(&hndl, argv[1], READ)) != 0)
    {
        printf("%s: %s: No such file or directory.\n", argv[0], argv[1]);
        goto end;
    }
    if ((res = size(hndl, &sz)) != 0)
    {
        printf("%s: %s: Unable to read the file.\n", argv[0], argv[1]);
        goto end;
    }
    
    char *data = malloc(sz + 2);
    data[sz] = '\n';
    data[sz + 1] = '\0';

    if (read(hndl, data, sz, NULL) == 0)
        write(stdout, data, sz);

    free(data);
close:
    close(hndl);
end:
    return res;
}