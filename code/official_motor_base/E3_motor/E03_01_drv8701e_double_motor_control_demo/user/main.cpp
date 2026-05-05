#include "smartcar_motor.hpp"
#include "smartcar_display.hpp"
#include "smartcar_camera.hpp"
#include "seekfree_assistant.hpp"
#include "seekfree_assistant_interface.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "smartcar_line.hpp"
#include "smartcar_encoder.hpp"
#include "smartcar_speed.hpp"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

static volatile int g_running = 1;

static void delay_ms(int ms)
{
    usleep(ms * 1000);
}

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int abs_i(int v)
{
    return v < 0 ? -v : v;
}


// ================= SeekFree Assistant TCP RGB565 preview =================
// PC: SeekFree Assistant V1.2.7 -> Image Transfer -> Realtime Image -> Network -> TCP Server
// PC listen address: 0.0.0.0
// PC listen port:    8086
// Current PC WiFi IPv4: 192.168.11.228
static const char *ASSISTANT_TCP_SERVER_IP = "192.168.11.228";
static const int ASSISTANT_TCP_SERVER_PORT = 8086;
static const bool ASSISTANT_PREVIEW_ENABLE = false;

static int g_assistant_sock = -1;
static bool g_assistant_enabled = false;

// SeekFree Assistant expects RGB565 bytes in high-byte-first order.
// camera_get_rgb565() memory order must be byte-swapped before sending.
static uint16 g_assistant_rgb565_swapped[320 * 240];

static uint64_t assistant_now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
}

static void assistant_tcp_close(void)
{
    if (g_assistant_sock >= 0)
    {
        close(g_assistant_sock);
        g_assistant_sock = -1;
    }
    g_assistant_enabled = false;
}

static bool assistant_tcp_connect(void)
{
    assistant_tcp_close();

    g_assistant_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_assistant_sock < 0)
    {
        printf("[assistant] socket failed: %s\n", strerror(errno));
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ASSISTANT_TCP_SERVER_PORT);

    if (inet_pton(AF_INET, ASSISTANT_TCP_SERVER_IP, &addr.sin_addr) != 1)
    {
        printf("[assistant] bad ip: %s\n", ASSISTANT_TCP_SERVER_IP);
        assistant_tcp_close();
        return false;
    }

    printf("[assistant] connecting to %s:%d ...\n",
           ASSISTANT_TCP_SERVER_IP, ASSISTANT_TCP_SERVER_PORT);

    if (connect(g_assistant_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("[assistant] connect failed: %s\n", strerror(errno));
        printf("[assistant] make sure PC assistant is listening on TCP Server 0.0.0.0:8086\n");
        assistant_tcp_close();
        return false;
    }

    int one = 1;
    setsockopt(g_assistant_sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    int sndbuf = 64 * 1024;
    setsockopt(g_assistant_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));

    printf("[assistant] connected. TCP_NODELAY=1 SO_SNDBUF=%d\n", sndbuf);
    g_assistant_enabled = true;
    return true;
}

static uint32 assistant_tcp_send(const uint8 *buff, uint32 length)
{
    if (g_assistant_sock < 0 || buff == NULL || length == 0)
    {
        return 0;
    }

    uint32 sent_total = 0;

    while (sent_total < length)
    {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif

        ssize_t n = send(g_assistant_sock, buff + sent_total, length - sent_total, flags);

        if (n <= 0)
        {
            printf("[assistant] send failed %u/%u: %s\n",
                   sent_total, length, strerror(errno));
            assistant_tcp_close();
            break;
        }

        sent_total += (uint32)n;
    }

    return sent_total;
}

static uint32 assistant_tcp_recv(uint8 *buff, uint32 length)
{
    (void)buff;
    (void)length;
    return 0;
}

static void assistant_preview_init(void)
{
    if (!ASSISTANT_PREVIEW_ENABLE)
    {
        g_assistant_enabled = false;
        printf("[assistant] preview disabled by build option\n");
        return;
    }

#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    if (assistant_tcp_connect())
    {
        seekfree_assistant_interface_init(assistant_tcp_send, assistant_tcp_recv);
        printf("[assistant] seekfree interface init done\n");
    }
    else
    {
        printf("[assistant] preview disabled, line-follow continues without PC image\n");
    }
}

