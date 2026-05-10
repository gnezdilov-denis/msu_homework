//
//  main.c
//  мгу
//
//  Created by Гнездилов Денис  on 21.09.2025.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lodepng.h"

unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height)
{
    unsigned char* image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error != 0) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    return image;
}

void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
    unsigned char* png;
    long unsigned int pngsize;
    int error = lodepng_encode32(&png, &pngsize, image, width, height);
    if (error == 0) {
        lodepng_save_file(png, pngsize, filename);
    } else {
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    free(png);
}

void to_gray(unsigned char* picture, unsigned char* gray, int bw_size)
{
    for (int i = 0; i < bw_size; i++) {
        gray[i] = (unsigned char)(
            (77  * (int)picture[i*4+0] +150 * (int)picture[i*4+1] + 29  * (int)picture[i*4+2]) >> 8);
    }
}

void median_filter(unsigned char* gray, float* out, int width, int height, int R)
{
    int win = (2*R+1)*(2*R+1);
    unsigned char* buf = (unsigned char*)malloc(win * sizeof(unsigned char));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int n = 0;
            for (int dy = -R; dy <= R; dy++) {
                for (int dx = -R; dx <= R; dx++) {
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        buf[n++] = gray[ny*width + nx];
                    }
                }
            }
            for (int i = 1; i < n; i++) {
                unsigned char key = buf[i];
                int j = i - 1;
                while (j >= 0 && buf[j] > key) {
                    buf[j+1] = buf[j];
                    j--;
                }
                buf[j+1] = key;
            }
            out[y*width + x] = (float)buf[n/2];
        }
    }
    free(buf);
}

void box_mean(unsigned char* gray, float* bg, int width, int height, int R)
{
    int n = width * height;
    double* hsum = (double*)calloc(n, sizeof(double));
    double* vsum = (double*)calloc(n, sizeof(double));

    for (int y = 0; y < height; y++) {
        double* pref = (double*)calloc(width + 1, sizeof(double));
        for (int x = 0; x < width; x++) {
            pref[x+1] = pref[x] + gray[y*width + x];
        }
        for (int x = 0; x < width; x++) {
            int x0 = x - R;
            int x1 = x + R;
            if (x0 < 0) {
                x0 = 0;
            }
            if (x1 >= width) {
                x1 = width - 1;
            }
            hsum[y*width + x] = (pref[x1+1] - pref[x0]) / (double)(x1 - x0 + 1);
        }
        free(pref);
    }
    for (int x = 0; x < width; x++) {
        double* pref = (double*)calloc(height + 1, sizeof(double));
        for (int y = 0; y < height; y++) {
            pref[y+1] = pref[y] + hsum[y*width + x];
        }
        for (int y = 0; y < height; y++) {
            int y0 = y - R;
            int y1 = y + R;
            if (y0 < 0) {
                y0 = 0;
            }
            if (y1 >= height) {
                y1 = height - 1;
            }
            bg[y*width + x] = (float)((pref[y1+1] - pref[y0]) / (double)(y1 - y0 + 1));
        }
        free(pref);
    }
    free(hsum);
    free(vsum);
}

void local_maximum(unsigned char* gray, unsigned char* is_max, int width, int height, int R)
{
    int bw_size = width * height;
    for (int i = 0; i < bw_size; i++) {
        is_max[i] = 1;
    }
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            unsigned char v = gray[i];
            for (int dy = -R; dy <= R && is_max[i]; dy++) {
                for (int dx = -R; dx <= R && is_max[i]; dx++) {
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < height && nx >= 0 && nx < width) {
                        if (gray[ny*width + nx] > v) {
                            is_max[i] = 0;
                        }
                    }
                }
            }
        }
    }
}

