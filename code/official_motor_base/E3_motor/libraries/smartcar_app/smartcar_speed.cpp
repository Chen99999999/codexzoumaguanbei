#include "smartcar_speed.hpp"

#include "smartcar_motor.hpp"
#include "smartcar_encoder.hpp"

#include <cstdio>

static const float SPEED_KP = 0.002000f;
static const float SPEED_KI = 0.000025f;
static const float SPEED_FF = 0.005500f;

static const int PWM_LIMIT_X100 = 2400;
static const int PWM_SLEW_LIMIT_X100 = 80;
static const float INTEGRAL_LIMIT = 3000.0f;

static const int LEFT_PWM_MIN_RUN_X100 = 480;
static const int RIGHT_PWM_MIN_RUN_X100 = 800;

static const int LEFT_PWM_KICK_X100 = 700;
static const int RIGHT_PWM_KICK_X100 = 1300;

static const int LEFT_KICK_START_FRAMES = 1;
static const int RIGHT_KICK_START_FRAMES = 1;

static const int LEFT_STALL_MEASURED_MAX = 40;
static const int RIGHT_STALL_MEASURED_MAX = 40;

static bool g_speed_inited = false;
static int g_sample_ms = 100;

static int g_left_target = 0;
static int g_right_target = 0;

static int g_left_measured = 0;
static int g_right_measured = 0;

static float g_left_integral = 0.0f;
static float g_right_integral = 0.0f;

static int g_left_pwm_x100 = 0;
static int g_right_pwm_x100 = 0;

static int g_left_kick_frames = 0;
static int g_right_kick_frames = 0;

