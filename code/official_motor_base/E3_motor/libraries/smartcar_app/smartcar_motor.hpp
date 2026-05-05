#pragma once

#include "zf_common_headfile.hpp"

// ============================================================
// smartcar_motor.hpp
// DRV8701 双电机控制封装
//
// motor_set_pwm:
//   单位：整数百分比
//   例如 5 = 5%
//
// motor_set_pwm_x100:
//   单位：百分比 * 100
//   例如 550 = 5.50%
//        1234 = 12.34%
// ============================================================

void motor_init(void);
void motor_stop(void);

// 兼容旧接口：整数百分比
void motor_set_pwm(int left_percent, int right_percent);

// 新接口：百分比 * 100，闭环优先用这个
void motor_set_pwm_x100(int left_percent_x100, int right_percent_x100);

uint32 motor_get_left_duty_max(void);
uint32 motor_get_right_duty_max(void);
