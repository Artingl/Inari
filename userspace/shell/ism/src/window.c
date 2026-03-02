#include <io.h>
#include <sys.h>
#include <list.h>
#include <lib.h>
#include <string.h>

#include "core.h"
#include "window.h"
#include "input.h"
#include "video.h"
#include "cursor.h"

#define WINDOW_FRAME_HEIGHT 20

#define draw_bold_frame(buffer, x, y, width, height) do { \
        fill_rect(buffer, 0xAA, x, y, width, height, 1);  \
        fill_rect(buffer, 0xDD, x, y, 2, height, 1);  \
        fill_rect(buffer, 0xDD, x, y + 1, width, 2, 1);   \
        fill_rect(buffer, 0x33, x, y + height, width, 2, 1);  \
        fill_rect(buffer, 0x33, x + width - 2, y + 1, 2, height - 1, 1); } while (0)

#define collides_element(x, y, element) ( (int32_t)x >= (int32_t)(element)->x && (int32_t)y >= (int32_t)(element)->y && \
                                          (int32_t)x < (int32_t)(element)->x + (int32_t)(element)->width && \
                                          (int32_t)y < (int32_t)(element)->y + (int32_t)(element)->height )

static wind_t last_frid = 1;
static struct ism_window root_window;

#define REGION_SIZE_SHIFT 4
#define REGION_SIZE       (2<<(REGION_SIZE_SHIFT-1))

static uint8_t *video_buffer = NULL;
static uint8_t *dirty_regions = NULL;
static struct ism_video_map video_map;
static struct ism_window *held_window = NULL;
static int32_t video_width, video_height;

#pragma GCC push_options
#pragma GCC optimize("O3")

static inline int fill_rect(struct ism_buffer buffer, uint32_t color, int32_t x_orig, int32_t y_orig, int32_t width, int32_t height, uint32_t px_len) {
    int32_t x, y;
    uint8_t r, g, b;
    uintptr_t off;

    for (y = y_orig; y < height + y_orig; y++)
        for (x = x_orig; x < width + x_orig; x++) {
            if (y >= buffer.height || x >= buffer.width || x < 0 || y < 0)
                continue;
            off = y * buffer.width + x;
            if (off >= buffer.size)
                continue;
            r = px_len > 2 ? color >> 16 : (uint8_t)color;
            g = px_len > 2 ? (color >> 8) & 0xff : (uint8_t)color;
            b = px_len > 2 ? color & 0xff : (uint8_t)color;

            ((uint32_t*)buffer.base)[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
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
    if (!window) return NULL;
    if (window->window_id == id)  return window;
    struct list_head *pos;
    struct ism_window *entry, *found;
    list_for_each(pos, &window->children) {
        entry = list_entry(pos, struct ism_window, list);
        if (entry->window_id == id)  return entry;
        if ((found = foreach_children(id, entry)) != NULL)
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
        if (collides_element(x, y, entry))  return entry;
    }

    return NULL;
}

static struct ism_window *get_window(wind_t id) {
    return foreach_children(id, &root_window);
}

static int copy_window_buffer(struct ism_window *window) {
    int res = 0;
    
    int32_t w_cols = (video_width + REGION_SIZE - 1) >> REGION_SIZE_SHIFT;
    
    int32_t win_x_start = window->x;
    int32_t win_y_start = window->y;
    int32_t win_x_end = window->x + window->width;
    int32_t win_y_end = window->y + window->height;

    if (win_x_start < 0) win_x_start = 0;
    if (win_y_start < 0) win_y_start = 0;
    if (win_x_end > video_width) win_x_end = video_width;
    if (win_y_end > video_height) win_y_end = video_height;

    if (win_x_start >= win_x_end || win_y_start >= win_y_end)
        return 0; 

    int32_t start_bx = win_x_start >> REGION_SIZE_SHIFT;
    int32_t start_by = win_y_start >> REGION_SIZE_SHIFT;
    int32_t end_bx = (win_x_end - 1) >> REGION_SIZE_SHIFT;
    int32_t end_by = (win_y_end - 1) >> REGION_SIZE_SHIFT;

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
                    
                    ((uint32_t*)video_buffer)[screen_off] = ((uint32_t*)window->buffer.base)[win_off];
                    res = 1;
                }
            }
        }
    }

    /* Draw window frame if successful draw above */
    if (res) {
        struct ism_buffer vbuf = {
            .base = video_buffer,
            .size = video_map.size,
            .width = video_width,
            .height = video_height
        };

        /* Draw frame and border around window */
        if (window->flags & ISM_WINDOW_FLAG_BORDER) {
            fill_rect(vbuf, 0xAA, window->x - 4,                  window->y - 4, 4, window->height + 8, 1);
            fill_rect(vbuf, 0xAA, window->x - 4,                  window->y - 4, window->width + 8, 4, 1);
            fill_rect(vbuf, 0xAA, window->x + window->width,      window->y - 4, 4, window->height + 8, 1);
            fill_rect(vbuf, 0xAA, window->x - 4,                  window->y + window->height, window->width + 8, 4, 1);

            fill_rect(vbuf, 0x22, window->x,                      window->y, 1, window->height, 1);
            fill_rect(vbuf, 0x22, window->x,                      window->y, window->width + 1, 1, 1);
            fill_rect(vbuf, 0x66, window->x + window->width,      window->y + 1, 1, window->height, 1);
            fill_rect(vbuf, 0x66, window->x,                      window->y + window->height, window->width + 1, 1, 1);

            fill_rect(vbuf, 0xFF, window->x - 4,                  window->y - 4, 1, window->height + 8, 1);
            fill_rect(vbuf, 0xFF, window->x - 4,                  window->y - 4, window->width + 8, 1, 1);
            fill_rect(vbuf, 0xCC, window->x + window->width + 4,  window->y - 4, 1, window->height + 9, 1);
            fill_rect(vbuf, 0xCC, window->x - 4,                  window->y + window->height + 4, window->width + 9, 1, 1);
        }

        if (window->flags & ISM_WINDOW_FLAG_FRAME) {
            fill_rect(vbuf, 0xAA, window->x, window->y - 3, window->width + 1, WINDOW_FRAME_HEIGHT, 1);
            fill_rect(vbuf, 0x33, window->x, window->y + WINDOW_FRAME_HEIGHT - 3, window->width + 1, 1, 1);

            // draw_bold_frame(window->buffer, window->x - 2, window->y - 2 , window->width + 5, WINDOW_FRAME_HEIGHT);
        }
    }

    return res;
}

