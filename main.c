#include <Windows.h>
#include <emmintrin.h>
#include <smmintrin.h>

int _fltused;

BOOL g_is_open = FALSE;
LONG g_width = 640;
LONG g_height = 480;
void* g_pixels = NULL;
BITMAPINFO bmp_info;
UINT64 avg = 0U;
UINT64 avg_fs = 0U;
UINT64 avg_simd = 0U;
UINT64 frame_count = 0U;
__m128i g_zero;
__m128i g_one;

typedef struct aos_tri {
    UINT32 p1[2];
    UINT32 p2[2];
    UINT32 p3[2];
} aos_tri;

typedef struct soa_tri {
    UINT32 x[3];
    UINT32 y[3];
} soa_tri;

aos_tri aos_triangles[] = {
    {
        .p1 = { 128, 384 },
        .p2 = { 320, 96 },
        .p3 = { 512, 384 },
    },
};
soa_tri soa_triangles[] = {
    {
        .x = { 128, 320, 512 },
        .y = { 384, 96, 384 },
    },
};

void RenderFullscren(void) {
    DWORD64 start = __rdtsc();

    UINT8* pixels = (UINT8*)g_pixels;
    aos_tri t = aos_triangles[0];
    UINT32 max_x = max(t.p1[0], max(t.p2[0], t.p3[0]));
    UINT32 min_x = min(t.p1[0], min(t.p2[0], t.p3[0]));
    UINT32 max_y = max(t.p1[1], max(t.p2[1], t.p3[1]));
    UINT32 min_y = min(t.p1[1], min(t.p2[1], t.p3[1]));

    INT32 v1[2] = { t.p2[0] - t.p1[0], t.p2[1] - t.p1[1] };
    INT32 v2[2] = { t.p3[0] - t.p2[0], t.p3[1] - t.p2[1] };
    INT32 v3[2] = { t.p1[0] - t.p3[0], t.p1[1] - t.p3[1] };

    for (INT32 i = 0; i < g_width * g_height; i++) {
        UINT32 x = i % g_width;
        UINT32 y = i / g_width;
        if (min_x <= x && x <= max_x &&
            min_y <= y && y <= max_y) {

            INT32 t1[2] = { x - t.p1[0], y - t.p1[1] };
            INT32 t2[2] = { x - t.p2[0], y - t.p2[1] };
            INT32 t3[2] = { x - t.p3[0], y - t.p3[1] };

            INT32 d_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            INT32 d_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            INT32 d_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            if (d_t1 > 0.f && d_t2 > 0.f && d_t3 > 0.f) {
                pixels[i * 4] = 255;
            }
        }
    }

    DWORD64 end = __rdtsc();
    DWORD64 diff = end - start;

    avg_fs = ((avg_fs * frame_count) + diff) / (frame_count + 1);

    char buffer[1024];
    wsprintfA(buffer, "fullscreen scan %u cycles per triangle\n", diff);
    OutputDebugStringA(buffer);
    wsprintfA(buffer, "fullscreen scan %u avg cycles per triangle\n", avg_fs);
    OutputDebugStringA(buffer);
}

void RenderTriangle(void) {
    DWORD64 start = __rdtsc();

    UINT8* pixels = (UINT8*)g_pixels;
    aos_tri t = aos_triangles[0];
    UINT32 max_x = max(t.p1[0], max(t.p2[0], t.p3[0]));
    UINT32 min_x = min(t.p1[0], min(t.p2[0], t.p3[0]));
    UINT32 max_y = max(t.p1[1], max(t.p2[1], t.p3[1]));
    UINT32 min_y = min(t.p1[1], min(t.p2[1], t.p3[1]));

    INT32 v1[2] = { t.p2[0] - t.p1[0], t.p2[1] - t.p1[1] };
    INT32 v2[2] = { t.p3[0] - t.p2[0], t.p3[1] - t.p2[1] };
    INT32 v3[2] = { t.p1[0] - t.p3[0], t.p1[1] - t.p3[1] };

    for (UINT32 y = min_y; y < max_y + 1; y++) {
        for (UINT32 x = min_x; x < max_x + 1; x++) {

            INT32 t1[2] = { x - t.p1[0], y - t.p1[1] };
            INT32 t2[2] = { x - t.p2[0], y - t.p2[1] };
            INT32 t3[2] = { x - t.p3[0], y - t.p3[1] };

            INT32 d_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            INT32 d_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            INT32 d_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            if (d_t1 > 0.f && d_t2 > 0.f && d_t3 > 0.f) {
                pixels[((y * g_width) + x) * 4 + 1] = 255;
            }
        }
    }

    DWORD64 end = __rdtsc();
    DWORD64 diff = end - start;

    // avg = (avg + diff) / 2;
    avg = ((avg * frame_count) + diff) / (frame_count + 1);

    char buffer[1024];
    wsprintfA(buffer, "%u cycles per triangle\n", diff);
    OutputDebugStringA(buffer);
    wsprintfA(buffer, "%u avg cycles per triangle\n", avg);
    OutputDebugStringA(buffer);
}

