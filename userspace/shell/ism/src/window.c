#include <errno.h>
#include <io.h>
#include <lib.h>
#include <list.h>
#include <signals.h>
#include <string.h>
#include <sys.h>

#include "icons.h"
#include "input.h"
#include "video.h"
#include "window.h"
#include <ism.h>

#define SSFN_IMPLEMENTATION
#include "ssfn.h"

#define WINDOW_FRAME_HEIGHT 24

#define draw_bold_frame(buffer, x, y, width, height, pressed)                                                          \
    do {                                                                                                               \
        uint8_t tl_out = (pressed) ? 0x00 : 0xFF;                                                                      \
        uint8_t tl_in = (pressed) ? 0x55 : 0xAA;                                                                       \
        uint8_t br_in = (pressed) ? 0xAA : 0x55;                                                                       \
        uint8_t br_out = (pressed) ? 0xFF : 0x00;                                                                      \
        fill_rect(buffer, tl_out, x, y, width, 1, 1);                                                                  \
        fill_rect(buffer, tl_out, x, y, 1, height, 1);                                                                 \
        fill_rect(buffer, tl_in, x + 1, y + 1, width - 2, 1, 1);                                                       \
        fill_rect(buffer, tl_in, x + 1, y + 1, 1, height - 2, 1);                                                      \
        fill_rect(buffer, br_in, x + 1, y + height - 2, width - 2, 1, 1);                                              \
        fill_rect(buffer, br_in, x + width - 2, y + 1, 1, height - 2, 1);                                              \
        fill_rect(buffer, br_out, x, y + height - 1, width, 1, 1);                                                     \
        fill_rect(buffer, br_out, x + width - 1, y, 1, height, 1);                                                     \
        fill_rect(buffer, 0xAA, x + 2, y + 2, width - 4, height - 4, 1);                                               \
    } while (0)

#define collides_element(x, y, element)                                                                                \
    ((int32_t)x >= (int32_t)(element)->x && (int32_t)y >= (int32_t)(element)->y &&                                     \
     (int32_t)x < (int32_t)(element)->x + (int32_t)(element)->width &&                                                 \
     (int32_t)y < (int32_t)(element)->y + (int32_t)(element)->height)

static wind_t last_frid = 1;
static struct ism_window root_window;

#define REGION_SIZE_SHIFT 4
#define REGION_SIZE       (2 << (REGION_SIZE_SHIFT - 1))

static uint8_t *video_buffer = NULL;
static uint8_t *dirty_regions = NULL;
static struct ism_video_map video_map;
static int32_t video_width, video_height;
static uint8_t cursor_style = ISM_CURSOR_NORMAL;

/* TODO: move to something like font.c */
static ssfn_t font_ctx = {0};
static uint8_t *font_buf = NULL;
static uint32_t font_size;

#pragma GCC push_options
#pragma GCC optimize("O3")

