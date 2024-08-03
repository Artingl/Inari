#include <kernel/kernel.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/core/console/console.h>

#include <kernel/include/math.h>
#include <kernel/include/string.h>

struct console console;

void __console_printc(char c);

void console_init(uint16_t serial_port)
{
    memset(&console, 0, sizeof(struct console));
    // spinlock_init(&console.spinlock);

    // initialize serial for the console
    if (serial_init(serial_port, CONSOLE_SERIAL_BAUD) == SERIAL_SUCCESS)
    {
        console.serial_port = serial_port;
    }
    else {
        console.serial_port = 0;
    }

    console.heap_allocated = 0;
    console.must_flush = 0;

}

void console_clear()
{
    // spinlock_acquire(&console.spinlock);
    console.offset_x = 0;
    console.offset_y = 0;
    memset(console.buffer, 0, console.buffer_width * console.buffer_height * sizeof(uint32_t));
    memset(console.lines_state, 0, console.buffer_height * sizeof(uint32_t));
    // spinlock_release(&console.spinlock);
}

void console_late_init()
{
    // spinlock_acquire(&console.spinlock);
    if (!console.heap_allocated)
    {
        console.buffer_width = CONSOLE_WIDTH;
        console.buffer_height = CONSOLE_HEIGHT;
        console.buffer = kcalloc(sizeof(uint32_t), console.buffer_width * console.buffer_height);
        console.lines_state = kcalloc(sizeof(uint32_t), console.buffer_height);
        memset(console.buffer, 0, console.buffer_width * console.buffer_height * sizeof(uint32_t));
        memset(console.lines_state, 0, console.buffer_height * sizeof(uint32_t));
        console.heap_allocated = 1;
    }
    // spinlock_release(&console.spinlock);
}

int console_print(const char *msg)
{
    // spinlock_acquire(&console.spinlock);
    char c;
    int i = 0;

    while (c = *(msg++))
    {
        // use special routine to print the character onto the screen
        // (will check if the character is NL, TAB, etc.)
        __console_printc(c);
        i++;
    }

    if (console.must_flush)
        console_flush();
    // spinlock_release(&console.spinlock);
    return i;
}

int console_printc(char c)
{
    spinlock_acquire(&console.spinlock);
    __console_printc(c);

    if (console.must_flush)
        console_flush();
    spinlock_release(&console.spinlock);
    return 1;
}

void console_flush()
{
}

void __console_printc(char c)
{
    if (console.serial_port != 0)
    {
        if (c == '\n')
        {
            serial_putc(console.serial_port, '\r');
            serial_putc(console.serial_port, '\n');
        }
        else if (c == '\t')
        {
            serial_putc(console.serial_port, ' ');
            serial_putc(console.serial_port, ' ');
            serial_putc(console.serial_port, ' ');
            serial_putc(console.serial_port, ' ');
        }
        else
            serial_putc(console.serial_port, c);
    }
    return;

    // print to the screen only if we allocated the buffer on heap
    if (console.heap_allocated)
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

        // if (IS_UNICODE(c) && !console.is_unicode)
        // {
        //     console.unicode_bytes = (c & 0x20) ? ((c & 0x10) ? ((c & 0x08) ? ((c & 0x04) ? 6 : 5) : 4) : 3) : 2;
        //     console.unicode_bytes_start = console.unicode_bytes;
        //     console.is_unicode = true;
        // }

        // if (console.is_unicode)
        // {
        //     ((uint8_t *)&console.buffer[offset])[console.unicode_bytes_start - console.unicode_bytes] = c;
        //     console.unicode_bytes--;

        //     if (console.unicode_bytes <= 0)
        //     {
        //         console.offset_x++;
        //         console.unicode_bytes = 0;
        //         console.unicode_bytes_start = 0;
        //         console.is_unicode = false;
        //     }
        // }
        // else
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
            
            console.must_flush = true;
        }
    }
}