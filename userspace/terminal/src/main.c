#include <io.h>
#include <lib.h>
#include <signals.h>
#include <sys.h>

#include <ism.h>

/* TODO: move everything related to UI to toolkit library */
#define SSFN_IMPLEMENTATION
#include "ssfn.h"

static ssfn_t font_ctx = {0};
static uint8_t *font_buf = NULL;
static uint32_t font_size;

void render_text(struct ism_draw_window_payload *draw, const char *text, int x, int y) {
    ssfn_select(&font_ctx, SSFN_FAMILY_SERIF, NULL, SSFN_STYLE_REGULAR, 16);

    ssfn_buf_t buf = {.ptr = draw->buffer,
                      .w = draw->width,
                      .h = draw->height,
                      .p = draw->width * 4,
                      .x = x,
                      .y = y,
                      .fg = 0xFF000000};

    char *ptr = (char *)text;
    while (*ptr) {
        ptr += ssfn_render(&font_ctx, &buf, ptr);
    }
}

handle_t ism_ipc;

int main(void) {
    if (ipc_open("ism.window", &ism_ipc) != 0) {
        printf("%s: Unable to open IPC with ISM.\n", get_name());
        return -1;
    }

    /* Create a window */
    wind_t wind;
    struct ism_create_window __attribute__((aligned(0x1000)))
    window = {.name = "Terminal",
              .x = 40,
              .y = 40,
              .width = 640,
              .height = 600,
              .flags = ISM_WINDOW_FLAG_BORDER | ISM_WINDOW_FLAG_FRAME,
              .parent = ISM_WINDOW_ROOT};
    ipc_send(ism_ipc, ISM_IPC_CREATE_WINDOW, &window, sizeof(window));
    if ((wind = ipc_wait(ism_ipc, 1)) < 0) {
        printf("%s: Unable to create window.\n", get_name());
        return -1;
    }

    /* Load font */
    handle_t font_handle;
    if (open(&font_handle, "/system/fonts/freesans.sfn", READ) != 0) {
        printf("%s: Unable to load font: /system/fonts/freesans.sfn\n", get_name());
        return -1;
    }

    if (size(font_handle, &font_size) != 0) {
        printf("%s: Unable to load font: /system/fonts/freesans.sfn\n", get_name());
        return -1;
    }

    if ((font_buf = malloc(font_size)) == NULL) {
        printf("%s: Unable to allocate memory.\n", get_name());
        return -1;
    }

    if (read(font_handle, font_buf, font_size, NULL) != 0) {
        printf("%s: Unable to load font: /system/fonts/freesans.sfn\n", get_name());
        return -1;
    }
    close(font_handle);

    memset(&font_ctx, 0, sizeof(ssfn_t));
    ssfn_load(&font_ctx, (ssfn_font_t *)font_buf);

    size_t draw_size = sizeof(struct ism_draw_window_payload) + window.width * window.height * 4;
    struct ism_draw_window_payload *draw_window =
        (struct ism_draw_window_payload *)align_up((uintptr_t)malloc(draw_size + 0x1000), 0x1000);
    draw_window->wind = wind;
    draw_window->bpp = 32;
    draw_window->width = window.width;
    draw_window->height = window.height;
    draw_window->x = 0;
    draw_window->y = 0;

    for (int x = 0; x < window.width; x++)
        for (int y = 0; y < window.height; y++) {
            ((uint32_t *)draw_window->buffer)[y * window.width + x] = 0xffBBBBBB;
        }

    int idx = 0, y_off = 0;
    pid_t pid;
    tid_t tid;
    uint8_t state;
    time_t usg, totalusg;
    char cmd[128], strbuff[128];
    time_t time = 0;
    do {
        for (int x = 0; x < window.width; x++)
            for (int y = 0; y < window.height; y++) {
                ((uint32_t *)draw_window->buffer)[y * window.width + x] = 0xffBBBBBB;
            }

        uptime(&time);
        sprintf(strbuff, "uptime: %llu day(s), %llu hour(s), %llu minute(s), %llu second(s). (us: %llu)",
                time / 86400000000, (time / 3600000000) % 24, (time / 60000000) % 60, (time / 1000000) % 60, time);
        render_text(draw_window, strbuff, 50, 50);

        /* Display system usage as a test */
        render_text(draw_window, "  PID   USG   PATH", 50, 70);
        totalusg = 1;
        idx = 0;
        while (lsproc(idx++, &cmd[0], &pid, &usg) > 0)
            totalusg += usg;
        idx = 0;
        while (lsproc(idx++, &cmd[0], &pid, &usg) > 0) {
            sprintf(strbuff, "  %llu   %2f%%   %s", pid, ((double)usg / (double)totalusg) * 100.0f, cmd);
            render_text(draw_window, strbuff, 50, 70 + idx * 22);
        }

        y_off = 100 + idx * 22;
        render_text(draw_window, "  TID   USG   STATE   NAME", 50, y_off);
        totalusg = 1;
        idx = 0;
        while (lsthrd(idx++, &cmd[0], &tid, &usg, &state) > 0)
            totalusg += usg;
        idx = 0;
        while (lsthrd(idx++, &cmd[0], &tid, &usg, &state) > 0) {
            sprintf(strbuff, "  %llu   %2f%%   0x%x   %s", tid, ((double)usg / (double)totalusg) * 100.0f, state, &cmd[0]);
            render_text(draw_window, strbuff, 50, y_off + idx * 22);
        }

        if (ipc_send(ism_ipc, ISM_IPC_DRAW_WINDOW, draw_window, draw_size) != 0 || ipc_wait(ism_ipc, 1) != 0) {
            printf("%s: Unable to render the window.\n", get_name());
        }

    } while (1);
    // printf("terminal: HELLO!\n");
    // usleep(5000000);
    // printf("terminal: BYE!\n");
    return 0;
}
