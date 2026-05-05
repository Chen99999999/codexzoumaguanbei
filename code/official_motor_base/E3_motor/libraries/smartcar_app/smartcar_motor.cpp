#include "smartcar_motor.hpp"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#define MOTOR1_PWM_PATH     ZF_PWM_MOTOR_1
#define MOTOR1_DIR_PATH     ZF_GPIO_MOTOR_1

#define MOTOR2_PWM_PATH     ZF_PWM_MOTOR_2
#define MOTOR2_DIR_PATH     ZF_GPIO_MOTOR_2

// 最大 30.00%
#define MOTOR_PWM_X100_MAX 3000

// 实测：车辆前进方向对应 DIR = 0
static const int MOTOR1_FORWARD_LEVEL = 0;
static const int MOTOR2_FORWARD_LEVEL = 0;

static zf_driver_gpio motor1_dir(MOTOR1_DIR_PATH, O_RDWR);
static zf_driver_gpio motor2_dir(MOTOR2_DIR_PATH, O_RDWR);

static zf_driver_pwm motor1_pwm(MOTOR1_PWM_PATH);
static zf_driver_pwm motor2_pwm(MOTOR2_PWM_PATH);

static struct pwm_info motor1_pwm_info;
static struct pwm_info motor2_pwm_info;

static bool g_motor_inited = false;

static int clamp_int(int x, int low, int high)
{
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

static int abs_int(int x)
{
    return x >= 0 ? x : -x;
}

static void print_x100(const char *name, int value_x100)
{
    int sign = value_x100 < 0 ? -1 : 1;
    int abs_v = abs_int(value_x100);
    printf("%s=%s%d.%02d%%", name, sign < 0 ? "-" : "", abs_v / 100, abs_v % 100);
}

static void set_one_motor_x100(zf_driver_gpio &dir_dev,
                               zf_driver_pwm &pwm_dev,
                               const struct pwm_info &pwm_info,
                               int forward_level,
                               int percent_x100)
{
    percent_x100 = clamp_int(percent_x100, -MOTOR_PWM_X100_MAX, MOTOR_PWM_X100_MAX);

    if (percent_x100 == 0)
    {
        pwm_dev.set_duty(0);
        return;
    }

    int dir_level = percent_x100 > 0 ? forward_level : (1 - forward_level);
    int duty_x100 = abs_int(percent_x100);

    // percent_x100 = 10000 时表示 100.00%
    uint32 duty = (uint32)((uint64)duty_x100 * pwm_info.duty_max / 10000ULL);

    dir_dev.set_level(dir_level);
    pwm_dev.set_duty(duty);

    // printf("[motor] ");
    print_x100("pwm", percent_x100);
    printf(" dir=%d duty=%u duty_max=%u\n", dir_level, duty, pwm_info.duty_max);
}

void motor_init(void)
{
    if (g_motor_inited)
    {
        return;
    }

    motor1_pwm.get_dev_info(&motor1_pwm_info);
    motor2_pwm.get_dev_info(&motor2_pwm_info);

    g_motor_inited = true;

    motor_stop();

    printf("[motor_init]\n");
    printf("  MOTOR1_DIR=%s\n", MOTOR1_DIR_PATH);
    printf("  MOTOR1_PWM=%s duty_max=%u forward_dir=%d\n",
           MOTOR1_PWM_PATH, motor1_pwm_info.duty_max, MOTOR1_FORWARD_LEVEL);
    printf("  MOTOR2_DIR=%s\n", MOTOR2_DIR_PATH);
    printf("  MOTOR2_PWM=%s duty_max=%u forward_dir=%d\n",
           MOTOR2_PWM_PATH, motor2_pwm_info.duty_max, MOTOR2_FORWARD_LEVEL);
}

void motor_stop(void)
{
    motor1_pwm.set_duty(0);
    motor2_pwm.set_duty(0);
}

void motor_set_pwm(int left_percent, int right_percent)
{
    motor_set_pwm_x100(left_percent * 100, right_percent * 100);
}

void motor_set_pwm_x100(int left_percent_x100, int right_percent_x100)
{
    if (!g_motor_inited)
    {
        motor_init();
    }

    left_percent_x100 = clamp_int(left_percent_x100, -MOTOR_PWM_X100_MAX, MOTOR_PWM_X100_MAX);
    right_percent_x100 = clamp_int(right_percent_x100, -MOTOR_PWM_X100_MAX, MOTOR_PWM_X100_MAX);

    set_one_motor_x100(motor1_dir, motor1_pwm, motor1_pwm_info,
                       MOTOR1_FORWARD_LEVEL, left_percent_x100);

    set_one_motor_x100(motor2_dir, motor2_pwm, motor2_pwm_info,
                       MOTOR2_FORWARD_LEVEL, right_percent_x100);

    // printf("[motor_set_pwm_x100] ");
    print_x100("L", left_percent_x100);
    printf(" ");
    print_x100("R", right_percent_x100);
    printf("\n");
}

uint32 motor_get_left_duty_max(void)
{
    return motor1_pwm_info.duty_max;
}

uint32 motor_get_right_duty_max(void)
{
    return motor2_pwm_info.duty_max;
}
