#include "smartcar_camera.hpp"
#include "smartcar_display.hpp"
#include "smartcar_encoder.hpp"
#include "smartcar_line.hpp"
#include "smartcar_motor.hpp"
#include "smartcar_speed.hpp"
#include "award_signal.hpp"
#include "award_vision_adapter.hpp"

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;
static FILE *g_log = NULL;
static long long g_start_ms = 0;

enum control_mode_t
{
    MODE_LOST = 0,
    MODE_FOLLOW = 1,
    MODE_HARD_LEFT = 2,
    MODE_HARD_RIGHT = 3,
    MODE_HOLD_LEFT = 4,
    MODE_HOLD_RIGHT = 5,
    MODE_RECOVER_LEFT = 6,
    MODE_RECOVER_RIGHT = 7,
};

static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

static int clamp_i(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int abs_i(int value)
{
    return value >= 0 ? value : -value;
}

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
}

static const char *mode_name(control_mode_t mode)
{
    switch (mode)
    {
    case MODE_LOST: return "LOST";
    case MODE_FOLLOW: return "FOLLOW";
    case MODE_HARD_LEFT: return "HARD_L";
    case MODE_HARD_RIGHT: return "HARD_R";
    case MODE_HOLD_LEFT: return "HOLD_L";
    case MODE_HOLD_RIGHT: return "HOLD_R";
    case MODE_RECOVER_LEFT: return "REC_L";
    case MODE_RECOVER_RIGHT: return "REC_R";
    default: return "UNK";
    }
}

static void ensure_log_dir(void)
{
    mkdir("/home/root/line_run", 0777);
    mkdir("/home/root/line_run/pgm", 0777);
    mkdir("/home/root/line_run/ppm", 0777);
}

static void open_log(void)
{
    ensure_log_dir();

    g_log = fopen("/home/root/line_run/ground10c_log.csv", "w");
    if (!g_log)
    {
        printf("[log] open failed: /home/root/line_run/ground10c_log.csv\n");
        return;
    }

    fprintf(g_log,
            "frame,ms,mode,valid,cx,raw_center,near_center,far_center,trend,raw_error,preview_error,used_error,left_x,right_x,rows,line_width,near_width,far_width,left_rows,right_rows,left_target,right_target,left_measured,right_measured,left_pwm_x100,right_pwm_x100,lost_count\n");
    fflush(g_log);
}

static void close_log(void)
{
    if (g_log)
    {
        fclose(g_log);
        g_log = NULL;
    }
}

static void write_log(int frame,
                      control_mode_t mode,
                      const smartcar_line_result_t &line,
                      int raw_error,
                      int preview_error,
                      int used_error,
                      int left_target,
                      int right_target,
                      int lost_count)
{
    if (!g_log)
    {
        return;
    }

    fprintf(g_log,
            "%d,%lld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            frame,
            now_ms() - g_start_ms,
            (int)mode,
            line.valid,
            line.center_x,
            line.raw_center_x,
            line.near_center_x,
            line.far_center_x,
            line.trend,
            raw_error,
            preview_error,
            used_error,
            line.left_x,
            line.right_x,
            line.used_rows,
            line.line_width,
            line.near_width,
            line.far_width,
            line.left_found_rows,
            line.right_found_rows,
            left_target,
            right_target,
            speed_control_get_left_measured(),
            speed_control_get_right_measured(),
            speed_control_get_left_pwm_x100(),
            speed_control_get_right_pwm_x100(),
            lost_count);
    fflush(g_log);
}

static void save_gray_pgm(const char *path, const unsigned char *gray, int width, int height)
{
    if (!gray || width <= 0 || height <= 0)
    {
        return;
    }

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return;
    }

    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    fwrite(gray, 1, width * height, fp);
    fclose(fp);
}

