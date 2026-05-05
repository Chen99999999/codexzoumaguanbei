#include "motor.hpp"
#include <cstdio>
#include <cstdint>

// ============================================================
// motor.cpp - 官方逐飞 DRV8701 桥接版
// 当前 motor_set_pwm 参数单位：百分比，范围 -30 ~ +30
// ============================================================

extern "C" void official_motor_init_bridge(uint32_t *max1, uint32_t *max2);
extern "C" void official_motor_stop_bridge(void);
extern "C" void official_motor_set_percent_bridge(int left_percent, int right_percent);

int Speed_Goal_l = 0;
int Speed_Goal_r = 0;

int16 Diff_SpeedL_expect = 0;
int16 Diff_SpeedR_expect = 0;

int Speed_PID_OUT_l = 0;
int Speed_PID_OUT_r = 0;

int PWM_Max = 30;
static int PWM_Min = -30;

static bool g_motor_inited = false;
static uint32_t g_motor1_duty_max = 0;
static uint32_t g_motor2_duty_max = 0;

static int clamp_int(int x, int low, int high)
{
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

void motor_init(void)
{
    official_motor_init_bridge(&g_motor1_duty_max, &g_motor2_duty_max);
    g_motor_inited = true;

    Speed_Goal_l = 0;
    Speed_Goal_r = 0;
    Diff_SpeedL_expect = 0;
    Diff_SpeedR_expect = 0;
    Speed_PID_OUT_l = 0;
    Speed_PID_OUT_r = 0;

    printf("[motor_init official bridge] M1 duty_max=%u, M2 duty_max=%u, PWM_Max=%d%%\n",
           g_motor1_duty_max, g_motor2_duty_max, PWM_Max);
}

void motor_stop(void)
{
    official_motor_stop_bridge();

    Speed_Goal_l = 0;
    Speed_Goal_r = 0;
    Diff_SpeedL_expect = 0;
    Diff_SpeedR_expect = 0;
    Speed_PID_OUT_l = 0;
    Speed_PID_OUT_r = 0;
}

void motor_set_pwm(int left_pwm, int right_pwm)
{
    if (!g_motor_inited)
    {
        motor_init();
    }

    left_pwm = clamp_int(left_pwm, PWM_Min, PWM_Max);
    right_pwm = clamp_int(right_pwm, PWM_Min, PWM_Max);

    Speed_Goal_l = left_pwm;
    Speed_Goal_r = right_pwm;

    official_motor_set_percent_bridge(left_pwm, right_pwm);

    printf("[motor_set_pwm official] left=%d%% right=%d%%\n", left_pwm, right_pwm);
}

void motor_set_speed(int left_speed, int right_speed)
{
    motor_set_pwm(left_speed, right_speed);
}

void Motor_Argument(void)
{
    Speed_Goal_l = 0;
    Speed_Goal_r = 0;
    Diff_SpeedL_expect = 0;
    Diff_SpeedR_expect = 0;
    Speed_PID_OUT_l = 0;
    Speed_PID_OUT_r = 0;
}

void Motor_Init1(void)
{
    motor_init();
}

void Motor_Control(void)
{
    motor_set_pwm(Speed_Goal_l, Speed_Goal_r);
}

void Motor_PID_Left(void)
{
}

void Motor_PID_Right(void)
{
}

void Motor_Diff_Pid1(void)
{
    Diff_SpeedL_expect = Speed_Goal_l;
    Diff_SpeedR_expect = Speed_Goal_r;
}

void Encoder_Test1(void)
{
    printf("[Encoder_Test1] official bridge version: encoder not used yet\n");
}
