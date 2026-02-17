#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <string.h>
#include <sys.h>
#include <errno.h>

char path[128] = {0};
int path_offset = 0;
uint32_t last_kb_event;
handle_t kb_hndl;
struct
{
    uint32_t event_id;
    dev_t dev;

    int released;
    uint8_t code;
    uint16_t key;
} kbd_event;

void exec_cmd()
{
    pid_t pid;
    if (execp(&pid, path) != 0)
        printf("error: command not found\n");
}

int main()
{
    printf("Simple test shell.\n\n");

    if (open(&kb_hndl, "/dev/input/char_kbd0", READ) != 0)
    {
        printf("error: unable to open keyboard.\n");
        exit(1);
    }

    printf("Contents of root:\n");
    struct fs_node node = {0};
    char *name;
    int res = -ENOENT;
    while ((res = readdir("/", &node)) > 0)
    {
        if (node.st_mode & STAT_DIR)
            name = "dir      ";
        else if (node.st_mode & STAT_FILE)
            name = "file     ";
        else if (node.st_mode & STAT_BLOCK)
            name = "bdev     ";
        else if (node.st_mode & STAT_CHAR)
            name = "chardev  ";

        printf("   %ssz %llukb   stat 0x%x     %s\n", name, node.size >> 10, (uint32_t)node.st_mode, node.name);
    }

    if (res == -ENOENT)
        printf("no such dir.\n");

    printf(">> ");

    do
    {
        if (read(kb_hndl, (void*)&kbd_event, sizeof(kbd_event), NULL) >= 0)
        {
            if (last_kb_event != kbd_event.event_id && kbd_event.released)
            {
                last_kb_event = kbd_event.event_id;
                if (kbd_event.key == 0x101)
                {
                    printf("\n");
                    exec_cmd();
                    path_offset = 0;
                    memset((void*)&path[0], 0, sizeof(path));
                    printf(">> ");
                }
                else {
                    path[path_offset++] = kbd_event.key;
                    path[path_offset] = '\0';

                    printf("%c", path[path_offset-1]);
                }
            }
        }

    } while(1);

    return 0;
}


// int main()
// {
//     struct fs_node node = {0};
//     int res = -ENOENT;
    
//     while ((res = readdir("/", &node)) > 0)
//     {
//         if (node.st_mode & STAT_DIR)
//         {
//             printf("   directory   %s\n", node.name);
//         }
//         else if (node.st_mode & STAT_FILE)
//         {
//             printf("   file        %s\n", node.name);
//         }
//         else if (node.st_mode & STAT_BLOCK)
//         {
//             printf("   bdev        %s\n", node.name);
//         }
//         else if (node.st_mode & STAT_CHAR)
//         {
//             printf("   chardev     %s\n", node.name);
//         }
//     }

//     if (res == -ENOENT)
//         printf("no such dir.\n");

//     /* Test serial write via devfs */
//     handle_t hndl;
//     if (open(&hndl, "/dev/input/char_serial0", WRITE) != 0)
//         printf("unable to open serial.\n");
//     else {
//         write(hndl, "hello from serial", 17);
//         close(hndl);
//     }
    
//     /* Test read keyboard */
//     struct
//     {
//         uint32_t event_id;
//         dev_t dev;

//         int released;
//         uint8_t code;
//         uint16_t key;
//     } kbd_event;
//     char key[2];
//     key[1] = '\0';

//     if (open(&hndl, "/dev/input/char_kbd0", READ) != 0)
//         printf("unable to open keyboard.\n");
//     else {
//         printf("waiting for keyboard events\n");
//         int last_event;
//         while (read(hndl, (void*)&kbd_event, sizeof(kbd_event), NULL) >= 0)
//         {
//             if (last_event != kbd_event.event_id)
//             {
//                 key[0] = kbd_event.key;
//                 if (kbd_event.released)
//                     printf(&key[0]);
//                 last_event = kbd_event.event_id;
//             }

//             if (key[0] == 'q')
//                 break;
//         }
        
//         printf("closing keyboard\n");
//         close(hndl);
//     }

//     printf("done\n");
//     return 0;
// }