static void render_window(struct ism_window *window) {
    /* Draw window background and contents */
    fill_rect(window->buffer, window->bg_color, 0, 0, window->width, window->height, 3);

    /* Slight offset to cover window frame */
    mark_region_range(window->x - 8, window->y - 8 - WINDOW_FRAME_HEIGHT, window->width + 16, window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);
}

static void move_front_window(struct ism_window *window) {
    if (!window || !window->parent) return;
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
    static int last_mouse_x = -1, last_mouse_y = -1, prev_mouse_state = -1;
    struct ism_mouse mouse;
    struct ism_window *window;
    if (input_read(&mouse, NULL) != 0)
        return 0;
    
    /* Check if any new mouse events occurred */
    if (mouse.pos_x != last_mouse_x || mouse.pos_y != last_mouse_y) {
        if (mouse.buttons[MOUSE_BTN1] && held_window)
            window = held_window;
        else {
            held_window = NULL;
            window = get_window_position(mouse.pos_x, mouse.pos_y);
        }
        
        /* Move window using mouse */
        if (window && mouse.buttons[MOUSE_BTN1] && ((mouse.pos_y - window->y) < WINDOW_FRAME_HEIGHT || held_window)) {
            /* Mark the previous window position region for redraw (With slight offset to cover window frame) */
            mark_region_range(window->x - 8, window->y - 8 - WINDOW_FRAME_HEIGHT, window->width + 16, window->height + 16 + WINDOW_FRAME_HEIGHT * 2, 1);

            window->x += mouse.pos_x - last_mouse_x;
            window->y += mouse.pos_y - last_mouse_y;
            held_window = window;
            
            move_front_window(held_window);
        }

        last_mouse_x = mouse.pos_x;
        last_mouse_y = mouse.pos_y;
    }

    /* Move to front if mouse clicked */
    if (mouse.buttons[MOUSE_BTN1] && prev_mouse_state != mouse.buttons[MOUSE_BTN1] && (window = get_window_position(mouse.pos_x, mouse.pos_y)))
        move_front_window(window);

    if (prev_mouse_state != mouse.buttons[MOUSE_BTN1])
        prev_mouse_state = mouse.buttons[MOUSE_BTN1];

    return 0;
}

