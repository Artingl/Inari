#include <io.h>
#include <sys.h>

int main()
{
    pid_t pid;
    get_pid(&pid);
    printf("Hello from test program! My pid is %d\n", pid);
    // execp(NULL, "/init.exe");

    while(1){}
}
