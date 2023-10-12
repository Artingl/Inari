#include <kernel/kernel.h>
#include <kernel/sys/console/console.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

#include <drivers/video/video.h>
#include <drivers/video/font.h>

struct console console;

int __heap_allocated = 0;
int __must_rerender = 0;
int __has_serial = 0;

void __printc(char c);

void sys_console_init()
{
    memset(&console, 0, sizeof(struct console));

    // initialize serial for the console
    __has_serial = serial_init(CONSOLE_SERIAL_PORT, CONSOLE_SERIAL_BAUD) == SERIAL_SUCCESS;
}

void sys_console_enable_heap()
{
    if (!__heap_allocated)
    {
        console.buffer_width = video_text_width();
        console.buffer_height = video_text_height();
        console.buffer = kcalloc(sizeof(uint32_t), console.buffer_width * console.buffer_height);
        console.lines_state = kcalloc(sizeof(uint32_t), console.buffer_height);
        memset(console.buffer, 0, console.buffer_width * console.buffer_height * sizeof(uint32_t));
        memset(console.lines_state, 0, console.buffer_height * sizeof(uint32_t));
        __heap_allocated = true;
    }
}

void sys_console_update_video()
{
    if (!__heap_allocated)
    {
        printk(KERN_WARNING "CONSOLE: buffer is not heap allocated.");
        return;
    }

    size_t i;
    uint32_t *old_buffer = console.buffer;
    uint32_t *old_line_states = console.lines_state;
    uint32_t old_width = console.buffer_width,
             old_height = console.buffer_height;
    uint8_t new_width = video_text_width();
    uint8_t new_height = video_text_height();

    uint32_t *new_buffer = kcalloc(sizeof(uint32_t), new_width * new_height);
    uint32_t *line_states = kcalloc(sizeof(uint32_t), new_height);
    if (new_buffer == NULL || line_states == NULL)
    {
        kfree(new_buffer);
        kfree(line_states);
        printk(KERN_WARNING "CONSOLE: Unable to reallocate memory for kernel console.");
        return;
    }
    memset(new_buffer, 0, new_width * new_height * sizeof(uint32_t));
    memset(line_states, 0, new_height * sizeof(uint32_t));

    // copy lines to new buffer
    for (i = 0; i < new_height && i < old_height; i++)
    {
        memcpy(
            &new_buffer[i * new_width], &old_buffer[i * old_width], min(new_width, old_width) * 4);
    }

    console.lines_state = line_states;
    console.buffer = new_buffer;
    console.buffer_width = new_width;
    console.buffer_height = new_height;

    kfree(old_buffer);
    kfree(old_line_states);
    sys_console_flush();
}

int sys_console_print(const char *msg)
{
    char c;
    int i = 0;

    while (c = *(msg++))
    {
        // use special routine to print the character onto the screen
        // (will check if the character is NL, TAB, etc.)
        __printc(c);
        i++;
    }

    if (__must_rerender)
        sys_console_render();
    return i;
}

int sys_console_printc(char c)
{
    __printc(c);

    if (__must_rerender)
        sys_console_render();
    return 1;
}

void sys_console_render()
{
    sys_console_flush();
#if 0
    size_t i, offset;

    // print to the screen only if we allocated the buffer on heap
    if (__heap_allocated)
    {
        // update only lines that changed since last render
        for (i = 0; i < console.buffer_height; i++)
        {
            if (console.lines_state[i] & CONSOLE_LINE_UPDATED)
            {
                console.lines_state[i] &= ~CONSOLE_LINE_UPDATED;
                offset = i * console.buffer_width;

                video_text_print_at(&console.buffer[offset], 0x07, offset, console.buffer_width);
            }
        }
        __must_rerender = false;
    }
#endif
}

void sys_console_flush()
{
    size_t i, offset;

    // print to the screen only if we allocated the buffer on heap
    if (__heap_allocated)
    {
        video_text_render(console.buffer, console.buffer_width, console.buffer_height);
        __must_rerender = false;
    }
}

void __printc(char c)
{
    if (__has_serial)
    {
        if (c == '\n')
        {
            serial_putc(CONSOLE_SERIAL_PORT, '\r');
            serial_putc(CONSOLE_SERIAL_PORT, '\n');
        }
        else if (c == '\t')
        {
            serial_putc(CONSOLE_SERIAL_PORT, ' ');
            serial_putc(CONSOLE_SERIAL_PORT, ' ');
            serial_putc(CONSOLE_SERIAL_PORT, ' ');
            serial_putc(CONSOLE_SERIAL_PORT, ' ');
        }
        else
            serial_putc(CONSOLE_SERIAL_PORT, c);
    }

    // print to the screen only if we allocated the buffer on heap
    if (__heap_allocated)
    {
        size_t last_line_offset, width = console.buffer_width, offset = 0;

        // scroll the buffer if required
        if (console.offset_y >= console.buffer_height)
        {
            console.offset_y = console.buffer_height - 1;
            last_line_offset = console.buffer_width * console.buffer_height - console.buffer_width;
            
            console.lines_state[console.offset_y]   |= CONSOLE_LINE_UPDATED;
            console.lines_state[console.offset_y-1] |= CONSOLE_LINE_UPDATED;

            memcpy(
                &console.buffer[0],
                &console.buffer[console.buffer_width],
                last_line_offset * sizeof(uint32_t));

            memset(&console.buffer[last_line_offset], 0, console.buffer_width * sizeof(uint32_t));
        }

        // handle "special" ascii codes
        switch (c)
        {
        case '\n':
        {
            // set the X offset to the length of the screen, so it would move the cursor to the next line
            console.offset_x = width;
            goto end;
        }
        case '\t':
        {
            console.offset_x += CONSOLE_TAB_SIZE;
            goto end;
        }
        }

        if (c == '\n')
        {
            console.offset_x++;
            goto end;
        }

        // print the char if it is not "special"
        offset = console.offset_y * width + console.offset_x;

        if (IS_UNICODE(c) && !console.is_unicode)
        {
            console.unicode_bytes = (c & 0x20) ? ((c & 0x10) ? ((c & 0x08) ? ((c & 0x04) ? 6 : 5) : 4) : 3) : 2;
            console.unicode_bytes_start = console.unicode_bytes;
            console.is_unicode = true;
        }

        if (console.is_unicode)
        {
            ((uint8_t *)&console.buffer[offset])[console.unicode_bytes_start - console.unicode_bytes] = c;
            console.unicode_bytes--;

            if (console.unicode_bytes <= 0)
            {
                console.offset_x++;
                console.unicode_bytes = 0;
                console.unicode_bytes_start = 0;
                console.is_unicode = false;
            }
        }
        else
        {
            console.buffer[offset] = c;
            console.offset_x++;
        }

    end:
        while (console.offset_x >= width)
        {
            console.offset_x -= width;
            console.offset_y++;
            
            console.lines_state[console.offset_y+1] |= CONSOLE_LINE_UPDATED;
            console.lines_state[console.offset_y]   |= CONSOLE_LINE_UPDATED;
            console.lines_state[console.offset_y-1] |= CONSOLE_LINE_UPDATED;
            
            __must_rerender = true;
        }
    }
}