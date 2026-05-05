#include "zf_common_headfile.h"

extern volatile uint32 g_pit_ticks;
extern volatile uint32 g_motor_control_ticks;
extern volatile int16 g_last_encoder_left;
extern volatile int16 g_last_encoder_right;
extern volatile int32 g_last_left_pwm_cmd;
extern volatile int32 g_last_right_pwm_cmd;

static void cleanup_all()
{
    pwm_set_duty(PWM1, 0);
    pwm_set_duty(PWM2, 0);
    pwm_set_duty(PWM3, 0);
}

static void sigint_handler(int)
{
    cleanup_all();
    exit(0);
}

static uint8 image_copy[UVC_HEIGHT][UVC_WIDTH];

int main()
{
    uint32 frame_total = 0;
    uint32 last_frame_total = 0;
    uint32 last_pit_ticks = 0;
    uint32 last_motor_ticks = 0;

    signal(SIGINT, sigint_handler);
    atexit(cleanup_all);

    Init_all();
    Data_Settings();
    printf("assistant disabled, run standalone\r\n");

    while (true)
    {
        if (wait_image_refresh() < 0)
        {
            exit(0);
        }

        ++frame_total;
        ImageProcess();

        for (int y = 0; y < 60; ++y)
        {
            for (int x = 0; x < 80; ++x)
            {
                uint8 color = (Pixle[y][x] == 0) ? 0 : 255;
                int map_y = y * 2;
                int map_x = x * 2;
                image_copy[map_y][map_x] = color;
                image_copy[map_y][map_x + 1] = color;
                image_copy[map_y + 1][map_x] = color;
                image_copy[map_y + 1][map_x + 1] = color;
            }
        }

        for (int y = 0; y < 60; ++y)
        {
            int left_x = ImageDeal[y].close_LeftBorder * 2;
            int right_x = ImageDeal[y].close_RightBorder * 2;
            int mid_x = ImageDeal[y].Center * 2;
            int map_y = y * 2;

            if (left_x >= 0 && left_x < 160)
            {
                image_copy[map_y][left_x] = 128;
                image_copy[map_y + 1][left_x] = 128;
            }

            if (right_x >= 0 && right_x < 160)
            {
                image_copy[map_y][right_x] = 128;
                image_copy[map_y + 1][right_x] = 128;
            }

            if (mid_x >= 0 && mid_x < 160)
            {
                image_copy[map_y][mid_x] = 128;
                image_copy[map_y + 1][mid_x] = 128;
            }
        }

        if ((frame_total % 60) == 0)
        {
            uint32 pit_now = g_pit_ticks;
            uint32 motor_now = g_motor_control_ticks;
            printf("[alive] frame=%u frame_delta=%u pit=%u pit_delta=%u motor=%u motor_delta=%u det=%d road=%d rings=%d Lenc=%d Renc=%d Lpwm=%d Rpwm=%d\r\n",
                   frame_total,
                   frame_total - last_frame_total,
                   pit_now,
                   pit_now - last_pit_ticks,
                   motor_now,
                   motor_now - last_motor_ticks,
                   ImageStatus.Det_True,
                   ImageStatus.Road_type,
                   ImageFlag.image_element_rings_flag,
                   g_last_encoder_left,
                   g_last_encoder_right,
                   g_last_left_pwm_cmd,
                   g_last_right_pwm_cmd);
            last_frame_total = frame_total;
            last_pit_ticks = pit_now;
            last_motor_ticks = motor_now;
        }
    }

    return 0;
}
