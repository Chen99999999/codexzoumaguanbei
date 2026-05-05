#include "smartcar_display.hpp"
#include "zf_device_ips200_fb.hpp"

#include <cstdio>

static zf_device_ips200 g_ips200;
static bool g_display_inited = false;

static const int LINE_X = 0;
static const int LINE_Y0 = 16;
static const int LINE_GAP = 24;
static const int LINE_TEXT_CHARS = 32;

static int row_to_y(int row)
{
    if (row < 0) row = 0;
    return LINE_Y0 + row * LINE_GAP;
}

void display_init(void)
{
    if (g_display_inited)
    {
        return;
    }

    g_ips200.init(FB_PATH, 0);
    g_ips200.full(RGB565_BLACK);

    g_display_inited = true;

    g_ips200.show_string(0, 16, "DISPLAY INIT OK");
    printf("[display_init] ips200 ok, FB_PATH=%s\n", FB_PATH);
}

void display_clear(void)
{
    if (!g_display_inited)
    {
        display_init();
    }

    g_ips200.full(RGB565_BLACK);
}

void display_title(const char *title)
{
    if (!g_display_inited)
    {
        display_init();
    }

    g_ips200.show_string(LINE_X, row_to_y(0), title);
}

void display_line(int row, const char *text)
{
    if (!g_display_inited)
    {
        display_init();
    }

    char buf[LINE_TEXT_CHARS + 1];
    snprintf(buf, sizeof(buf), "%-*.*s", LINE_TEXT_CHARS, LINE_TEXT_CHARS, text ? text : "");
    g_ips200.show_string(LINE_X, row_to_y(row), buf);
}

void display_value(int row, const char *name, int value)
{
    if (!g_display_inited)
    {
        display_init();
    }

    char text[64];
    snprintf(text, sizeof(text), "%s=%d", name, value);
    char buf[LINE_TEXT_CHARS + 1];
    snprintf(buf, sizeof(buf), "%-*.*s", LINE_TEXT_CHARS, LINE_TEXT_CHARS, text);
    g_ips200.show_string(LINE_X, row_to_y(row), buf);
}

void display_two_values(int row, const char *name1, int value1, const char *name2, int value2)
{
    if (!g_display_inited)
    {
        display_init();
    }

    char text[96];
    snprintf(text, sizeof(text), "%s=%d %s=%d", name1, value1, name2, value2);
    char buf[LINE_TEXT_CHARS + 1];
    snprintf(buf, sizeof(buf), "%-*.*s", LINE_TEXT_CHARS, LINE_TEXT_CHARS, text);
    g_ips200.show_string(LINE_X, row_to_y(row), buf);
}

void display_motor_pwm(int left_percent, int right_percent)
{
    display_two_values(2, "L", left_percent, "R", right_percent);
}

void display_status(const char *status)
{
    display_line(1, status);
}


void display_show_gray_image(const uint8 *image, int width, int height)
{
    if (!g_display_inited)
    {
        display_init();
    }

    if (image == nullptr)
    {
        g_ips200.full(RGB565_BLACK);
        g_ips200.show_string(0, 16, "GRAY IMAGE NULL");
        return;
    }

    // 官方接口原尺寸显示，所以画面会在左上角，不强制放大。
    g_ips200.displayimage_gray(image, width, height);
}
