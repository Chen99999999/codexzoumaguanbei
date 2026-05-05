#pragma once

#include "zf_common_headfile.h"
#include "Servo.hpp"

// 逐飞新版内核已验证设备节点
#define MOTOR1_DIR   "/dev/zf_gpio_motor_1"
#define MOTOR1_PWM   "/dev/zf_pwm_motor_1"

#define MOTOR2_DIR   "/dev/zf_gpio_motor_2"
#define MOTOR2_PWM   "/dev/zf_pwm_motor_2"

// 旧工程兼容变量
extern int Speed_Goal_l;
extern int Speed_Goal_r;

extern int16 Diff_SpeedL_expect;
extern int16 Diff_SpeedR_expect;

extern int Speed_PID_OUT_l;
extern int Speed_PID_OUT_r;

extern int PWM_Max;

// 当前主线接口：开环 PWM 控制
void motor_init(void);
void motor_stop(void);
void motor_set_pwm(int left_pwm, int right_pwm);

// 后续编码器通了以后再启用
void motor_set_speed(int left_speed, int right_speed);

// 旧工程兼容函数名
void Encoder_Test1(void);
void Motor_Argument(void);
void Motor_Init1(void);
void Motor_PID_Left(void);
void Motor_PID_Right(void);
void Motor_Control(void);
void Motor_Diff_Pid1(void);
