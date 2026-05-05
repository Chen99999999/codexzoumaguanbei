#include "zf_common_headfile.h"
#include "Init.hpp"
#include "motor.hpp"

#include <cstdio>
#include <cstdlib>
#include <csignal>

#define TEST_PWM        20
#define STEP_TIME_MS    2200
#define STOP_TIME_MS    800

void cleanup()
{
    printf("[cleanup] motor_stop\n");
    motor_stop();
}

static void signal_handler(int signum)
{
    printf("[signal] %d, motor_stop and exit\n", signum);
    motor_stop();
    std::exit(0);
}

static void screen_show_step(const char *name, int left_pwm, int right_pwm)
{
    char line1[64];
    char line2[64];

    snprintf(line1, sizeof(line1), "%s", name);
    snprintf(line2, sizeof(line2), "L=%d R=%d", left_pwm, right_pwm);

    ips200_full(RGB565_BLACK);
    ips200_show_string(10, 20, "MOTOR OPENLOOP");
    ips200_show_string(10, 60, line1);
    ips200_show_string(10, 90, line2);
}

static void run_step(const char *name, int left_pwm, int right_pwm, int ms)
{
    printf("\n========== %s ==========\n", name);
    printf("left_pwm=%d right_pwm=%d\n", left_pwm, right_pwm);

    screen_show_step(name, left_pwm, right_pwm);

    motor_set_pwm(left_pwm, right_pwm);
    system_delay_ms(ms);

    motor_stop();
    system_delay_ms(STOP_TIME_MS);
}

int main()
{
    std::atexit(cleanup);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    printf("========== SMARTCAR MOTOR OPENLOOP TEST ==========\n");
    printf("MOTOR1_DIR = %s\n", MOTOR1_DIR);
    printf("MOTOR1_PWM = %s\n", MOTOR1_PWM);
    printf("MOTOR2_DIR = %s\n", MOTOR2_DIR);
    printf("MOTOR2_PWM = %s\n", MOTOR2_PWM);

    Init_all();

    ips200_init("/dev/fb0");
    ips200_full(RGB565_BLACK);
    ips200_show_string(10, 20, "MOTOR INIT");

    Motor_Argument();
    motor_init();
    motor_stop();

    printf("\n注意：先悬空测试，不要让车直接落地跑。\n");
    ips200_show_string(10, 60, "WHEELS UP");

    system_delay_ms(2000);

    while (1)
    {
        run_step("STOP", 0, 0, 1200);

        run_step("LEFT +", TEST_PWM, 0, STEP_TIME_MS);
        run_step("LEFT -", -TEST_PWM, 0, STEP_TIME_MS);

        run_step("RIGHT +", 0, TEST_PWM, STEP_TIME_MS);
        run_step("RIGHT -", 0, -TEST_PWM, STEP_TIME_MS);

        run_step("BOTH +", TEST_PWM, TEST_PWM, STEP_TIME_MS);
        run_step("BOTH -", -TEST_PWM, -TEST_PWM, STEP_TIME_MS);

        run_step("TURN L", -TEST_PWM, TEST_PWM, STEP_TIME_MS);
        run_step("TURN R", TEST_PWM, -TEST_PWM, STEP_TIME_MS);
    }

    return 0;
}
