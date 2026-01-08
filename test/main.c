#include <typedefs.h>
#include <io.h>
#include <sys.h>
#include <errno.h>

int main()
{
    struct fs_node node = {0};
    int res = -ENOENT;
    
    while ((res = readdir("/", &node)) > 0)
    {
        if (node.st_mode & STAT_DIR)
        {
            printf("   directory   %s\n", node.name);
        }
        else if (node.st_mode & STAT_FILE)
        {
            printf("   file        %s\n", node.name);
        }
        else if (node.st_mode & STAT_BLOCK)
        {
            printf("   bdev        %s\n", node.name);
        }
        else if (node.st_mode & STAT_CHAR)
        {
            printf("   chardev     %s\n", node.name);
        }
    }

    if (res == -ENOENT)
        printf("no such dir.\n");

    /* Test serial write via devfs */
    handle_t hndl;
    if (open(&hndl, "/dev/input/char_serial0", WRITE) != 0)
        printf("unable to open serial.\n");
    else {
        write(hndl, "hello from serial", 17);
        close(hndl);
    }
    
    /* Test read keyboard */
    struct
    {
        uint32_t event_id;
        dev_t dev;

        int released;
        uint8_t code;
        uint16_t key;
    } kbd_event;
    char key[2];
    key[1] = '\0';

    if (open(&hndl, "/dev/input/char_kbd0", READ) != 0)
        printf("unable to open keyboard.\n");
    else {
        printf("waiting for keyboard events\n");
        int last_event;
        while (read(hndl, (void*)&kbd_event, sizeof(kbd_event), NULL) >= 0)
        {
            if (last_event != kbd_event.event_id)
            {
                key[0] = kbd_event.key;
                if (kbd_event.released)
                    printf(&key[0]);
                last_event = kbd_event.event_id;
            }

            if (key[0] == 'q')
                break;
        }
        
        printf("closing keyboard\n");
        close(hndl);
    }

    printf("done\n");
    return 0;
}
