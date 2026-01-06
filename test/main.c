#include <typedefs.h>
#include <sys.h>
#include <errno.h>


int main()
{
    debug("init launched");

    /* Mount devfs */
    mount(0, "/dev");

    struct fs_node node = {0};
    
    while (readdir("/", &node) > 0)
    {
        if (node.st_mode & IO_STAT_DIR)
        {
            debug("directory");
            debug(node.name);
            debug("");
        }
        if (node.st_mode & IO_STAT_FILE)
        {
            debug("file");
            debug(node.name);
            debug("");
        }
    }

    debug("done");
    usleep(1000000);
    return 0;
}
