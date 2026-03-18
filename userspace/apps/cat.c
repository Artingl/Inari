#include <io.h>
#include <sys.h>
#include <errno.h>
#include <lib.h>

int main(int argc, char const *argv[])
{
    handle_t hndl;
    int res = 0;
    size_t sz;

    if (argc < 2)
    {
        printf("usage: %s file\n", argv[0]);
        res = -1;
        goto end;
    }

    if ((res = open(&hndl, argv[1], READ)) != 0)
    {
        printf("%s: %s: %s.\n", argv[0], argv[1], errstr[-res] ? errstr[-res] : "Invalid error");
        goto end;
    }
    if ((res = size(hndl, &sz)) != 0)
    {
        printf("%s: %s: %s.\n", argv[0], argv[1], errstr[-res] ? errstr[-res] : "Invalid error");
        goto end;
    }
    
    char *data = malloc(sz + 2);
    if (!data)
    {
        printf("%s: %s: %s.\n", argv[0], argv[1], errstr[ENOMEM]);
        goto close;
    }

    data[sz] = '\n';
    data[sz + 1] = '\0';

    if ((res = read(hndl, data, sz, NULL)) != 0)
    {
        printf("%s: %s: %s.\n", argv[0], argv[1], errstr[-res] ? errstr[-res] : "Invalid error");
        free(data);
        close(hndl);
        goto end;
    }

    write(stdout, data, sz);
    flush(stdout);
    free(data);
close:
    close(hndl);
end:
    return res;
}