static void save_debug_ppm(const char *path,
                           const unsigned char *gray,
                           int width,
                           int height,
                           const smartcar_line_result_t &line)
{
    if (!gray || width <= 0 || height <= 0)
    {
        return;
    }

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", width, height);

    for (int y = 0; y < height; y++)
    {
        int row80 = y / 2;
        for (int x = 0; x < width; x++)
        {
            unsigned char pixel[3];
            unsigned char g = gray[y * width + x];
            pixel[0] = g;
            pixel[1] = g;
            pixel[2] = g;

            if (line.valid)
            {
                if (row80 >= 0 && row80 < line.debug_rows && line.debug_valid_row[row80])
                {
                    int lx = line.debug_left_x[row80];
                    int rx = line.debug_right_x[row80];
                    int cx = line.debug_center_x[row80];

                    if (x >= lx - 1 && x <= lx + 1)
                    {
                        pixel[0] = 255;
                        pixel[1] = 64;
                        pixel[2] = 64;
                    }
                    if (x >= rx - 1 && x <= rx + 1)
                    {
                        pixel[0] = 64;
                        pixel[1] = 128;
                        pixel[2] = 255;
                    }
                    if (x >= cx - 1 && x <= cx + 1)
                    {
                        pixel[0] = 64;
                        pixel[1] = 255;
                        pixel[2] = 64;
                    }
                }

                if (y >= line.row_y - 2 && y <= line.row_y + 2)
                {
                    if (x >= line.raw_center_x - 1 && x <= line.raw_center_x + 1)
                    {
                        pixel[0] = 255;
                        pixel[1] = 255;
                        pixel[2] = 0;
                    }
                    if (x >= line.preview_center_x - 1 && x <= line.preview_center_x + 1)
                    {
                        pixel[0] = 255;
                        pixel[1] = 0;
                        pixel[2] = 255;
                    }
                }

                if (y >= line.near_row_y - 2 && y <= line.near_row_y + 2)
                {
                    if (x >= line.near_center_x - 1 && x <= line.near_center_x + 1)
                    {
                        pixel[0] = 255;
                        pixel[1] = 160;
                        pixel[2] = 0;
                    }
                }

                if (y >= line.far_row_y - 2 && y <= line.far_row_y + 2)
                {
                    if (x >= line.far_center_x - 1 && x <= line.far_center_x + 1)
                    {
                        pixel[0] = 0;
                        pixel[1] = 255;
                        pixel[2] = 255;
                    }
                }
            }

            fwrite(pixel, 1, 3, fp);
        }
    }

    fclose(fp);
}

static void stop_car(void)
{
    speed_control_stop();
    motor_stop();
}

static void hold_car_stopped(int ms_total)
{
    int loops = ms_total / 50;
    if (loops < 1) loops = 1;
    for (int i = 0; i < loops; ++i)
    {
        speed_control_stop();
        motor_stop();
        usleep(50 * 1000);
    }
}

static void on_signal(int)
{
    g_running = 0;
    motor_stop();
}

static void show_status(const smartcar_line_result_t &line,
                        control_mode_t mode,
                        int raw_error,
                        int preview_error,
                        int used_error,
                        int left_target,
                        int right_target,
                        int lost_count)
{
    display_line(0, "LEARN");
    display_line(1, mode_name(mode));
    display_two_values(2, "RAW", raw_error, "PRE", preview_error);
    display_two_values(3, "USE", used_error, "TR", line.trend);
    display_two_values(4, "CX", line.center_x, "V", line.valid);
    display_two_values(5, "LT", left_target, "RT", right_target);
    display_two_values(6, "ML", speed_control_get_left_measured(), "MR", speed_control_get_right_measured());
    display_two_values(7, "LO", lost_count, "LW", line.line_width);
}

static int choose_gain(int abs_error)
{
    if (abs_error <= 3) return 4;
    if (abs_error <= 8) return 6;
    if (abs_error <= 16) return 10;
    return 14;
}

static int sign_i(int value)
{
    if (value > 0) return 1;
    if (value < 0) return -1;
    return 0;
}

typedef struct
{
    const char *name;
    int left_pwm_x100;
    int right_pwm_x100;
    int hold_ms;
} pwm_test_stage_t;

static int is_pwm_test_mode(int argc, char **argv)
{
    return argc > 1 && argv[1] && strcmp(argv[1], "pwmtest") == 0;
}

static int is_land_test_mode(int argc, char **argv)
{
    return argc > 1 && argv[1] && strcmp(argv[1], "landtest") == 0;
}

