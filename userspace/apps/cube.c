// Source: https://github.com/saatvikrao/Spinning-Cube

#pragma GCC push_options
#pragma GCC optimize("O3")

#include <io.h>
#include <lib.h>
#include <math.h>
#include <string.h>
#include <sys.h>
#include <types.h>

#include <ism.h>

float A, B, C;

#define PX_SCALE 2
#define CUBE_W 30

float cubeWidth = CUBE_W;
int width = (CUBE_W * 2 + 10), height = (CUBE_W * 2 + 10);
float zBuffer[(CUBE_W * 2 + 10) * (CUBE_W * 2 + 10)];
int distanceFromCam = 140;
float horizontalOffset = 0;
float K1 = 40;

float incrementSpeed = 0.5;

float x, y, z;
float ooz;
int xp, yp;
int idx;

float sinA, cosA;
float sinB, cosB;
float sinC, cosC;

float calculateX(int i, int j, int k) {
    return (j * sinA * sinB * cosC - k * cosA * sinB * cosC + j * cosA * sinC + k * sinA * sinC +
            i * cosB * cosC);
}

float calculateY(int i, int j, int k) {
    return (j * cosA * cosC + k * sinA * cosC - j * sinA * sinB * sinC + k * cosA * sinB * sinC -
            i * cosB * sinC);
}

float calculateZ(int i, int j, int k) { return (k * cosA * cosB - j * sinA * cosB + i * sinB); }

void calculateForSurface(uint32_t *window_buffer, int w, int h, float cubeX, float cubeY, float cubeZ, uint32_t ch) {
    x = calculateX(cubeX, cubeY, cubeZ);
    y = calculateY(cubeX, cubeY, cubeZ);
    z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;

    ooz = 1 / z;

    xp = (int)(width / 2 + horizontalOffset + K1 * ooz * x * 2);
    yp = (int)(height / 2 + K1 * ooz * y);

    idx = xp + yp * width;
    if (idx >= 0 && idx < width * height) {
        if (ooz > zBuffer[idx]) {
            zBuffer[idx] = ooz;

            for (int sx = 0; sx < (1<<PX_SCALE); sx++)
                for (int sy = 0; sy < (1<<PX_SCALE); sy++) {
                    window_buffer[(sy + (yp << PX_SCALE)) * w + (sx + (xp << PX_SCALE))] = 0xff000000 | ch;
                }
        }
    }
}

void render_cube(uint32_t *window_buffer, int w, int h) {
    memset(zBuffer, 0, width * height * 4);

    sinA = sin(A), cosA = cos(A);
    sinB = sin(B), cosB = cos(B);
    sinC = sin(C), cosC = cos(C);

    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed) {
            calculateForSurface(window_buffer, w, h, cubeX, cubeY, -cubeWidth, 0xFFFFFF);
            calculateForSurface(window_buffer, w, h, cubeWidth, cubeY, cubeX, 0xCCCCCC);
            calculateForSurface(window_buffer, w, h, -cubeWidth, cubeY, -cubeX, 0xAAAAAA);
            calculateForSurface(window_buffer, w, h, -cubeX, cubeY, cubeWidth, 0x888888);
            calculateForSurface(window_buffer, w, h, cubeX, -cubeWidth, -cubeY, 0x666666);
            calculateForSurface(window_buffer, w, h, cubeX, cubeWidth, cubeY, 0x555555);
        }
    }

    A += 0.05;
    B += 0.05;
    C += 0.01;
}

#pragma GCC pop_options

handle_t ism_ipc;

int main(int argc, char *argv[]) {
    if (ipc_open("ism.window", &ism_ipc) != 0) {
        printf("%s: Unable to open IPC with ISM.\n", get_name());
        return -1;
    }

    /* Create a window */
    wind_t wind;
    struct ism_create_window __attribute__((aligned(0x1000)))
    window = {.name = "Cube",
              .x = 240,
              .y = 40,
              .width = width << PX_SCALE,
              .height = height << PX_SCALE,
              .flags = ISM_WINDOW_FLAG_BORDER | ISM_WINDOW_FLAG_FRAME,
              .parent = ISM_WINDOW_ROOT};
    ipc_send(ism_ipc, ISM_IPC_CREATE_WINDOW, &window, sizeof(window));
    if ((wind = ipc_wait(ism_ipc, 1)) < 0) {
        printf("%s: Unable to create window.\n", get_name());
        return -1;
    }

    size_t draw_size = sizeof(struct ism_draw_window_payload) + window.width * window.height * 4;
    struct ism_draw_window_payload *draw_window =
        (struct ism_draw_window_payload *)align_up((uintptr_t)malloc(draw_size + 0x1000), 0x1000);
    draw_window->wind = wind;
    draw_window->bpp = 32;
    draw_window->width = window.width;
    draw_window->height = window.height;
    draw_window->x = 0;
    draw_window->y = 0;

    time_t start = 0, end = 0, last_print = 0;
    size_t cnt_ftime = 0;
    time_t total_ftime = 0;
    do {
        uptime(&start);
        for (int x = 0; x < window.width; x++)
            for (int y = 0; y < window.height; y++) {
                ((uint32_t *)draw_window->buffer)[y * window.width + x] = 0xff000000;
            }

        render_cube((uint32_t *)draw_window->buffer, window.width, window.height);

        if (ipc_send(ism_ipc, ISM_IPC_DRAW_WINDOW, draw_window, draw_size) != 0 || ipc_wait(ism_ipc, 1) != 0) {
            printf("%s: Unable to render the window.\n", get_name());
        }

        uptime(&end);
        usleep(16000 - ((end - start) > 16000 ? 0 : (end - start)));

        total_ftime += (end - start);
        cnt_ftime++;

        if (last_print < end) {
            last_print = end + 1000000;
            printf("%s: fps %llu (avg. frame time %llu)\n", get_name(), 1000000 / (total_ftime / cnt_ftime),  total_ftime / cnt_ftime);
        }
    } while (1);

    return 0;
}
