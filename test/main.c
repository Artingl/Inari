#include <typedefs.h>
#include <sys.h>
#include <errno.h>

int main()
{
    debug("init launched");

    /* Mount devfs */
    mount(0, "/dev");

    struct fs_node node = {0};
    int res = -ENOENT;
    
    while ((res = readdir("/dev/input", &node)) > 0)
    {
        if (node.st_mode & STAT_DIR)
        {
            debug("directory");
            debug(node.name);
            debug("");
        }
        else if (node.st_mode & STAT_FILE)
        {
            debug("file");
            debug(node.name);
            debug("");
        }
        else if (node.st_mode & STAT_BLOCK)
        {
            debug("bdev");
            debug(node.name);
            debug("");
        }
    }

    if (res == -ENOENT)
        debug("no such dir.");

    /* Test serial write via devfs */
    handle_t hndl;
    if (open(&hndl, "/dev/input/char_serial0", WRITE) != 0)
        debug("unable to open serial.");
    else {
        write(hndl, "hello from serial", 17);
        close(hndl);
    }

    debug("done");
    return 0;
}