static int clamp_int(int x, int low, int high)
{
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

static float clamp_float(float x, float low, float high)
{
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

static int sign_int(int x)
{
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

static int abs_int(int x)
{
    return x >= 0 ? x : -x;
}

static int round_float_to_int(float x)
{
    if (x >= 0.0f)
    {
        return (int)(x + 0.5f);
    }

    return (int)(x - 0.5f);
}

static int apply_slew_limit(int target_x100, int last_x100)
{
    int diff = target_x100 - last_x100;

    if (diff > PWM_SLEW_LIMIT_X100)
    {
        return last_x100 + PWM_SLEW_LIMIT_X100;
    }

    if (diff < -PWM_SLEW_LIMIT_X100)
    {
        return last_x100 - PWM_SLEW_LIMIT_X100;
    }

    return target_x100;
}

static int compute_pi_pwm_x100(int target, int measured, float &integral)
{
    if (target == 0)
    {
        integral = 0.0f;
        return 0;
    }

    int error = target - measured;
    integral += (float)error;
    integral = clamp_float(integral, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    float pwm_percent = 0.0f;
    pwm_percent += SPEED_FF * (float)target;
    pwm_percent += SPEED_KP * (float)error;
    pwm_percent += SPEED_KI * integral;

    int pwm_x100 = round_float_to_int(pwm_percent * 100.0f);
    return clamp_int(pwm_x100, -PWM_LIMIT_X100, PWM_LIMIT_X100);
}

static void reset_kick_if_needed(int target, int last_pwm_x100, int &kick_frames)
{
    if (target == 0)
    {
        kick_frames = 0;
        return;
    }

    if (last_pwm_x100 != 0 && sign_int(target) != sign_int(last_pwm_x100))
    {
        kick_frames = 0;
    }
}

static int apply_min_and_kick(int target,
                              int measured,
                              int last_pwm_x100,
                              int pwm_x100,
                              int min_run_x100,
                              int kick_x100,
                              int kick_start_frames,
                              int stall_measured_max,
                              int &kick_frames)
{
    int s = sign_int(target);
    if (s == 0)
    {
        kick_frames = 0;
        return 0;
    }

    int measured_abs = abs_int(measured);
    int pwm_abs = abs_int(pwm_x100);
    int last_pwm_abs = abs_int(last_pwm_x100);

    if (measured_abs <= stall_measured_max)
    {
        if (kick_frames == 0 && last_pwm_abs < kick_x100)
        {
            kick_frames = kick_start_frames;
        }
    }
    else
    {
        kick_frames = 0;
    }

    if (kick_frames > 0)
    {
        kick_frames--;
        pwm_x100 = s * kick_x100;
    }
    else if (pwm_abs > 0 && pwm_abs < min_run_x100)
    {
        pwm_x100 = s * min_run_x100;
    }

    return clamp_int(pwm_x100, -PWM_LIMIT_X100, PWM_LIMIT_X100);
}

static int compute_left_pwm_x100(int target, int measured, float &integral, int last_pwm_x100)
{
    int pwm_x100 = compute_pi_pwm_x100(target, measured, integral);
    pwm_x100 = apply_slew_limit(pwm_x100, last_pwm_x100);
    reset_kick_if_needed(target, last_pwm_x100, g_left_kick_frames);
    pwm_x100 = apply_min_and_kick(target,
                                  measured,
                                  last_pwm_x100,
                                  pwm_x100,
                                  LEFT_PWM_MIN_RUN_X100,
                                  LEFT_PWM_KICK_X100,
                                  LEFT_KICK_START_FRAMES,
                                  LEFT_STALL_MEASURED_MAX,
                                  g_left_kick_frames);
    return pwm_x100;
}

static int compute_right_pwm_x100(int target, int measured, float &integral, int last_pwm_x100)
{
    int pwm_x100 = compute_pi_pwm_x100(target, measured, integral);
    pwm_x100 = apply_slew_limit(pwm_x100, last_pwm_x100);
    reset_kick_if_needed(target, last_pwm_x100, g_right_kick_frames);
    pwm_x100 = apply_min_and_kick(target,
                                  measured,
                                  last_pwm_x100,
                                  pwm_x100,
                                  RIGHT_PWM_MIN_RUN_X100,
                                  RIGHT_PWM_KICK_X100,
                                  RIGHT_KICK_START_FRAMES,
                                  RIGHT_STALL_MEASURED_MAX,
                                  g_right_kick_frames);
    return pwm_x100;
}

static void print_pwm_x100(const char *name, int value_x100)
{
    int abs_v = abs_int(value_x100);
    printf("%s=%s%d.%02d%%", name, value_x100 < 0 ? "-" : "", abs_v / 100, abs_v % 100);
}

void speed_control_init(int sample_ms)
{
    g_sample_ms = sample_ms;

    g_left_target = 0;
    g_right_target = 0;
    g_left_measured = 0;
    g_right_measured = 0;
    g_left_integral = 0.0f;
    g_right_integral = 0.0f;
    g_left_pwm_x100 = 0;
    g_right_pwm_x100 = 0;
    g_left_kick_frames = 0;
    g_right_kick_frames = 0;

    motor_init();
    motor_stop();
    encoder_init();
    encoder_clear();

    g_speed_inited = true;

    printf("[speed_init] sample_ms=%d Kp=%.6f Ki=%.6f FF=%.6f PWM_LIMIT=%d.%02d%% slew=%d.%02d%%\n",
           g_sample_ms,
           SPEED_KP,
           SPEED_KI,
           SPEED_FF,
           PWM_LIMIT_X100 / 100,
           PWM_LIMIT_X100 % 100,
           PWM_SLEW_LIMIT_X100 / 100,
           PWM_SLEW_LIMIT_X100 % 100);
    printf("[speed_init] left_min=%d.%02d%% right_min=%d.%02d%% right_kick=%d.%02d%% kick_frames=%d\n",
           LEFT_PWM_MIN_RUN_X100 / 100,
           LEFT_PWM_MIN_RUN_X100 % 100,
           RIGHT_PWM_MIN_RUN_X100 / 100,
           RIGHT_PWM_MIN_RUN_X100 % 100,
           RIGHT_PWM_KICK_X100 / 100,
           RIGHT_PWM_KICK_X100 % 100,
           RIGHT_KICK_START_FRAMES);
}

void speed_control_stop(void)
{
    g_left_target = 0;
    g_right_target = 0;
    g_left_integral = 0.0f;
    g_right_integral = 0.0f;
    g_left_pwm_x100 = 0;
    g_right_pwm_x100 = 0;
    g_left_kick_frames = 0;
    g_right_kick_frames = 0;

    motor_stop();
    encoder_clear();
}

void speed_control_set_target(int left_count_per_sample, int right_count_per_sample)
{
    g_left_target = left_count_per_sample;
    g_right_target = right_count_per_sample;
}

void speed_control_update(void)
{
    if (!g_speed_inited)
    {
        speed_control_init(g_sample_ms);
    }

    encoder_update();
    g_left_measured = encoder_get_left();
    g_right_measured = encoder_get_right();

    g_left_pwm_x100 = compute_left_pwm_x100(g_left_target,
                                            g_left_measured,
                                            g_left_integral,
                                            g_left_pwm_x100);

    g_right_pwm_x100 = compute_right_pwm_x100(g_right_target,
                                              g_right_measured,
                                              g_right_integral,
                                              g_right_pwm_x100);

    motor_set_pwm_x100(g_left_pwm_x100, g_right_pwm_x100);
}

int speed_control_get_left_target(void)
{
    return g_left_target;
}

int speed_control_get_right_target(void)
{
    return g_right_target;
}

int speed_control_get_left_measured(void)
{
    return g_left_measured;
}

int speed_control_get_right_measured(void)
{
    return g_right_measured;
}

int speed_control_get_left_pwm(void)
{
    return g_left_pwm_x100 / 100;
}

int speed_control_get_right_pwm(void)
{
    return g_right_pwm_x100 / 100;
}

int speed_control_get_left_pwm_x100(void)
{
    return g_left_pwm_x100;
}

int speed_control_get_right_pwm_x100(void)
{
    return g_right_pwm_x100;
}

void speed_control_print(void)
{
    printf("[speed] TL=%d TR=%d | ML=%d MR=%d | ",
           g_left_target,
           g_right_target,
           g_left_measured,
           g_right_measured);
    print_pwm_x100("PL", g_left_pwm_x100);
    printf(" ");
    print_pwm_x100("PR", g_right_pwm_x100);
    printf(" kickL=%d kickR=%d\n", g_left_kick_frames, g_right_kick_frames);
}