void RenderTriangleSIMD(void) {
    DWORD64 start = __rdtsc();

    UINT64* pixels = (UINT64*)g_pixels;
    soa_tri t = soa_triangles[0];
    UINT32 max_x = max(t.x[0], max(t.x[1], t.x[2]));
    UINT32 min_x = min(t.x[0], min(t.x[1], t.x[2]));
    UINT32 max_y = max(t.y[0], max(t.y[1], t.y[2]));
    UINT32 min_y = min(t.y[0], min(t.y[1], t.y[2]));

    max_x += max_x % 2;
    min_x -= min_x % 2;
    max_y += max_y % 2;
    min_y -= min_y % 2;

    __m128i p1_x = _mm_set1_epi32(t.x[0]);
    __m128i p1_y = _mm_set1_epi32(t.y[0]);
    __m128i p2_x = _mm_set1_epi32(t.x[1]);
    __m128i p2_y = _mm_set1_epi32(t.y[1]);
    __m128i p3_x = _mm_set1_epi32(t.x[2]);
    __m128i p3_y = _mm_set1_epi32(t.y[2]);

    __m128i v1_x = _mm_set1_epi32(t.x[1] - t.x[0]);
    __m128i v1_y = _mm_set1_epi32(t.y[1] - t.y[0]);
    __m128i v2_x = _mm_set1_epi32(t.x[2] - t.x[1]);
    __m128i v2_y = _mm_set1_epi32(t.y[2] - t.y[1]);
    __m128i v3_x = _mm_set1_epi32(t.x[0] - t.x[2]);
    __m128i v3_y = _mm_set1_epi32(t.y[0] - t.y[2]);

    // min and max must be multiples of 2
    for (UINT32 y = min_y; y < max_y + 1; y+=2) {
        UINT32 row_1 = y * g_width;
        UINT32 row_2 = (y + 1) * g_width;
        for (UINT32 x = min_x; x < max_x + 1; x+=2) {
            // work on 2x2 
            // _________
            // |   |   |
            // | 1 | 2 |
            // |___|___|
            // |   |   |
            // | 3 | 4 |
            // |___|___|
            // 1
            // {
            //     INT32 t1[2] = { x - tt.p1[0], y - tt.p1[1] };
            //     INT32 t2[2] = { x - tt.p2[0], y - tt.p2[1] };
            //     INT32 t3[2] = { x - tt.p3[0], y - tt.p3[1] };

            //     INT32 dt_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            //     INT32 dt_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            //     INT32 dt_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            //     if (dt_t1 > 0.f && dt_t2 > 0.f && dt_t3 > 0.f) {
            //         pixels[((y * g_width) + x) * 4 + 2] = 255;
            //     }
            // }
            // // 2
            // {
            //     INT32 t1[2] = { x + 1 - t.p1[0], y - t.p1[1] };
            //     INT32 t2[2] = { x + 1 - t.p2[0], y - t.p2[1] };
            //     INT32 t3[2] = { x + 1 - t.p3[0], y - t.p3[1] };

            //     INT32 d_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            //     INT32 d_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            //     INT32 d_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            //     if (d_t1 > 0.f && d_t2 > 0.f && d_t3 > 0.f) {
            //         pixels[((y * g_width) + x + 1) * 4 + 2] = 255;
            //     }
            // }
            // // 3
            // {
            //     INT32 t1[2] = { x - t.p1[0], y + 1 - t.p1[1] };
            //     INT32 t2[2] = { x - t.p2[0], y + 1 - t.p2[1] };
            //     INT32 t3[2] = { x - t.p3[0], y + 1 - t.p3[1] };

            //     INT32 d_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            //     INT32 d_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            //     INT32 d_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            //     if (d_t1 > 0.f && d_t2 > 0.f && d_t3 > 0.f) {
            //         pixels[(((y + 1) * g_width) + x) * 4 + 2] = 255;
            //     }
            // }
            // // 4
            // {
            //     INT32 t1[2] = { x + 1 - t.p1[0], y + 1 - t.p1[1] };
            //     INT32 t2[2] = { x + 1 - t.p2[0], y + 1 - t.p2[1] };
            //     INT32 t3[2] = { x + 1 - t.p3[0], y + 1 - t.p3[1] };

            //     INT32 d_t1 = v1[0] * t1[1] - t1[0] * v1[1];
            //     INT32 d_t2 = v2[0] * t2[1] - t2[0] * v2[1];
            //     INT32 d_t3 = v3[0] * t3[1] - t3[0] * v3[1];

            //     if (d_t1 > 0.f && d_t2 > 0.f && d_t3 > 0.f) {
            //         pixels[(((y + 1) * g_width) + x + 1) * 4 + 2] = 255;
            //     }
            // }

            __m128i vx = _mm_set_epi32(x + 1, x, x + 1, x);
            __m128i vy = _mm_set_epi32(y + 1, y + 1, y, y);

            __m128i p_t1x = _mm_sub_epi32(vx, p1_x);
            __m128i p_t1y = _mm_sub_epi32(vy, p1_y);
            __m128i p_t2x = _mm_sub_epi32(vx, p2_x);
            __m128i p_t2y = _mm_sub_epi32(vy, p2_y);
            __m128i p_t3x = _mm_sub_epi32(vx, p3_x);
            __m128i p_t3y = _mm_sub_epi32(vy, p3_y);

            __m128i d_t1_1 = _mm_mullo_epi32(v1_x, p_t1y);
            __m128i d_t1_2 = _mm_mullo_epi32(v1_y, p_t1x);
            __m128i d_t2_1 = _mm_mullo_epi32(v2_x, p_t2y);
            __m128i d_t2_2 = _mm_mullo_epi32(v2_y, p_t2x);
            __m128i d_t3_1 = _mm_mullo_epi32(v3_x, p_t3y);
            __m128i d_t3_2 = _mm_mullo_epi32(v3_y, p_t3x);

            __m128i d_t1 = _mm_sub_epi32(d_t1_1, d_t1_2);
            __m128i d_t2 = _mm_sub_epi32(d_t2_1, d_t2_2);
            __m128i d_t3 = _mm_sub_epi32(d_t3_1, d_t3_2);

            __m128i t1_gtz = _mm_cmpgt_epi32(d_t1, g_zero);
            __m128i t2_gtz = _mm_cmpgt_epi32(d_t2, g_zero);
            __m128i t1_and_t2 = _mm_and_si128(t1_gtz, t2_gtz);
            __m128i t3_gtz = _mm_cmpgt_epi32(d_t3, g_zero);

            __m128i all_gtz = _mm_and_si128(t1_and_t2, t3_gtz);

            pixels[(row_1 + x) / 2] = _mm_extract_epi64(all_gtz, 0);
            pixels[(row_2 + x) / 2] = _mm_extract_epi64(all_gtz, 1);
        }
    }

    DWORD64 end = __rdtsc();
    DWORD64 diff = end - start;

    // avg_simd = (avg_simd + diff) / 2;
    avg_simd = ((avg_simd * frame_count) + diff) / (frame_count + 1);

    char buffer[1024];
    wsprintfA(buffer, "SIMD %u cycles per triangle\n", diff);
    OutputDebugStringA(buffer);
    wsprintfA(buffer, "SIMD %u avg cycles per triangle\n", avg_simd);
    OutputDebugStringA(buffer);
}