static void assistant_send_rgb565_preview_limited(int frame, const smartcar_line_result_t *line)
{
    (void)frame;

    if (!ASSISTANT_PREVIEW_ENABLE)
    {
        return;
    }

    static uint64_t last_reconnect_ms = 0;

    if (!g_assistant_enabled || g_assistant_sock < 0)
    {
        uint64_t now_retry_ms = assistant_now_ms();

        // 如果逐飞助手断开/发送失败，每 1 秒自动尝试重连一次。
        if (last_reconnect_ms == 0 || now_retry_ms - last_reconnect_ms >= 1000)
        {
            last_reconnect_ms = now_retry_ms;
            if (assistant_tcp_connect())
            {
                seekfree_assistant_interface_init(assistant_tcp_send, assistant_tcp_recv);
                printf("[assistant] reconnected and interface init done\n");
            }
        }

        return;
    }

    static uint64_t last_send_ms = 0;
    uint64_t now_ms = assistant_now_ms();

    // Limit to about 8 FPS, still avoid disturbing line-follow timing.
    if (last_send_ms != 0 && now_ms - last_send_ms < 120)
    {
        return;
    }
    last_send_ms = now_ms;

    uint16 *rgb565 = camera_get_rgb565();
    int width = camera_get_width();
    int height = camera_get_height();

    if (rgb565 == NULL || width <= 0 || height <= 0 || width > 320 || height > 240)
    {
        printf("[assistant] invalid rgb565=%p width=%d height=%d\n", rgb565, width, height);
        return;
    }

    uint32 pixels = (uint32)width * (uint32)height;
    for (uint32 i = 0; i < pixels; ++i)
    {
        uint16 px = rgb565[i];
        g_assistant_rgb565_swapped[i] = (uint16)((px << 8) | (px >> 8));
    }

    // Overlay line-follow debug markers on preview buffer only.
    // Green: image center. Red: detected track center.
    // Blue: seed-row left/right boundary. Yellow: seed-row detected width.
    const int half = 1;

    const uint16 green_swapped  = (uint16)((0x07E0 << 8) | (0x07E0 >> 8)); // RGB565 green
    const uint16 red_swapped    = (uint16)((0xF800 << 8) | (0xF800 >> 8)); // RGB565 red
    const uint16 blue_swapped   = (uint16)((0x001F << 8) | (0x001F >> 8)); // RGB565 blue
    const uint16 yellow_swapped = (uint16)((0xFFE0 << 8) | (0xFFE0 >> 8)); // RGB565 yellow

    // Image center: green vertical line.
    const int img_center_x = width / 2;
    for (int y = 0; y < height; ++y)
    {
        for (int dx = -half; dx <= half; ++dx)
        {
            int x = img_center_x + dx;
            if (x >= 0 && x < width)
            {
                g_assistant_rgb565_swapped[(uint32)y * width + x] = green_swapped;
            }
        }
    }

    if (line != NULL && line->valid)
    {
        // Detected center: red vertical line.
        for (int y = 0; y < height; ++y)
        {
            for (int dx = -half; dx <= half; ++dx)
            {
                int x = line->center_x + dx;
                if (x >= 0 && x < width)
                {
                    g_assistant_rgb565_swapped[(uint32)y * width + x] = red_swapped;
                }
            }
        }

        // Seed-row left/right boundaries: blue vertical lines.
        for (int y = 0; y < height; ++y)
        {
            for (int dx = -half; dx <= half; ++dx)
            {
                int xl = line->left_x + dx;
                int xr = line->right_x + dx;

                if (xl >= 0 && xl < width)
                {
                    g_assistant_rgb565_swapped[(uint32)y * width + xl] = blue_swapped;
                }
                if (xr >= 0 && xr < width)
                {
                    g_assistant_rgb565_swapped[(uint32)y * width + xr] = blue_swapped;
                }
            }
        }

        // Seed row width: yellow horizontal segment from left_x to right_x.
        int row_y = line->row_y;
        int x0 = line->left_x;
        int x1 = line->right_x;

        if (x0 > x1)
        {
            int t = x0;
            x0 = x1;
            x1 = t;
        }

        if (x0 < 0) x0 = 0;
        if (x1 >= width) x1 = width - 1;

        for (int dy = -half; dy <= half; ++dy)
        {
            int y = row_y + dy;
            if (y < 0 || y >= height)
            {
                continue;
            }

            for (int x = x0; x <= x1; ++x)
            {
                g_assistant_rgb565_swapped[(uint32)y * width + x] = yellow_swapped;
            }
        }
    }

    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_RGB565,
                                                 g_assistant_rgb565_swapped,
                                                 (uint16)width,
                                                 (uint16)height);
    seekfree_assistant_camera_boundary_config(NO_BOUNDARY, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    seekfree_assistant_camera_send();
}
// ========================================================================