int ism_window_render(void) {
    static int last_mouse_x = 0, last_mouse_y = 0;
    uint8_t *cursor = (uint8_t*)cursor_normal;
    struct ism_mouse mouse;
    uintptr_t off;
    uint8_t c = 0;
    int32_t w = (video_width >> REGION_SIZE_SHIFT), h = (video_height >> REGION_SIZE_SHIFT);
    int32_t r_y, r_x, c_x, c_y, x, y;

    foreach_render_window(&root_window);

    /* Render mouse cursor */
    if (input_read(&mouse, NULL) == 0) {
        if (last_mouse_x != mouse.pos_x || last_mouse_y != mouse.pos_y) {
            mark_region_range((int32_t)last_mouse_x - MOUSE_WIDTH, (int32_t)last_mouse_y - MOUSE_WIDTH, MOUSE_WIDTH * 2, MOUSE_HEIGHT * 2, 1);
            last_mouse_x = (int)mouse.pos_x;
            last_mouse_y = (int)mouse.pos_y;
            mark_region_range((int32_t)last_mouse_x - MOUSE_WIDTH, (int32_t)last_mouse_y - MOUSE_WIDTH, MOUSE_WIDTH * 2, MOUSE_HEIGHT * 2, 1);
        }
    }

    /* Update screen buffer */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            if (!dirty_regions[y * w + x])
                continue;
            dirty_regions[y * w + x] = 0;
            for (r_y = 0; r_y < REGION_SIZE; r_y++)
                for (r_x = 0; r_x < REGION_SIZE; r_x++) {
                    c_y = ((y << REGION_SIZE_SHIFT) + r_y);
                    c_x = ((x << REGION_SIZE_SHIFT) + r_x);
                    off = c_y * video_width + c_x;
                    if (off >= video_map.size)
                        continue;
                    /* Draw mouse */
                    if (c_y >= last_mouse_y && c_y < last_mouse_y + MOUSE_HEIGHT && c_x >= last_mouse_x && c_x < last_mouse_x + MOUSE_WIDTH) {
                        c = cursor[(c_y - last_mouse_y) * MOUSE_WIDTH + (c_x - last_mouse_x)];
                        if (c != T) {
                            ((uint32_t*)video_map.base)[off] = (((((0xff << 8) | c) << 8) | c) << 8) | c;
                            continue;
                        }
                    }

                    ((uint32_t*)video_map.base)[off] = ((uint32_t*)video_buffer)[off];
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

    if ((dirty_regions = malloc((video_width >> REGION_SIZE_SHIFT) * (video_height >> REGION_SIZE_SHIFT))) == NULL) {
        printf("%s: Unable to allocate memory.\n", get_name());
        return -1;
    }

    if ((video_buffer = malloc(video_map.size)) == NULL) {
        printf("%s: Unable to allocate memory.\n", get_name());
        return -1;
    }

    /* Mark all regions as dirty at first */
    memset(dirty_regions, 1, (video_width >> REGION_SIZE_SHIFT) * (video_height >> REGION_SIZE_SHIFT));

    memcpy((void*)root_window.name, "root\0", 5);
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
    render_window(&root_window);
    return 0;
}

int ism_window_cleanup(void) {
    if (video_buffer) free(video_buffer);
    if (dirty_regions) free(dirty_regions);
    dirty_regions = NULL;
    video_buffer = NULL;
    return 0;
}

int ism_window_create(wind_t *id, wind_t parent, const char *name, uint32_t flags, int32_t x, int32_t y, int32_t width, int32_t height) {
    struct ism_window *parent_window = get_window(parent);
    if (!parent_window)  return -1;

    struct ism_window *new_window = (struct ism_window*)malloc(sizeof(struct ism_window));
    INIT_LIST_HEAD(&new_window->children);
    memcpy((void*)new_window->name, name, (strlen(name) + 1) > ISM_MAX_WINDOW_NAME_LN ? ISM_MAX_WINDOW_NAME_LN : strlen(name) + 1);
    new_window->name[ISM_MAX_WINDOW_NAME_LN - 1] = 0;
    new_window->window_id = last_frid++;
    new_window->x = x;
    new_window->y = y;
    new_window->width = width;
    new_window->height = height;
    new_window->buffer.width = width;
    new_window->buffer.height = height;
    new_window->bg_color = new_window->window_id == 1 ? 0x00BBBB :0xBB00BB;
    new_window->flags = flags;
    new_window->parent = parent_window;
    new_window->buffer.size = new_window->width * new_window->height * 4;
    new_window->buffer.base = malloc(new_window->buffer.size);
    list_add(&new_window->list, &parent_window->children);
    if (id) *id = new_window->window_id;
    render_window(new_window);
    return 0;
}

int ism_window_destroy(wind_t id) {
    if (id == ISM_WINDOW_ROOT)   return -1;
    struct ism_window *window = get_window(id);
    if (!window) return -1;
    list_del(&window->list);
    free(window->buffer.base);
    free(window);

    /* TODO: cleanup children too */
    return 0;
}