int dfs(unsigned char* territory, unsigned char* visited,
        int start, int bw_size, int width)
{
    int* stack = (int*)malloc((size_t)bw_size * 8 * sizeof(int));
    int top = 0, size = 0;

    stack[top++] = start;
    visited[start] = 1;

    int offsets[8];
    offsets[0] =  1; offsets[1] = -1;
    offsets[2] =  width; offsets[3] = -width;
    offsets[4] =  width+1; offsets[5] =  width-1;
    offsets[6] = -width+1; offsets[7] = -width-1;

    while (top > 0) {
        int cur = stack[--top];
        size++;
        for (int d = 0; d < 8; d++) {
            int nb = cur + offsets[d];
            if (nb >= 0 && nb < bw_size && !visited[nb] && territory[nb]) {
                visited[nb]  = 1;
                stack[top++] = nb;
            }
        }
    }
    free(stack);
    return size;
}

void draw_mark(unsigned char* vis, int width, int height, int cx, int cy)
{
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = cx + dx, ny = cy + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int idx = ny * width + nx;
                vis[idx*4+0] = 255;
                vis[idx*4+1] = 0;
                vis[idx*4+2] = 0;
                vis[idx*4+3] = 255;
            }
        }
    }
}

int main(void)
{
    const char* filename = "scene.png";

    float CONTRAST_MIN = 60.0f;
    float BG_LARGE_MAX = 40.0f;
    int   MAX_SHIP_PX  = 3;

    unsigned int width = 0, height = 0;
    unsigned char* picture = load_png(filename, &width, &height);

    int ok = (picture != NULL);
    if (!ok) {
        printf("Problem reading picture from the file %s\n", filename);
    }

    int W = 0, H = 0;
    if (ok) {
        W = (int)width;
        H = (int)height;
    }
    int bw_size = W * H;

    unsigned char* gray = NULL;
    float* bg_median = NULL;
    float* bg_large  = NULL;
    unsigned char* is_max = NULL;
    unsigned char* territory = NULL;
    unsigned char* visited = NULL;
    unsigned char* vis = NULL;

    if (ok) {
        gray = (unsigned char*)calloc(bw_size, sizeof(unsigned char));
        bg_median = (float*) calloc(bw_size, sizeof(float));
        bg_large = (float*) calloc(bw_size, sizeof(float));
        is_max = (unsigned char*)calloc(bw_size, sizeof(unsigned char));
        territory = (unsigned char*)calloc(bw_size, sizeof(unsigned char));
        visited = (unsigned char*)calloc(bw_size, sizeof(unsigned char));
        vis = (unsigned char*)calloc(bw_size, 4 * sizeof(unsigned char));

        to_gray(picture, gray, bw_size);

        printf("Computing median filter...\n");
        median_filter(gray, bg_median, W, H, 10);

        printf("Computing large background...\n");
        box_mean(gray, bg_large, W, H, 75);

        printf("Finding local maxima...\n");
        local_maximum(gray, is_max, W, H, 4);

        for (int i = 0; i < bw_size; i++) {
            float contrast = (float)gray[i] - bg_median[i];
            int is_tanker  = is_max[i] && (contrast > CONTRAST_MIN) && (bg_large[i] < BG_LARGE_MAX);
            if (is_tanker) {
                territory[i] = 255;
            } else {
                territory[i] = 0;
            }
        }

        for (int i = 0; i < bw_size; i++) {
            vis[i*4+0] = picture[i*4+0];
            vis[i*4+1] = picture[i*4+1];
            vis[i*4+2] = picture[i*4+2];
            vis[i*4+3] = 255;
        }

        int counter = 0;
        for (int i = 0; i < bw_size; i++) {
            if (!visited[i] && territory[i]) {
                int size = dfs(territory, visited, i, bw_size, W);
                if (size <= MAX_SHIP_PX) {
                    draw_mark(vis, W, H, i % W, i / W);
                    counter++;
                }
            }
        }

        printf("Tankers detected: %d\n", counter);
        write_png("detections.png", vis, width, height);
    }

    free(gray);
    free(bg_median);
    free(bg_large);
    free(is_max);
    free(territory);
    free(visited);
    free(vis);
    free(picture);
    return 0;
}
