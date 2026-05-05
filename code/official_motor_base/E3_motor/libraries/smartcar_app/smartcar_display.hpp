#pragma once

#include "zf_common_headfile.hpp"

// 屏幕封装接口：上层只调用 smartcar_display，不直接碰逐飞底层对象

void display_init(void);
void display_clear(void);

void display_title(const char *title);
void display_line(int row, const char *text);
void display_value(int row, const char *name, int value);
void display_two_values(int row, const char *name1, int value1, const char *name2, int value2);

void display_motor_pwm(int left_percent, int right_percent);
void display_status(const char *status);

void display_show_gray_image(const uint8 *image, int width, int height);