#if 0
static void save_line_debug_ppm(int frame, const smartcar_line_result_t &line, int raw_err, int used_err)
{
    const unsigned char *gray = (const unsigned char *)camera_get_gray();
    int w = camera_get_width();
    int h = camera_get_height();

    if (!gray || w <= 0 || h <= 0)
    {
        return;
    }

    mkdir("/home/root/line_debug", 0755);

    char path[256];
    snprintf(path, sizeof(path),
             "/home/root/line_debug/frame_%04d_cx%d_raw%d_used%d.ppm",
             frame, line.center_x, raw_err, used_err);

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", w, h);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            unsigned char v = gray[y * w + x];
            unsigned char r = v;
            unsigned char g = v;
            unsigned char b = v;

            // 绿色：图像中心
            if (abs_i(x - w / 2) <= 1)
            {
                r = 0; g = 255; b = 0;
            }

            // 红色：算法输出中心 line.center_x
            if (line.valid && abs_i(x - line.center_x) <= 1)
            {
                r = 255; g = 0; b = 0;
            }

            // 蓝色：seed 行检测到的左右边界
            if (line.valid && (abs_i(x - line.left_x) <= 1 || abs_i(x - line.right_x) <= 1))
            {
                r = 0; g = 80; b = 255;
            }

            // 黄色：seed 行
            if (line.valid && abs_i(y - line.row_y) <= 1 && x >= line.left_x && x <= line.right_x)
            {
                r = 255; g = 255; b = 0;
            }

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    fclose(fp);
}
#endif

static void on_signal(int)
{
    g_running = 0;
    speed_control_stop();
    motor_stop();
}

