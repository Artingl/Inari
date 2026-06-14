#include <io.h>
#include <sys.h>

int main(int argc, char const *argv[]) {
    char *path;
    /* If no path is provided, use the path from options */
    if (argc < 2) {
        union p_option_value result;
        if (p_option_get("path", &result) != 0) {
            printf("usage: %s directory\n", argv[0]);
            return -1;
        }

        path = result.value;
    } else
        path = (char *)argv[1];

    printf("Contents of %s:\n", path);
    struct fs_node node = {0};
    char *name;
    int res = 0, found_files = 0;
    while ((res = readdir(path, &node)) > 0) {
        if (node.st_mode & STAT_DIR)
            name = "dir      ";
        else if (node.st_mode & STAT_FILE)
            name = "file     ";
        else if (node.st_mode & STAT_BLOCK)
            name = "bdev     ";
        else if (node.st_mode & STAT_CHAR)
            name = "chardev  ";
        else
            name = "invalid  ";

        printf("   %ssz %llu%s\tstat 0x%x\t%s\n", name, node.size >= 1024 ? node.size >> 10 : node.size,
               node.size >= 1024 ? "kb" : "b", (uint32_t)node.st_mode, node.name);
        found_files = 1;
    }

    if (!found_files && res < 0)
        printf("No such directory.\n");
    else if (!found_files && res == 0)
        printf("Directory is empty.\n");

    if (found_files)
        return 0;

    return res;
}