static int run_pwm_test_mode(void)
{
    static const pwm_test_stage_t stages[] =
    {
        {"STOP_0", 0, 0, 1000},
        {"L_6", 600, 0, 1200},
        {"L_8", 800, 0, 1200},
        {"L_10", 1000, 0, 1200},
        {"L_12", 1200, 0, 1200},
        {"STOP_1", 0, 0, 1000},
        {"R_6", 0, 600, 1200},
        {"R_8", 0, 800, 1200},
        {"R_10", 0, 1000, 1200},
        {"R_12", 0, 1200, 1200},
        {"R_13", 0, 1300, 1200},
        {"STOP_2", 0, 0, 1000},
        {"B_8", 800, 800, 1200},
        {"B_10", 1000, 1000, 1200},
        {"B_12", 1200, 1200, 1200},
        {"BASE", 1200, 1280, 1500},
        {"STOP_3", 0, 0, 1200},
    };

    printf("\n========== SMARTCAR PWM ENCODER TEST ==========\n");
    printf("[CTRL] Ctrl+C stop motor.\n");
    printf("[MODE] open-loop pwm + encoder observe\n");

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    motor_init();
    motor_stop();
    display_init();
    display_clear();
    display_line(0, "PWM TEST");

    encoder_init();
    encoder_clear();

    for (unsigned int i = 0; i < sizeof(stages) / sizeof(stages[0]) && g_running; ++i)
    {
        const pwm_test_stage_t &stage = stages[i];
        printf("[pwmtest] stage=%s LPWM=%d RPWM=%d hold_ms=%d\n",
               stage.name,
               stage.left_pwm_x100,
               stage.right_pwm_x100,
               stage.hold_ms);

        encoder_clear();
        motor_set_pwm_x100(stage.left_pwm_x100, stage.right_pwm_x100);

        int loops = stage.hold_ms / 100;
        if (loops < 1) loops = 1;

        for (int k = 0; k < loops && g_running; ++k)
        {
            usleep(100 * 1000);
            encoder_update();

            int ml = encoder_get_left();
            int mr = encoder_get_right();

            display_line(0, "PWM TEST");
            display_line(1, stage.name);
            display_two_values(2, "LP", stage.left_pwm_x100, "RP", stage.right_pwm_x100);
            display_two_values(3, "ML", ml, "MR", mr);
            display_two_values(4, "K", k, "N", loops);

            printf("[pwmtest] stage=%s t=%d/%d LPWM=%d RPWM=%d ML=%d MR=%d\n",
                   stage.name,
                   k + 1,
                   loops,
                   stage.left_pwm_x100,
                   stage.right_pwm_x100,
                   ml,
                   mr);
        }

        motor_stop();
        usleep(200 * 1000);
    }

    motor_stop();
    printf("[pwmtest] stopped\n");
    return 0;
}