int main(int, char **)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("\n========== SMARTCAR GROUND LINE TEST V10C CLOSED LOOP ==========\n");
    printf("[BUILD_TAG] RESTORE_GOOD_RIGHT_EXIT_20260502_1855\n");
    printf("[MODE] camera feedback + encoder speed closed-loop line following.\n");
    printf("[SAFETY] Ctrl+C stop motor.\n");

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    motor_init();
    motor_stop();

    encoder_init();
    encoder_clear();

    display_init();
    display_clear();
    display_line(0, "GROUND V10C");

    if (!camera_init())
    {
        printf("[main] camera_init failed\n");
        while (1)
        {
            motor_stop();
            delay_ms(1000);
        }
    }

    assistant_preview_init();

    line_init();

    // 编码器速度闭环周期 100ms
    speed_control_init(60);
    speed_control_stop();

    // ===== 基准速度 =====
    // 保留你已经验证过的直行基准，但整体略降速
    const int LEFT_BASE  = 1150;
    const int RIGHT_BASE = 1320;

    // ===== 目标速度限幅 =====
    // V9 的 900 太高，内侧轮降不下来，转弯半径太大
    const int TARGET_MIN = 0;
    const int TARGET_MAX = 2300;

    // ===== 转向参数 =====
    const int ERR_DEADBAND = 2;

    // 小误差/中误差/大误差分段增益
    const int GAIN_SMALL = 12;
    const int GAIN_MID   = 28;
    const int GAIN_BIG   = 46;
    const int LEFT_GAIN_BONUS = 4;
    const int RIGHT_GAIN_BONUS = 12;
    const int CURVE_BRAKE_GAIN = 16;
    const int CURVE_BRAKE_MAX = 700;
    const int RIGHT_CURVE_BRAKE_EXTRA_GAIN = 10;
    const int LOW_CONF_BRAKE_ROWS = 38;
    const int LOW_CONF_BRAKE_LW = 118;
    const int LOW_CONF_BRAKE_GAIN = 12;
    const int LOW_CONF_BRAKE_MAX = 420;

    // 比 V9 的 650 大，否则差速不够
    const int TURN_LIMIT = 1500;

    // ===== outer wheel boost cap =====
    // 进弯时不要靠外侧轮猛加速来转弯，主要靠内侧轮降速形成差速。
    // 否则 USED=28 时外侧轮会从基础速度直接冲到 2100+。
    const int OUTER_BOOST_CAP = 650;

    // ===== right pivot assist =====
    // 右弯现在弱，是因为 RIGHT_CAP=28 让它进不了 HARD_RIGHT=40 的强右模式；
    // 当右弯已经很明确时，保持左外侧轮不过快，但把右内侧轮压低，增强右转。
    const int RIGHT_PIVOT_TRIGGER = 16;
    const int RIGHT_PIVOT_OUTER = 2000;
    const int RIGHT_PIVOT_INNER = 0;
    const int RIGHT_PIVOT_MIN_ROWS = 30;
    const int RIGHT_PIVOT_MIN_LW = 80;
    const int RIGHT_PIVOT_SLOW_OUTER = 1750;

    // 强弯阈值：左弯保留强左；右弯不再轻易 RT=0 强右打
    const int HARD_LEFT_ERR = 15;
    const int HARD_RIGHT_ERR = 24;

    // 右弯普通差速上限：足够转弯，但不让右轮直接停死
    const int RIGHT_USED_CAP = 42;

    // 强弯时仍然走 speed_control_set_target，保持闭环
    const int HARD_INNER_TARGET = 0;
    const int HARD_OUTER_TARGET = 1900;
    const int HARD_LEFT_OUTER_TARGET = 1750;
    const int HARD_RIGHT_SLOW_OUTER = 1700;

    // 弯道方向保持，防止视觉把右边线误当中心后反打
    const int HOLD_TRIGGER_ERR = 8;
    const int LEFT_HOLD_TRIGGER_ERR = 8;
    const int HOLD_FRAMES = 3;   // 3 * 80ms = 240ms

    // 左弯释放后的保护窗口：
    // 防止刚出左弯时，视觉一两帧把右边线误判成中心，立刻强右打。
    const int LEFT_EXIT_GUARD_FRAMES = 6;
    const int LEFT_EXIT_RIGHT_ERR = 8;
    const int LOST_USED_ROWS_MIN = 10;
    const int LOST_LINE_WIDTH_MIN = 34;
    const int LOST_LINE_WIDTH_MAX = 170;
    printf("[param] BASE L=%d R=%d TARGET_MIN=%d TARGET_MAX=%d HARD_LEFT=%d HARD_RIGHT=%d RIGHT_CAP=%d HARD_INNER=%d HARD_OUTER=%d\n",
           LEFT_BASE, RIGHT_BASE, TARGET_MIN, TARGET_MAX,
           HARD_LEFT_ERR, HARD_RIGHT_ERR, RIGHT_USED_CAP,
           HARD_INNER_TARGET, HARD_OUTER_TARGET);

    printf("[main] start in 2 seconds.\n");
    display_line(9, "START IN 2S");
    delay_ms(2000);

    int frame = 0;
    int lost_count = 0;

    int filtered_err = 0;
    int filter_inited = 0;

    int left_hold = 0;
    int right_hold = 0;
    int left_exit_guard = 0;

    while (g_running)
    {
        if (!camera_update())
        {
            speed_control_set_target(0, 0);
            speed_control_update();
            motor_stop();
            delay_ms(40);
            continue;
        }

        line_update();
        smartcar_line_result_t line = line_get_result();

        assistant_send_rgb565_preview_limited(frame, &line);

        int raw_err = 0;
        int used_err = 0;
        int lt = 0;
        int rt = 0;
        int turn = 0;
        int mode = 0; // 0 lost, 1 normal closed-loop, 2 hard-left closed-loop, 3 hard-right closed-loop

        int line_confidence_bad =
            (!line.valid) ||
            (line.used_rows < LOST_USED_ROWS_MIN) ||
            (line.line_width < LOST_LINE_WIDTH_MIN) ||
            (line.line_width > LOST_LINE_WIDTH_MAX);

        if (!line_confidence_bad)
        {
            lost_count = 0;
            raw_err = line.error;

            // ===== 误差滤波 =====
            if (!filter_inited)
            {
                filtered_err = raw_err;
                filter_inited = 1;
            }
            else
            {
                // 比 V9 更灵敏，当前帧权重大
                if ((filtered_err > 0 && raw_err < 0) ||
                    (filtered_err < 0 && raw_err > 0) ||
                    abs_i(raw_err) >= 18)
                {
                    filtered_err = raw_err;
                }
                else
                {
                    filtered_err = (filtered_err + raw_err * 4) / 5;
                }
            }

            used_err = filtered_err;

            if (abs_i(used_err) <= ERR_DEADBAND)
            {
                used_err = 0;
            }

            // 回正优先：原始误差已经接近 0 时，释放弯道保持
            if (abs_i(raw_err) <= 4)
            {
                if (left_hold > 0)
                {
                    left_exit_guard = LEFT_EXIT_GUARD_FRAMES;
                }

                left_hold = 0;
                right_hold = 0;
                filtered_err = 0;
                used_err = 0;
            }

            int was_left_hold = left_hold;

            // ===== 方向保持：已有方向优先，避免左弯出弯时被一帧右边线误判直接抢方向 =====
            // err < 0 左转；err > 0 右转
            if (left_hold > 0)
            {
                // 左弯保持期内，不允许一帧正误差直接切到右弯保持。
                // raw 接近 0 的情况前面已经释放；这里主要防右边线误识别。
                if (raw_err <= -LEFT_HOLD_TRIGGER_ERR)
                {
                    left_hold = HOLD_FRAMES;
                }
                else
                {
                    left_hold--;
                }
                right_hold = 0;
            }
            else if (right_hold > 0)
            {
                // 右弯同理，避免一帧负误差直接反打。
                if (raw_err >= HOLD_TRIGGER_ERR)
                {
                    right_hold = HOLD_FRAMES;
                }
                else
                {
                    right_hold--;
                }
                left_hold = 0;
            }
            else if (raw_err <= -LEFT_HOLD_TRIGGER_ERR)
            {
                left_hold = HOLD_FRAMES;
                right_hold = 0;
            }
            else if (raw_err >= HOLD_TRIGGER_ERR)
            {
                right_hold = HOLD_FRAMES;
                left_hold = 0;
            }

            // 如果左弯保持刚刚自然结束，也启动退出保护窗口。
            // 之前只在 raw 接近 0 时触发，容易只剩 1~2 帧保护，不够挡住后续右边线误判。
            if (was_left_hold > 0 && left_hold == 0 && raw_err > -HOLD_TRIGGER_ERR)
            {
                left_exit_guard = LEFT_EXIT_GUARD_FRAMES;
            }

            // 左弯保持期，禁止突然正误差把车骗去右转
            if (left_hold > 0 && used_err > 0)
            {
                used_err = -6;
            }

            // 右弯保持期，禁止突然负误差把车骗去左转
            if (right_hold > 0 && used_err < 0)
            {
                used_err = 6;
            }

            // ===== anti-right-edge-follow，温和版 =====
            // 只在左弯保持期内处理疑似右边线误判，不再强行 -10，避免直道摇晃。
            int anti_right_edge = 0;
            if (left_hold > 0 && raw_err >= 8)
            {
                anti_right_edge = 1;
            }

            // 行数少、线宽异常时，右侧中心更可疑，给轻微左修正。
            if (left_hold > 0 && (line.used_rows < 24 || line.line_width < 70) && raw_err > 0)
            {
                anti_right_edge = 1;
            }

            if (anti_right_edge)
            {
                used_err = -6;
                right_hold = 0;
            }

            // ===== left-exit guard =====
            // 左弯刚释放后的几帧，过去这里会把可疑右偏压成 USED=0。
            // 现在改成“限幅右转”：允许右弯进去，但不直接强右打。
            if (left_exit_guard > 0)
            {
                if (raw_err <= -HOLD_TRIGGER_ERR)
                {
                    // 真的又进入左弯，退出保护。
                    left_exit_guard = 0;
                }
                else if (raw_err >= LEFT_EXIT_RIGHT_ERR)
                {
                    // 关键修正：看到右弯时不要压成 0，而是给限幅右修。
                    if (raw_err >= 24 && line.used_rows >= 24 && line.line_width >= 44)
                    {
                        // 15 是普通右转上限；HARD_ERR=16，不能到 16，否则会强右。
                        used_err = 22;
                    }
                    else if (raw_err >= 18)
                    {
                        used_err = 18;
                    }
                    else
                    {
                        used_err = 12;
                    }

                    right_hold = 0;
                    left_exit_guard--;
                }
                else
                {
                    left_exit_guard--;
                }
            }

            // ===== right-exit release =====
            // 右弯保持期内，如果 RAW 已经降下来，说明接近出弯，立刻释放右弯保持。
            // 防止右弯已经转过去后，RH 还保持导致继续右打冲出赛道。
            if (right_hold > 0 && raw_err <= 18)
            {
                right_hold = 0;
                if (used_err > 10)
                {
                    used_err = 10;
                }
            }

            // ===== low-confidence strong-turn guard =====
            // 左弯目前表现还可以，继续保守保护。
            // 右弯之前 LW<70 就被提前压成 USED=6，导致 RAW 很大也转不过去；
            // 因此右弯只在视觉极差时才压成轻微右修。
            if (used_err <= -HARD_LEFT_ERR &&
                (line.used_rows < 18 || line.line_width < 70))
            {
                used_err = -6;
                left_hold = 0;
                right_hold = 0;
            }
            else if (used_err >= HARD_RIGHT_ERR &&
                     (line.used_rows < 12 || line.line_width < 24))
            {
                used_err = 6;
                left_hold = 0;
                right_hold = 0;
            }

            // ===== right-turn low-confidence guard =====
            // 之前 LW<70 就把右弯压成 USED=6，导致很多右弯明明 RAW 很大但转不过去。
            // 现在只在视觉极差时轻微右修；LW=40~60 的右弯允许正常转。
            if (used_err > 0 && (line.used_rows < 12 || line.line_width < 24))
            {
                used_err = 6;
                right_hold = 0;
            }

            if (used_err > RIGHT_USED_CAP)
            {
                used_err = RIGHT_USED_CAP;
                right_hold = 0;
            }

            // ===== 闭环循迹目标速度计算 =====
            if (used_err <= -HARD_LEFT_ERR)
            {
                // 强左弯：左轮目标 0，右轮目标高
                // 注意：这里不是开环 PWM，仍然是编码器速度闭环
                mode = 2;
                lt = HARD_INNER_TARGET;
                rt = HARD_LEFT_OUTER_TARGET;
            }
            else if (used_err >= HARD_RIGHT_ERR)
            {
                // 强右弯：左轮目标高，右轮目标 0
                mode = 3;
                lt = (line.used_rows < LOW_CONF_BRAKE_ROWS || line.line_width < LOW_CONF_BRAKE_LW)
                         ? HARD_RIGHT_SLOW_OUTER
                         : HARD_OUTER_TARGET;
                rt = HARD_INNER_TARGET;
            }
            else
            {
                mode = 1;

                int abs_err = abs_i(used_err);
                int gain = GAIN_SMALL;

                if (abs_err >= 10)
                {
                    gain = GAIN_BIG;
                }
                else if (abs_err >= 5)
                {
                    gain = GAIN_MID;
                }
                else
                {
                    gain = GAIN_SMALL;
                }

                turn = used_err * gain;
                if (used_err < 0)
                {
                    turn += used_err * LEFT_GAIN_BONUS;
                }
                else if (used_err > 0)
                {
                    turn += used_err * RIGHT_GAIN_BONUS;
                }
                turn = clamp_i(turn, -TURN_LIMIT, TURN_LIMIT);

                lt = LEFT_BASE + turn;
                rt = RIGHT_BASE - turn;

                // 右弯 used_err>0：左轮是外侧轮，不允许暴涨；
                // 左弯 used_err<0：右轮是外侧轮，不允许暴涨。
                // 内侧轮仍然允许明显降速，用差速来转弯。
                if (used_err > 0)
                {
                    lt = clamp_i(lt, TARGET_MIN, LEFT_BASE + OUTER_BOOST_CAP);
                }
                else if (used_err < 0)
                {
                    rt = clamp_i(rt, TARGET_MIN, RIGHT_BASE + OUTER_BOOST_CAP);
                }

                // 右弯明确时，右内侧轮再压低一点，补偿右转弱。
                // 不提高左外侧轮，避免进弯速度继续变快。
                if (used_err >= RIGHT_PIVOT_TRIGGER &&
                    line.used_rows >= RIGHT_PIVOT_MIN_ROWS &&
                    line.line_width >= RIGHT_PIVOT_MIN_LW)
                {
                    lt = (line.used_rows < LOW_CONF_BRAKE_ROWS || line.line_width < LOW_CONF_BRAKE_LW)
                             ? RIGHT_PIVOT_SLOW_OUTER
                             : RIGHT_PIVOT_OUTER;
                    rt = RIGHT_PIVOT_INNER;
                    mode = 3;
                }

                int curve_brake = abs_err * CURVE_BRAKE_GAIN;
                if (used_err > 0)
                {
                    curve_brake += abs_err * RIGHT_CURVE_BRAKE_EXTRA_GAIN;
                }
                if (curve_brake > CURVE_BRAKE_MAX)
                {
                    curve_brake = CURVE_BRAKE_MAX;
                }

                int low_conf_brake = 0;
                if (line.used_rows < LOW_CONF_BRAKE_ROWS)
                {
                    low_conf_brake += (LOW_CONF_BRAKE_ROWS - line.used_rows) * LOW_CONF_BRAKE_GAIN;
                }
                if (line.line_width < LOW_CONF_BRAKE_LW)
                {
                    low_conf_brake += (LOW_CONF_BRAKE_LW - line.line_width) * LOW_CONF_BRAKE_GAIN / 2;
                }
                if (low_conf_brake > LOW_CONF_BRAKE_MAX)
                {
                    low_conf_brake = LOW_CONF_BRAKE_MAX;
                }

                lt -= (curve_brake + low_conf_brake);
                rt -= (curve_brake + low_conf_brake);

                lt = clamp_i(lt, TARGET_MIN, TARGET_MAX);
                rt = clamp_i(rt, TARGET_MIN, TARGET_MAX);
            }


            // ===== 关键：闭环速度控制 =====
            speed_control_set_target(lt, rt);
            speed_control_update();
        }
        else
        {
            lost_count++;
            filter_inited = 0;
            filtered_err = 0;
            left_hold = 0;
            right_hold = 0;
            mode = 0;

            if (lost_count >= 3)
            {
                speed_control_set_target(0, 0);
                speed_control_update();
                motor_stop();
            }
            else
            {
                // 短暂丢线时闭环慢速直行，不立即急停
                speed_control_set_target(700, 900);
                speed_control_update();
            }
        }

        if ((frame % 5) == 0)
        {
            display_line(0, "GROUND V10C");
            display_value(1, "V", line.valid);
            display_value(2, "CX", line.center_x);
            display_value(3, "RAW", raw_err);
            display_value(4, "USED", used_err);
            display_value(5, "MODE", mode);
            display_value(6, "LT", lt);
            display_value(7, "RT", rt);
            display_value(8, "LH", left_hold);
            display_value(9, "LEG", left_exit_guard);

            int ml = speed_control_get_left_measured();
            int mr = speed_control_get_right_measured();

            printf("[ground10c] frame=%d mode=%d V=%d lost=%d CX=%d RAW=%d USED=%d LH=%d RH=%d LEG=%d ROWS=%d LW=%d LT=%d RT=%d ML=%d MR=%d\n",
                   frame, mode, line.valid, lost_count,
                   line.center_x, raw_err, used_err,
                   left_hold, right_hold, left_exit_guard,
                   line.used_rows, line.line_width,
                   lt, rt, ml, mr);
        }

        // 连续保存调试帧：相当于低帧率摄像头录像。
        // 每 2 帧保存一张，最多保存 1200 张，避免写满板子。
        if ((frame % 2 == 0) && frame < 1200)
        {
            // Disabled while SeekFree Assistant realtime preview is enabled.
            // Disk PPM dumping competes with camera/line-follow/TCP image sending.
            // save_line_debug_ppm(frame, line, raw_err, used_err);
        }

        frame++;
        delay_ms(40);
    }

    speed_control_stop();
    motor_stop();

    assistant_tcp_close();
    printf("[main] exit.\n");
    return 0;
}
