#include <errno.h>
#include <io.h>
#include <string.h>
#include <sys.h>

int main(int argc, char *argv[]) {
    time_t time = 0;
    int res;
    if ((res = uptime(&time)) != 0) {
        printf("%s: Unable to get uptime: %s\n", get_name(), errno(res));
        return -1;
    }

    if (argc > 1 && strcmp(argv[1], "-u") == 0)
        printf("%llu day(s), %llu hour(s), %llu minute(s), %llu second(s). (us: %llu)\n", time / 86400000000,
               (time / 3600000000) % 24, (time / 60000000) % 60, (time / 1000000) % 60, time);
    else
        printf("%llu day(s), %llu hour(s), %llu minute(s), %llu second(s).\n", time / 86400000000,
               (time / 3600000000) % 24, (time / 60000000) % 60, (time / 1000000) % 60);
    return 0;
}
