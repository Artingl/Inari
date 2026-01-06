#include <typedefs.h>
#include <sys.h>
#include <errno.h>


int main()
{
    debug("init launched");

    /* Mount devfs */
    mount(0, "/dev");

    struct fs_node nodes[8];
    int total_files = readdir("/", &nodes[0], 0, 8);

    if (total_files < 0)
    {
        debug("unable to read dir.");
        exit(1);
    }

    for (int i = 0; i < total_files; i++)
    {
        if (nodes[i].st_mode & IO_STAT_DIR)
        {
            debug("directory");
            debug(nodes[i].name);
            debug("");
        }
        if (nodes[i].st_mode & IO_STAT_FILE)
        {
            debug("file");
            debug(nodes[i].name);
            debug("");
        }
    }

    return 0;
}