void ResizeBackBuffer() {
    if (g_pixels) {
        VirtualFree(g_pixels, 0, MEM_RELEASE);
    }

    bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmp_info.bmiHeader.biWidth = g_width;
    bmp_info.bmiHeader.biHeight = -g_height;
    bmp_info.bmiHeader.biPlanes = 1;
    bmp_info.bmiHeader.biBitCount = 32;
    bmp_info.bmiHeader.biCompression = BI_RGB;

    g_pixels = VirtualAlloc(NULL, g_width * g_height * 4, MEM_COMMIT, PAGE_READWRITE);
}

LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch(msg) {
        case WM_CREATE: {
            g_is_open = TRUE;

            RECT rect;
            GetWindowRect(window, &rect);

            g_width = rect.right - rect.left;
            g_height = rect.bottom - rect.top;

            ResizeBackBuffer();
            break;
        }
        case WM_SIZE: {
            g_width = LOWORD(lparam);
            g_height = HIWORD(lparam);

            ResizeBackBuffer();
            break;
        }
        case WM_CLOSE: {
            g_is_open = FALSE;
            PostQuitMessage(0);
            break;
        }
        case WM_DESTROY: {
            DestroyWindow(window);
            break;
        }
        case WM_PAINT: {
            RenderFullscren();
            RenderTriangle();
            RenderTriangleSIMD();
            frame_count++;

            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(window, &paint);

            StretchDIBits(
                hdc,
                0, 0, g_width, g_height,
                0, 0, g_width, g_height,
                g_pixels,
                &bmp_info,
                DIB_RGB_COLORS,
                SRCCOPY
            );

            EndPaint(window, &paint);
            break;
        }
        default:
            break;
    }

    return DefWindowProcA(window, msg, wparam, lparam);
}

int mainCRTStartup(void) {
    const char* class_name = "sftw_rst_class";
    WNDCLASS wnd_class = {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = &WndProc,
        .hInstance = NULL,
        .lpszClassName = class_name,
    };
    RegisterClass(&wnd_class);

    HWND wnd = CreateWindowA(class_name, "Software Rasterizer", WS_OVERLAPPEDWINDOW, 10, 10, g_width, g_height, NULL, NULL, NULL, NULL);

    ShowWindow(wnd, SW_SHOW);

    g_zero = _mm_set1_epi32(-1);
    g_one = _mm_set1_epi32(255);

    while(g_is_open) {
        MSG msg;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderFullscren();
        RenderTriangle();
        RenderTriangleSIMD();
        frame_count++;
    }

    return 0;
}