static inline int fill_rect(struct ism_buffer buffer, uint32_t color, int32_t x_orig, int32_t y_orig, int32_t width,
                            int32_t height, uint32_t px_len) {
    int32_t x, y;
    uint8_t r, g, b;
    uintptr_t off;

    for (y = y_orig; y < height + y_orig; y++)
        for (x = x_orig; x < width + x_orig; x++) {
            if (y >= buffer.height || x >= buffer.width || x < 0 || y < 0)
                continue;
            off = y * buffer.width + x;
            if ((off << 2) >= buffer.size)
                continue;
            r = px_len > 2 ? color >> 16 : (uint8_t)color;
            g = px_len > 2 ? (color >> 8) & 0xff : (uint8_t)color;
            b = px_len > 2 ? color & 0xff : (uint8_t)color;

            ((uint32_t *)buffer.base)[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
        }

    return 0;
}

static inline int blit_icon(struct ism_buffer buffer, uint8_t *buf, int32_t x_orig, int32_t y_orig, int32_t width,
                            int32_t height) {
    int32_t x, y;
    uint8_t c;
    uintptr_t off, buf_off = 0, buf_size = width * height;

    for (y = y_orig; y < height + y_orig; y++)
        for (x = x_orig; x < width + x_orig; x++) {
            if (y >= buffer.height || x >= buffer.width || x < 0 || y < 0)
                continue;
            off = y * buffer.width + x;
            buf_off = (y - y_orig) * width + (x - x_orig);
            if ((off << 2) >= video_map.size || buf_off >= buf_size)
                continue;
            c = buf[buf_off];
            if (c == T_CLR)
                continue;

            ((uint32_t *)buffer.base)[off] = (((((0xff << 8) | c) << 8) | c) << 8) | c;
        }

    return 0;
}

static inline int blit_buffer(struct ism_buffer buffer, struct ism_buffer src, int32_t x_orig, int32_t y_orig) {
    int32_t x, y;
    uintptr_t off, buf_off;

    for (y = y_orig; y < src.height + y_orig; y++)
        for (x = x_orig; x < src.width + x_orig; x++) {
            if (y >= buffer.height || x >= buffer.width || x < 0 || y < 0)
                continue;
            off = y * buffer.width + x;
            buf_off = (y - y_orig) * src.width + (x - x_orig);
            if ((off << 2) >= video_map.size || (buf_off << 2) >= src.size)
                continue;

            ((uint32_t *)buffer.base)[off] = ((uint32_t *)src.base)[buf_off];
        }

    return 0;
}

static inline void mark_region_range(int32_t x, int32_t y, int32_t width, int32_t height, uint8_t is_dirty) {
    x >>= REGION_SIZE_SHIFT;
    y >>= REGION_SIZE_SHIFT;
    int32_t ox = x, oy = y;
    int32_t end_x = ox + (width >> REGION_SIZE_SHIFT) + 1, end_y = oy + (height >> REGION_SIZE_SHIFT) + 1;
    int32_t w = (video_width >> REGION_SIZE_SHIFT), h = (video_height >> REGION_SIZE_SHIFT);
    for (x = ox; x <= end_x; x++)
        for (y = oy; y <= end_y; y++) {
            if (x >= w || y >= h || x < 0 || y < 0)
                continue;
            dirty_regions[y * w + x] = is_dirty;
        }
}

static inline void mark_region(int32_t x, int32_t y, uint8_t is_dirty) {
    mark_region_range(x, y, REGION_SIZE, REGION_SIZE, is_dirty);
}

static inline uint8_t is_region_range_dirty(int32_t x, int32_t y, int32_t width, int32_t height) {
    x >>= REGION_SIZE_SHIFT;
    y >>= REGION_SIZE_SHIFT;
    int32_t ox = x, oy = y;
    int32_t end_x = ox + (width >> REGION_SIZE_SHIFT) + 1, end_y = oy + (height >> REGION_SIZE_SHIFT) + 1;
    int32_t w = (video_width >> REGION_SIZE_SHIFT), h = (video_height >> REGION_SIZE_SHIFT);
    uint8_t is_dirty = 0;
    for (x = ox; x <= end_x; x++)
        for (y = oy; y <= end_y; y++) {
            if (x >= w || y >= h || x < 0 || y < 0)
                continue;
            if (!is_dirty)
                is_dirty = dirty_regions[y * w + x];
        }
    return is_dirty;
}

static inline uint8_t is_region_dirty(int32_t x, int32_t y) {
    return is_region_range_dirty(x, y, REGION_SIZE, REGION_SIZE);
}

static struct ism_window *foreach_children(wind_t id, struct ism_window *window) {
    if (!window)
        return NULL;
    if (window->window_id == id)
        return window;
    struct list_head *pos;
    struct ism_window *entry, *found;
    list_for_each(pos, &window->children) {
        entry = list_entry(pos, struct ism_window, list);
        if (entry->window_id == id)
            return entry;
        if ((found = foreach_children(id, entry)) != NULL)
            return found;
    }

    return NULL;
}

static struct ism_window *foreach_children_pid(pid_t pid, struct ism_window *window) {
    if (!window)
        return NULL;
    if (window->owner == pid)
        return window;
    struct list_head *pos;
    struct ism_window *entry, *found;
    list_for_each(pos, &window->children) {
        entry = list_entry(pos, struct ism_window, list);
        if (entry->owner == pid)
            return entry;
        if ((found = foreach_children_pid(pid, entry)) != NULL)
            return found;
    }

    return NULL;
}

/* Return window that collides with provided coordinates.
   TODO: Check children window too. */
static struct ism_window *get_window_position(int x, int y) {
    struct list_head *pos;
    struct ism_window *entry;
    list_for_each_prev(pos, &root_window.children) {
        entry = list_entry(pos, struct ism_window, list);
        if (collides_element(x, y, entry))
            return entry;
    }

    return NULL;
}

static struct ism_window *get_window(wind_t id) { return foreach_children(id, &root_window); }

static struct ism_window *get_window_pid(pid_t pid) { return foreach_children_pid(pid, &root_window); }

static int copy_window_buffer(struct ism_window *window) {
    int res = 0;

    int32_t w_cols = video_width >> REGION_SIZE_SHIFT;

    int32_t win_x_start = window->x;
    int32_t win_y_start = window->y;
    int32_t win_x_end = window->x + window->width;
    int32_t win_y_end = window->y + window->height;

    if (win_x_start < 0)
        win_x_start = 0;
    if (win_y_start < 0)
        win_y_start = 0;
    if (win_x_end > video_width)
        win_x_end = video_width;
    if (win_y_end > video_height)
        win_y_end = video_height;

    if (win_x_start >= win_x_end || win_y_start >= win_y_end)
        return 0;

    int32_t start_bx = win_x_start >> REGION_SIZE_SHIFT;
    int32_t start_by = win_y_start >> REGION_SIZE_SHIFT;
    int32_t end_bx = win_x_end >> REGION_SIZE_SHIFT;
    int32_t end_by = win_y_end >> REGION_SIZE_SHIFT;

    for (int32_t by = start_by; by <= end_by; by++) {
        for (int32_t bx = start_bx; bx <= end_bx; bx++) {
            if (!dirty_regions[by * w_cols + bx])
                continue;

            int32_t block_x_start = bx << REGION_SIZE_SHIFT;
            int32_t block_y_start = by << REGION_SIZE_SHIFT;
            int32_t block_x_end = block_x_start + REGION_SIZE;
            int32_t block_y_end = block_y_start + REGION_SIZE;

            int32_t draw_x_start = (win_x_start > block_x_start) ? win_x_start : block_x_start;
            int32_t draw_y_start = (win_y_start > block_y_start) ? win_y_start : block_y_start;
            int32_t draw_x_end = (win_x_end < block_x_end) ? win_x_end : block_x_end;
            int32_t draw_y_end = (win_y_end < block_y_end) ? win_y_end : block_y_end;

            for (int32_t sy = draw_y_start; sy < draw_y_end; sy++) {
                int32_t wy = sy - window->y;

                for (int32_t sx = draw_x_start; sx < draw_x_end; sx++) {
                    int32_t wx = sx - window->x;

                    uintptr_t screen_off = sy * video_width + sx;
                    uintptr_t win_off = wy * window->width + wx;

                    ((uint32_t *)video_buffer)[screen_off] = ((uint32_t *)window->buffer.base)[win_off];
                    res = 1;
                }
            }
        }
    }

    /* Draw window frame if successful draw above */
    if (res) {
        struct ism_buffer vbuf = {
            .base = video_buffer, .size = video_map.size, .width = video_width, .height = video_height};

        /* Draw frame and border around window */
        if (window->flags & ISM_WINDOW_FLAG_BORDER) {
            fill_rect(vbuf, 0xAA, window->x - 4, window->y - 4, 4, window->height + 8, 1);
            fill_rect(vbuf, 0xAA, window->x - 4, window->y - 4, window->width + 8, 4, 1);
            fill_rect(vbuf, 0xAA, window->x + window->width, window->y - 4, 4, window->height + 8, 1);
            fill_rect(vbuf, 0xAA, window->x - 4, window->y + window->height, window->width + 8, 4, 1);

            fill_rect(vbuf, 0x55, window->x, window->y, 1, window->height, 1);
            fill_rect(vbuf, 0x55, window->x, window->y, window->width + 1, 1, 1);
            fill_rect(vbuf, 0xFF, window->x + window->width, window->y + 1, 1, window->height, 1);
            fill_rect(vbuf, 0xFF, window->x, window->y + window->height, window->width + 1, 1, 1);

            fill_rect(vbuf, 0xFF, window->x - 4, window->y - 4, 1, window->height + 8, 1);
            fill_rect(vbuf, 0xFF, window->x - 4, window->y - 4, window->width + 8, 1, 1);
            fill_rect(vbuf, 0x00, window->x + window->width + 4, window->y - 4, 1, window->height + 9, 1);
            fill_rect(vbuf, 0x00, window->x - 4, window->y + window->height + 4, window->width + 9, 1, 1);
        }

        if (window->flags & ISM_WINDOW_FLAG_FRAME) {
            fill_rect(vbuf, 0xAA, window->x, window->y - 3, window->width + 1, WINDOW_FRAME_HEIGHT, 1);
            fill_rect(vbuf, 0x55, window->x, window->y + WINDOW_FRAME_HEIGHT - 3, window->width + 1, 1, 1);

            draw_bold_frame(vbuf, window->x + 4, window->y + 1, 16, 16, window->controls_state[0]);
            draw_bold_frame(vbuf, window->x + 28, window->y + 1, 16, 16, window->controls_state[1]);
            draw_bold_frame(vbuf, window->x + 52, window->y + 1, 16, 16, window->controls_state[2]);

            /* Redraw name only if required */
            if (window->has_name_changed) {
                if (window->name_buffer.base)
                    free(window->name_buffer.base);
                window->has_name_changed = 0;
                ssfn_select(&font_ctx, SSFN_FAMILY_SERIF, NULL, SSFN_STYLE_REGULAR, 16);

                int width, height, left, top;
                ssfn_bbox(&font_ctx, window->name, &width, &height, &left, &top);
                window->name_buffer.size = width * height * 4;
                window->name_buffer.width = width;
                window->name_buffer.height = height;
                window->name_buffer.base = malloc(window->name_buffer.size);
                fill_rect(window->name_buffer, 0xAA, 0, 0, window->name_buffer.width, window->name_buffer.height, 1);

                ssfn_buf_t buf = {.ptr = window->name_buffer.base,
                                  .w = window->name_buffer.width,
                                  .h = window->name_buffer.height,
                                  .p = window->name_buffer.width * 4,
                                  .x = 0,
                                  .y = height - 4,
                                  .fg = 0xFF000000};

                char *ptr = window->name;
                while (*ptr) {
                    int res = ssfn_render(&font_ctx, &buf, ptr);
                    ptr += res;
                }
            }

            /* Draw buffered name */
            blit_buffer(vbuf, window->name_buffer, window->x + (window->width >> 1) - (window->name_buffer.width >> 1),
                        window->y);
        }
    }

    return res;
}

static void update_window(struct ism_window *window) {}

static void render_window(struct ism_window *window) {
    if (window->window_id == ISM_WINDOW_ROOT) {
        /* Draw window background and contents */
        fill_rect(window->buffer, window->bg_color, 0, 0, window->width, window->height, 3);
    } else {
        /* Note: Apps owning the window must manager the rendering on their side */
    }

    /* Mark region to rerender (With slight offset to cover window frame) */
    mark_region_range(window->x - 8, window->y - 8 - WINDOW_FRAME_HEIGHT, window->width + 16,
                      window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);
}

static void move_front_window(struct ism_window *window) {
    if (!window || !window->parent)
        return;
    if (list_move_to_end(&window->list, &window->parent->children) == 0) {
        render_window(window);
    }
}

static void foreach_render_window(struct ism_window *window) {
    if (!copy_window_buffer(window))
        return;

    struct list_head *pos;
    struct ism_window *entry;
    list_for_each(pos, &window->children) {
        entry = list_entry(pos, struct ism_window, list);
        foreach_render_window(entry);
    }
}

int ism_window_update(void) {
    cursor_style = ISM_CURSOR_NORMAL;
    static struct ism_window *held_window = NULL;
    static int last_mouse_x = -1, last_mouse_y = -1, prev_mouse_state = -1;
    struct ism_mouse mouse = {0};
    if (input_read(&mouse, NULL) != 0)
        return 0;
    struct ism_window *window = get_window_position(mouse.pos_x, mouse.pos_y);

    if (prev_mouse_state != mouse.buttons[MOUSE_BTN1] && window) {
        if (mouse.buttons[MOUSE_BTN1] && !held_window && (mouse.pos_y - window->y - 3) < WINDOW_FRAME_HEIGHT)
            held_window = window;
        else if (!mouse.buttons[MOUSE_BTN1])
            held_window = NULL;
    }

    /* Check if any new mouse events occurred */
    if (mouse.pos_x != last_mouse_x || mouse.pos_y != last_mouse_y) {
        /* Drag window using mouse */
        if (held_window) {
            /* Mark the previous window position region for redraw (With slight offset to cover window frame) */
            mark_region_range(held_window->x - 8, held_window->y - 8 - WINDOW_FRAME_HEIGHT, held_window->width + 16,
                              held_window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);

            held_window->x += mouse.pos_x - last_mouse_x;
            held_window->y += mouse.pos_y - last_mouse_y;

            update_window(held_window);
            move_front_window(held_window);
        }

        last_mouse_x = mouse.pos_x;
        last_mouse_y = mouse.pos_y;
    }

    /* Move to front if mouse clicked */
    if (mouse.buttons[MOUSE_BTN1] && prev_mouse_state != mouse.buttons[MOUSE_BTN1] && window)
        move_front_window(window);

    if (window && window->flags & ISM_WINDOW_FLAG_FRAME) {
        /* Send SIGQUIT to owner of a window */
        if (window->owner != 0 && window->controls_state[0] && !mouse.buttons[MOUSE_BTN1]) {
            signal(window->owner, SIGQUIT);
            window->controls_state[0] = 0;
        }

        /* TEST */
        if (window->controls_state[1] && !mouse.buttons[MOUSE_BTN1]) {
            execp(NULL, "/programs/cube.exe");
            window->controls_state[1] = 0;
        }

        if (window->controls_state[2] && !mouse.buttons[MOUSE_BTN2]) {
            printf("Test click!\n");
        }
        /* --------------- */

        /* Check if mouse hovers window controls */
        if (mouse.pos_y >= window->y + 1 && mouse.pos_y < window->y + 17) {
            /* Close button */
            if (window->controls_state[0] || (mouse.pos_x >= window->x && mouse.pos_x < window->x + 16)) {
                cursor_style = ISM_CURSOR_HAND;
                window->controls_state[0] = mouse.buttons[MOUSE_BTN1];
            }
            /* Maximize button */
            else if (window->controls_state[1] || (mouse.pos_x >= window->x + 24 && mouse.pos_x < window->x + 40)) {
                cursor_style = ISM_CURSOR_HAND;
                window->controls_state[1] = mouse.buttons[MOUSE_BTN1];
            }
            /* Minimize button */
            else if (window->controls_state[2] || (mouse.pos_x >= window->x + 48 && mouse.pos_x < window->x + 64)) {
                cursor_style = ISM_CURSOR_HAND;
                window->controls_state[2] = mouse.buttons[MOUSE_BTN1];
            }
        }

        if (prev_mouse_state != mouse.buttons[MOUSE_BTN1])
            render_window(window);
    }

    prev_mouse_state = mouse.buttons[MOUSE_BTN1];
    return 0;
}

int ism_window_render(void) {
    static int last_mouse_x = 0, last_mouse_y = 0;
    uint8_t *cursor = (uint8_t *)cursor_normal;
    struct ism_mouse mouse;
    uintptr_t off;
    uint8_t c = 0;
    int32_t w = (video_width >> REGION_SIZE_SHIFT), h = (video_height >> REGION_SIZE_SHIFT);
    int32_t r_y, r_x, c_x, c_y, x, y;

    switch (cursor_style) {
    case ISM_CURSOR_BEAM:
        cursor = (uint8_t *)cursor_ibeam;
        break;
    case ISM_CURSOR_HOURGLASS:
        cursor = (uint8_t *)cursor_hourglass;
        break;
    case ISM_CURSOR_HAND:
        cursor = (uint8_t *)cursor_hand;
        break;
    }

    foreach_render_window(&root_window);

    /* Render mouse cursor */
    if (input_read(&mouse, NULL) == 0) {
        if (last_mouse_x != mouse.pos_x || last_mouse_y != mouse.pos_y) {
            mark_region_range((int32_t)last_mouse_x - MOUSE_WIDTH, (int32_t)last_mouse_y - MOUSE_WIDTH, MOUSE_WIDTH * 2,
                              MOUSE_HEIGHT * 2, 1);
            last_mouse_x = (int)mouse.pos_x;
            last_mouse_y = (int)mouse.pos_y;
            mark_region_range((int32_t)last_mouse_x - MOUSE_WIDTH, (int32_t)last_mouse_y - MOUSE_WIDTH, MOUSE_WIDTH * 2,
                              MOUSE_HEIGHT * 2, 1);
        }
    }

    /* Update screen buffer */
    for (y = 0; y <= h; y++)
        for (x = 0; x <= w; x++) {
            if (!dirty_regions[y * w + x])
                continue;
            dirty_regions[y * w + x] = 0;
            for (r_y = 0; r_y < REGION_SIZE; r_y++)
                for (r_x = 0; r_x < REGION_SIZE; r_x++) {
                    c_y = ((y << REGION_SIZE_SHIFT) + r_y);
                    c_x = ((x << REGION_SIZE_SHIFT) + r_x);
                    off = c_y * video_width + c_x;
                    if ((off << 2) >= video_map.size)
                        continue;
                    /* Draw mouse */
                    if (c_y >= last_mouse_y && c_y < last_mouse_y + MOUSE_HEIGHT && c_x >= last_mouse_x &&
                        c_x < last_mouse_x + MOUSE_WIDTH) {
                        c = cursor[(c_y - last_mouse_y) * MOUSE_WIDTH + (c_x - last_mouse_x)];
                        if (c != T_CLR) {
                            ((uint32_t *)video_map.base)[off] = (((((0xff << 8) | c) << 8) | c) << 8) | c;
                            continue;
                        }
                    }

                    ((uint32_t *)video_map.base)[off] = ((uint32_t *)video_buffer)[off];
                }
        }

    return 0;
}

#pragma GCC pop_options

int ism_window_init(void) {
    if (ism_video_resolution(&video_width, &video_height, NULL) != 0)
        return -1;
    if (ism_video_map(&video_map) != 0)
        return -1;

    if ((dirty_regions =
             malloc(((video_width >> REGION_SIZE_SHIFT) + 1) * ((video_height >> REGION_SIZE_SHIFT) + 1))) == NULL) {
        printf("%s: Unable to allocate memory.\n", get_name());
        return -1;
    }

    if ((video_buffer = malloc(video_map.size)) == NULL) {
        printf("%s: Unable to allocate memory.\n", get_name());
        return -1;
    }

    /* Load font to use for window title rendering (and supposedly other stuff in future) */
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

    /* Mark all regions as dirty at first */
    memset(dirty_regions, 1, (video_width >> REGION_SIZE_SHIFT) * (video_height >> REGION_SIZE_SHIFT));

    memcpy((void *)root_window.name, "root\0", 5);
    INIT_LIST_HEAD(&root_window.children);
    root_window.x = 0;
    root_window.y = 0;
    root_window.width = video_width;
    root_window.height = video_height;
    root_window.buffer.width = video_width;
    root_window.buffer.height = video_height;
    root_window.window_id = ISM_WINDOW_ROOT;
    root_window.flags = 0;
    root_window.bg_color = ISM_WINDOW_ROOT_BGCOLOR;
    root_window.buffer.size = root_window.width * root_window.height * 4;
    root_window.buffer.base = malloc(root_window.buffer.size);
    root_window.has_name_changed = 1;
    root_window.name_buffer.base = 0;
    root_window.owner = 0;
    render_window(&root_window);
    return 0;
}

int ism_window_cleanup(void) {
    if (video_buffer)
        free(video_buffer);
    if (font_buf)
        free(font_buf);
    if (dirty_regions)
        free(dirty_regions);
    dirty_regions = NULL;
    video_buffer = NULL;
    font_buf = NULL;
    ssfn_free(&font_ctx);
    return 0;
}

int ism_window_create(pid_t owner, wind_t *id, wind_t parent, const char *name, uint32_t flags, int32_t x, int32_t y,
                      int32_t width, int32_t height) {
    struct ism_window *parent_window = get_window(parent);
    if (!parent_window)
        return -1;

    struct ism_window *new_window = (struct ism_window *)malloc(sizeof(struct ism_window));
    INIT_LIST_HEAD(&new_window->children);
    memcpy((void *)new_window->name, name,
           (strlen(name) + 1) > ISM_MAX_WINDOW_NAME_LN ? ISM_MAX_WINDOW_NAME_LN : strlen(name) + 1);
    new_window->name[ISM_MAX_WINDOW_NAME_LN - 1] = 0;
    new_window->window_id = last_frid++;
    new_window->x = x;
    new_window->y = y;
    new_window->width = width;
    new_window->height = height;
    new_window->buffer.width = width;
    new_window->buffer.height = height;
    new_window->bg_color = 0xBBBBBB;
    new_window->flags = flags;
    new_window->parent = parent_window;
    new_window->buffer.size = new_window->width * new_window->height * 4;
    new_window->owner = owner;
    new_window->buffer.base = malloc(new_window->buffer.size);
    new_window->has_name_changed = 1;
    new_window->name_buffer.base = 0;
    list_add(&new_window->list, &parent_window->children);
    if (id)
        *id = new_window->window_id;
    render_window(new_window);
    return 0;
}

int ism_window_destroy(wind_t id) {
    if (id == ISM_WINDOW_ROOT)
        return -1;
    struct ism_window *window = get_window(id);
    if (!window)
        return -1;
    /* Mark region to rerender, so we wont leave ghosts after destroying (With slight offset to cover window frame) */
    mark_region_range(window->x - 8, window->y - 8 - WINDOW_FRAME_HEIGHT, window->width + 16,
                      window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);
    list_del(&window->list);
    free(window->buffer.base);
    free(window);

    /* TODO: cleanup children too */
    return 0;
}

int ism_window_destroy_owner(pid_t owner) {
    if (owner == 0)
        return -1;
    struct ism_window *window = get_window_pid(owner);
    if (!window)
        return -1;
    /* Mark region to rerender, so we wont leave ghosts after destroying (With slight offset to cover window frame) */
    mark_region_range(window->x - 8, window->y - 8 - WINDOW_FRAME_HEIGHT, window->width + 16,
                      window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);
    list_del(&window->list);
    free(window->buffer.base);
    free(window);

    /* TODO: cleanup children too */
    return 0;
}

int window_handle_event(pid_t source, uint32_t message, void *data, size_t data_sz) {
    int res = -EINVAL;
    wind_t wind;

    struct ism_create_window *create_message = data;
    struct ism_draw_window_payload *draw_message = data;
    struct ism_window *window;
    struct ism_buffer buf0; //, buf1;

    // struct ism_buffer vbuf = {
    //     .base = video_buffer, .size = video_map.size, .width = video_width, .height = video_height};
    switch (message) {
    case ISM_IPC_CREATE_WINDOW:
        if (data_sz != sizeof(struct ism_create_window))
            goto err;
        if ((res = ism_window_create(source, &wind, create_message->parent, create_message->name, create_message->flags,
                                     create_message->x, create_message->y, create_message->width,
                                     create_message->height)) != 0)
            goto err;

        return wind;
    case ISM_IPC_DRAW_WINDOW:
        if (draw_message->bpp != 32) {
            /* TODO: Support other BPPs */
            return -EINVAL;
        }

        window = get_window_pid(source);
        if (!window)
            return -EINVAL;

        buf0 = (struct ism_buffer){.base = (uint8_t *)&draw_message->buffer,
                                   .size = draw_message->width * draw_message->height * 4,
                                   .width = draw_message->width,
                                   .height = draw_message->height};
        blit_buffer(window->buffer, buf0, draw_message->x, draw_message->y);
        render_window(window);
        return 0;
    case 0xFFFFFFFF: /* Connection closed */
        ism_window_destroy_owner(source);
        return 0;
    }

err:
    return res;
}
