#include "smartcar_camera.hpp"
#include "zf_device_uvc.hpp"

#include <cstdio>

static zf_device_uvc g_uvc;
static bool g_camera_inited = false;
static bool g_last_frame_ok = false;

static uint8* g_gray_image = nullptr;
static uint16* g_rgb565_image = nullptr;

bool camera_init(void)
{
    if (g_camera_inited)
    {
        return true;
    }

    printf("[camera_init] UVC_PATH=%s\n", UVC_PATH);
    printf("[camera_init] size=%dx%d\n", UVC_WIDTH, UVC_HEIGHT);

    if (g_uvc.init(UVC_PATH) < 0)
    {
        printf("[camera_init] failed\n");
        g_camera_inited = false;
        return false;
    }

    g_camera_inited = true;
    printf("[camera_init] ok\n");

    return true;
}

bool camera_update(void)
{
    if (!g_camera_inited)
    {
        if (!camera_init())
        {
            return false;
        }
    }

    if (g_uvc.wait_image_refresh() < 0)
    {
        printf("[camera_update] wait_image_refresh failed\n");
        g_last_frame_ok = false;
        return false;
    }

    g_gray_image = g_uvc.get_gray_image_ptr();
    g_rgb565_image = g_uvc.get_rgb_image_ptr();

    if (g_gray_image == nullptr)
    {
        printf("[camera_update] gray image null\n");
        g_last_frame_ok = false;
        return false;
    }

    g_last_frame_ok = true;
    return true;
}

uint8* camera_get_gray(void)
{
    return g_gray_image;
}

uint16* camera_get_rgb565(void)
{
    return g_rgb565_image;
}

int camera_get_width(void)
{
    return UVC_WIDTH;
}

int camera_get_height(void)
{
    return UVC_HEIGHT;
}

void camera_print_status(void)
{
    printf("[camera] inited=%d frame_ok=%d size=%dx%d gray=%p rgb=%p\n",
           g_camera_inited,
           g_last_frame_ok,
           UVC_WIDTH,
           UVC_HEIGHT,
           g_gray_image,
           g_rgb565_image);
}


// ===== camera save gray pgm begin =====
#include <cstdio>

bool camera_save_gray_pgm(const char *path)
{
    if (path == nullptr)
    {
        printf("[camera_save_gray_pgm] path null\n");
        return false;
    }

    if (g_gray_image == nullptr)
    {
        printf("[camera_save_gray_pgm] gray image null\n");
        return false;
    }

    FILE *fp = fopen(path, "wb");
    if (fp == nullptr)
    {
        printf("[camera_save_gray_pgm] open failed: %s\n", path);
        return false;
    }

    fprintf(fp, "P5\n%d %d\n255\n", UVC_WIDTH, UVC_HEIGHT);
    size_t written = fwrite(g_gray_image, 1, UVC_WIDTH * UVC_HEIGHT, fp);
    fclose(fp);

    if (written != (size_t)(UVC_WIDTH * UVC_HEIGHT))
    {
        printf("[camera_save_gray_pgm] write incomplete: %s written=%zu\n", path, written);
        return false;
    }

    printf("[camera_save_gray_pgm] saved: %s\n", path);
    return true;
}
// ===== camera save gray pgm end =====