static int run_land_test_mode(void)
{
    static const pwm_test_stage_t stages[] =
    {
        {"STOP_0", 0, 0, 1000},
        {"B_8", 800, 800, 1200},
        {"STOP_1", 0, 0, 1000},
        {"B_10", 1000, 1000, 1200},
        {"STOP_2", 0, 0, 1000},
        {"B_12", 1200, 1200, 1200},
        {"STOP_3", 0, 0, 1000},
        {"BASE", 1200, 1280, 1500},
        {"STOP_4", 0, 0, 1200},
    };

    printf("\n========== SMARTCAR LAND TEST ==========\n");
    printf("[CTRL] Ctrl+C stop motor.\n");
    printf("[MODE] low-speed ground capability test\n");
    printf("[WARN] place car on ground with clear straight space\n");

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    motor_init();
    motor_stop();
    display_init();
    display_clear();
    display_line(0, "LAND TEST");

    encoder_init();
    encoder_clear();

    for (unsigned int i = 0; i < sizeof(stages) / sizeof(stages[0]) && g_running; ++i)
    {
        const pwm_test_stage_t &stage = stages[i];
        printf("[landtest] stage=%s LPWM=%d RPWM=%d hold_ms=%d\n",
               stage.name,
               stage.left_pwm_x100,
               stage.right_pwm_x100,
               stage.hold_ms);

        encoder_clear();
        motor_set_pwm_x100(stage.left_pwm_x100, stage.right_pwm_x100);

        int loops = stage.hold_ms / 100;
        if (loops < 1) loops = 1;

        for (int k = 0; k < loops && g_running; ++k)
        {
            usleep(100 * 1000);
            encoder_update();

            int ml = encoder_get_left();
            int mr = encoder_get_right();

            display_line(0, "LAND TEST");
            display_line(1, stage.name);
            display_two_values(2, "LP", stage.left_pwm_x100, "RP", stage.right_pwm_x100);
            display_two_values(3, "ML", ml, "MR", mr);
            display_two_values(4, "K", k, "N", loops);

            printf("[landtest] stage=%s t=%d/%d LPWM=%d RPWM=%d ML=%d MR=%d\n",
                   stage.name,
                   k + 1,
                   loops,
                   stage.left_pwm_x100,
                   stage.right_pwm_x100,
                   ml,
                   mr);
        }

        motor_stop();
        usleep(600 * 1000);
    }

    motor_stop();
    printf("[landtest] stopped\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (is_pwm_test_mode(argc, argv))
    {
        return run_pwm_test_mode();
    }

    if (is_land_test_mode(argc, argv))
    {
        return run_land_test_mode();
    }

    const int LEARN_STATIC_MODE = 0;

    printf("\n========== SMARTCAR GROUND10C TRACK FOLLOW ==========\n");
    printf("[CTRL] Ctrl+C stop motor.\n");
    printf("[LOG] /home/root/line_run/ground10c_log.csv\n");

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    g_start_ms = now_ms();
    open_log();

    motor_init();
    motor_stop();
    hold_car_stopped(500);

    display_init();
    display_clear();
    display_line(0, "GROUND10C");

    if (!camera_init())
    {
        printf("[main] camera_init failed\n");
        display_line(1, "CAM FAIL");

        while (1)
        {
            motor_stop();
            delay_ms(1000);
        }
    }

    printf("[main] camera ok, size=%dx%d\n", camera_get_width(), camera_get_height());

    line_init();
    award_vision_init();
    speed_control_init(100);
    speed_control_stop();
    hold_car_stopped(3000);

    const int LEFT_BASE = 1200;
    const int RIGHT_BASE = 1280;
    const int TARGET_MIN = 0;
    const int TARGET_MAX = 2100;
    const int TURN_LIMIT = 950;
    const int ERR_DEADBAND = 4;

    const int HARD_ERR = 15;
    const int HARD_INNER_TARGET = 0;
    const int HARD_OUTER_TARGET = 2250;

    const int HOLD_TRIGGER_ERR = 10;
    const int HOLD_RELEASE_ERR = 6;
    const int HOLD_FRAMES = 6;
    const int FILTER_BLEND_OLD = 1;
    const int FILTER_BLEND_NEW = 2;
    const int FILTER_BLEND_DIV = FILTER_BLEND_OLD + FILTER_BLEND_NEW;
    const int SIGN_FLIP_FAST_ENTER = 8;

    const int RECOVER_FRAMES = 1;
    const int RECOVER_OUTER_TARGET = 1850;

    const int LEFT_TURN_INNER_DROP_GAIN = 18;
    const int RIGHT_TURN_INNER_DROP_GAIN = 10;
    const int LEFT_TURN_OUTER_FLOOR = 1950;
    const int RIGHT_TURN_OUTER_FLOOR = 1450;
    const int LEFT_TURN_CURVE_BRAKE_GAIN = 0;
    const int LEFT_TURN_CURVE_BRAKE_MAX = 0;
    const int RIGHT_TURN_CURVE_BRAKE_GAIN = 0;
    const int RIGHT_TURN_CURVE_BRAKE_MAX = 0;
    const int AWARD_CENTER_X = 45;
    const int AWARD_BIAS_LIMIT = 24;
    const int AWARD_RING_BONUS = 10;
    const int AWARD_EDGE_BONUS = 8;
    const int AWARD_EDGE_TRIGGER = 8;
    const int AWARD_OFFLINE_TRIGGER = 20;
    const int AWARD_OFFLINE_BONUS = 6;
    const int AWARD_OUTER_BOOST = 140;
    const int AWARD_INNER_DROP = 90;

    const int LOST_STOP_FRAMES = 3;
    const int SAVE_FRAME_INTERVAL = 10;
    const int MAX_SAVED_FRAMES = 160;
    const int STARTUP_HOLD_FRAMES = 30;
    const int STARTUP_LOCK_FRAMES = 4;
    const int MIN_USED_ROWS = 6;
    const int MIN_EDGE_FOUND_ROWS = 3;
    const int MIN_LINE_WIDTH = 18;
    const int MAX_LINE_WIDTH = 150;

    int frame_count = 0;
    int lost_count = 0;
    int saved_frames = 0;
    int last_turn_dir = 0;
    int used_error_filtered = 0;
    int hold_dir = 0;
    int hold_frames_left = 0;
    int line_lock_count = 0;

    while (g_running)
    {
        if (!camera_update())
        {
            printf("[main] camera_update failed\n");
            stop_car();
            delay_ms(100);
            continue;
        }

        line_update();
        smartcar_line_result_t line = line_get_result();

        if (!g_running)
        {
            break;
        }

        int width = camera_get_width();
        int raw_error = 0;
        int preview_error = 0;
        int used_error = 0;
        int award_bias = 0;
        int left_target = 0;
        int right_target = 0;
        control_mode_t mode = MODE_LOST;
        AwardSignal award = {};

        bool award_ok = award_vision_process_from_gray((const uint8 *)camera_get_gray(),
                                                       camera_get_width(),
                                                       camera_get_height(),
                                                       &award);

        if (award_ok)
        {
            int award_det_error = award.det_true - AWARD_CENTER_X;
            award_bias = award_det_error / 2;

            if (award.is_left_ring)
            {
                award_bias -= AWARD_RING_BONUS;
            }
            else if (award.is_right_ring)
            {
                award_bias += AWARD_RING_BONUS;
            }

            if (award.left_line >= award.right_line + AWARD_EDGE_TRIGGER)
            {
                award_bias -= AWARD_EDGE_BONUS;
            }
            else if (award.right_line >= award.left_line + AWARD_EDGE_TRIGGER)
            {
                award_bias += AWARD_EDGE_BONUS;
            }

            if (award.offline >= AWARD_OFFLINE_TRIGGER)
            {
                award_bias += sign_i(award_bias != 0 ? award_bias : award_det_error) * AWARD_OFFLINE_BONUS;
            }

            award_bias = clamp_i(award_bias, -AWARD_BIAS_LIMIT, AWARD_BIAS_LIMIT);
        }

        int line_ok = line.valid &&
                      line.used_rows >= MIN_USED_ROWS &&
                      line.left_found_rows >= MIN_EDGE_FOUND_ROWS &&
                      line.right_found_rows >= MIN_EDGE_FOUND_ROWS &&
                      line.line_width >= MIN_LINE_WIDTH &&
                      line.line_width <= MAX_LINE_WIDTH;

        if (line_ok)
        {
            lost_count = 0;

            raw_error = line.raw_center_x - width / 2;
            preview_error = line.error;
            int fused_error = raw_error;

            if (abs_i(raw_error) <= 6)
            {
                fused_error = raw_error;
            }
            else if (raw_error < 0 && preview_error < 0)
            {
                fused_error = (2 * raw_error + preview_error) / 3;
            }
            else if (raw_error > 0 && preview_error > 0)
            {
                fused_error = (2 * raw_error + preview_error) / 3;
            }
            else
            {
                fused_error = raw_error;
            }

            if (award_ok)
            {
                fused_error = clamp_i(fused_error + award_bias, -79, 79);
            }

            if (sign_i(fused_error) != sign_i(used_error_filtered) &&
                abs_i(fused_error) >= SIGN_FLIP_FAST_ENTER)
            {
                used_error_filtered = fused_error;
            }
            else
            {
                used_error_filtered =
                    (used_error_filtered * FILTER_BLEND_OLD + fused_error * FILTER_BLEND_NEW) / FILTER_BLEND_DIV;
            }

            used_error = used_error_filtered;

            if (used_error >= -ERR_DEADBAND && used_error <= ERR_DEADBAND)
            {
                used_error = 0;
                used_error_filtered = 0;
            }

            if (hold_dir < 0)
            {
                left_target = HARD_INNER_TARGET;
                right_target = HARD_OUTER_TARGET;
                last_turn_dir = -1;
                mode = MODE_HOLD_LEFT;

                if (used_error >= -HOLD_RELEASE_ERR || raw_error >= 2)
                {
                    hold_frames_left--;
                }
                else
                {
                    hold_frames_left = HOLD_FRAMES;
                }

                if (hold_frames_left <= 0)
                {
                    hold_dir = 0;
                }
            }
            else if (hold_dir > 0)
            {
                left_target = HARD_OUTER_TARGET;
                right_target = HARD_INNER_TARGET;
                last_turn_dir = 1;
                mode = MODE_HOLD_RIGHT;

                if (used_error <= HOLD_RELEASE_ERR || raw_error <= -2)
                {
                    hold_frames_left--;
                }
                else
                {
                    hold_frames_left = HOLD_FRAMES;
                }

                if (hold_frames_left <= 0)
                {
                    hold_dir = 0;
                }
            }
            else if (used_error <= -HARD_ERR && preview_error <= -10)
            {
                hold_dir = -1;
                hold_frames_left = HOLD_FRAMES;
                left_target = HARD_INNER_TARGET;
                right_target = HARD_OUTER_TARGET;
                last_turn_dir = -1;
                mode = MODE_HOLD_LEFT;
            }
            else if (used_error >= HARD_ERR && preview_error >= 10)
            {
                hold_dir = 1;
                hold_frames_left = HOLD_FRAMES;
                left_target = HARD_OUTER_TARGET;
                right_target = HARD_INNER_TARGET;
                last_turn_dir = 1;
                mode = MODE_HOLD_RIGHT;
            }
            else if (used_error <= -HOLD_TRIGGER_ERR && preview_error <= -8)
            {
                hold_dir = -1;
                hold_frames_left = HOLD_FRAMES / 2;
                left_target = HARD_INNER_TARGET;
                right_target = HARD_OUTER_TARGET;
                last_turn_dir = -1;
                mode = MODE_HOLD_LEFT;
            }
            else if (used_error >= HOLD_TRIGGER_ERR && preview_error >= 8)
            {
                hold_dir = 1;
                hold_frames_left = HOLD_FRAMES / 2;
                left_target = HARD_OUTER_TARGET;
                right_target = HARD_INNER_TARGET;
                last_turn_dir = 1;
                mode = MODE_HOLD_RIGHT;
            }
            else
            {
                int abs_error = abs_i(used_error);
                int gain = choose_gain(abs_error);
                int turn = clamp_i(used_error * gain, -TURN_LIMIT, TURN_LIMIT);
                left_target = LEFT_BASE + turn;
                right_target = RIGHT_BASE - turn;

                if (used_error < 0)
                {
                    int curve_brake = clamp_i(abs_error * LEFT_TURN_CURVE_BRAKE_GAIN,
                                              0,
                                              LEFT_TURN_CURVE_BRAKE_MAX);
                    left_target -= curve_brake;
                    right_target -= curve_brake;
                    left_target -= abs_error * LEFT_TURN_INNER_DROP_GAIN;
                    if (abs_error >= 6 && right_target < LEFT_TURN_OUTER_FLOOR)
                    {
                        right_target = LEFT_TURN_OUTER_FLOOR;
                    }
                    last_turn_dir = -1;
                }
                else if (used_error > 0)
                {
                    int curve_brake = clamp_i(abs_error * RIGHT_TURN_CURVE_BRAKE_GAIN,
                                              0,
                                              RIGHT_TURN_CURVE_BRAKE_MAX);
                    left_target -= curve_brake;
                    right_target -= curve_brake;
                    right_target -= abs_error * RIGHT_TURN_INNER_DROP_GAIN;
                    if (abs_error >= 6 && left_target < RIGHT_TURN_OUTER_FLOOR)
                    {
                        left_target = RIGHT_TURN_OUTER_FLOOR;
                    }
                    last_turn_dir = 1;
                }

                if (award_ok && award_bias <= -8)
                {
                    left_target -= AWARD_INNER_DROP;
                    right_target += AWARD_OUTER_BOOST;
                }
                else if (award_ok && award_bias >= 8)
                {
                    left_target += AWARD_OUTER_BOOST;
                    right_target -= AWARD_INNER_DROP;
                }

                left_target = clamp_i(left_target, TARGET_MIN, TARGET_MAX);
                right_target = clamp_i(right_target, TARGET_MIN, TARGET_MAX);

                mode = MODE_FOLLOW;
            }
        }
        else
        {
            lost_count++;
            raw_error = 0;
            preview_error = 0;
            used_error = 0;
            hold_dir = 0;
            hold_frames_left = 0;

            if (last_turn_dir < 0 && lost_count <= RECOVER_FRAMES)
            {
                left_target = 0;
                right_target = RECOVER_OUTER_TARGET;
                mode = MODE_RECOVER_LEFT;
            }
            else if (last_turn_dir > 0 && lost_count <= RECOVER_FRAMES)
            {
                left_target = RECOVER_OUTER_TARGET;
                right_target = 0;
                mode = MODE_RECOVER_RIGHT;
            }
            else
            {
                if (lost_count >= LOST_STOP_FRAMES)
                {
                    left_target = 0;
                    right_target = 0;
                    mode = MODE_LOST;
                }
            }
        }

        if (!g_running)
        {
            break;
        }

        if (frame_count < STARTUP_HOLD_FRAMES)
        {
            last_turn_dir = 0;
            hold_dir = 0;
            hold_frames_left = 0;
            line_lock_count = 0;
            left_target = 0;
            right_target = 0;
            used_error = 0;
            used_error_filtered = 0;
            mode = MODE_LOST;
            speed_control_stop();
            motor_stop();
        }
        else if (line_ok && line_lock_count < STARTUP_LOCK_FRAMES)
        {
            line_lock_count++;
            left_target = 0;
            right_target = 0;
            used_error = 0;
            used_error_filtered = 0;
            hold_dir = 0;
            hold_frames_left = 0;
            mode = MODE_LOST;
            speed_control_stop();
            motor_stop();
        }
        else if (!line_ok)
        {
            line_lock_count = 0;
        }

        if (LEARN_STATIC_MODE)
        {
            speed_control_stop();
            motor_stop();
            left_target = 0;
            right_target = 0;
        }
        else
        {
            speed_control_set_target(left_target, right_target);
            speed_control_update();
        }

        write_log(frame_count,
                  mode,
                  line,
                  raw_error,
                  preview_error,
                  used_error,
                  left_target,
                  right_target,
                  lost_count);

        if (saved_frames < MAX_SAVED_FRAMES &&
            (frame_count % SAVE_FRAME_INTERVAL == 0 || !line.valid))
        {
            char path[256];
            snprintf(path, sizeof(path), "/home/root/line_run/pgm/frame_%05d.pgm", frame_count);
            save_gray_pgm(path,
                          (const unsigned char *)camera_get_gray(),
                          camera_get_width(),
                          camera_get_height());
            snprintf(path, sizeof(path), "/home/root/line_run/ppm/frame_%05d.ppm", frame_count);
            save_debug_ppm(path,
                           (const unsigned char *)camera_get_gray(),
                           camera_get_width(),
                           camera_get_height(),
                           line);
            saved_frames++;
        }

        if ((frame_count % 1) == 0)
        {
            show_status(line,
                        mode,
                        raw_error,
                        preview_error,
                        used_error,
                        left_target,
                        right_target,
                        lost_count);

            printf("[ground10c] frame=%d mode=%s V=%d raw=%d pre=%d used=%d ab=%d det=%d road=%d ring=%d tr=%d cx=%d lt=%d rt=%d ml=%d mr=%d\n",
                   frame_count,
                   mode_name(mode),
                   line.valid,
                   raw_error,
                   preview_error,
                   used_error,
                   award_bias,
                   award.det_true,
                   award.road_type,
                   award.rings_flag,
                   line.trend,
                   line.center_x,
                   left_target,
                   right_target,
                   speed_control_get_left_measured(),
                   speed_control_get_right_measured());
        }

        frame_count++;
        delay_ms(100);
    }

    stop_car();
    close_log();
    printf("[main] stopped\n");

    return 0;
}
