#pragma once

#include "zf_common_headfile.hpp"

// 速度单位：每个采样周期内的编码器计数
// PWM 输出单位：x100，即 550 表示 5.50%

void speed_control_init(int sample_ms);
void speed_control_stop(void);

void speed_control_set_target(int left_count_per_sample, int right_count_per_sample);
void speed_control_update(void);

int speed_control_get_left_target(void);
int speed_control_get_right_target(void);

int speed_control_get_left_measured(void);
int speed_control_get_right_measured(void);

// 兼容旧显示：整数百分比
int speed_control_get_left_pwm(void);
int speed_control_get_right_pwm(void);

// 新显示：百分比 * 100
int speed_control_get_left_pwm_x100(void);
int speed_control_get_right_pwm_x100(void);

void speed_control_print(void);
