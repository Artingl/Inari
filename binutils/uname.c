#include <sys.h>
#include <io.h>
#include <string.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct utsname kuname;
    int res;
    int is_full = argc > 1 ? strcmp(argv[1], "-a") == 0 ? 1 : 0 : 0;

    if ((res = uname(&kuname)) != 0)
    {
        printf("%s: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
    }

    if (is_full)
        printf("%s %s %s %s %s\n",
            kuname.sysname, kuname.nodename,
            kuname.release, kuname.version, kuname.machine);
    else
        printf("%s\n", kuname.sysname);
    return 0;
}
