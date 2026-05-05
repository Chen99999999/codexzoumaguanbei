#include "smartcar_line.hpp"
#include "smartcar_camera.hpp"

#include <stdio.h>

#define DW 80
#define DH 60

#define BOTTOM_Y      58
#define TOP_Y         8
#define IMAGE_MID     40
#define SCAN_INTERVAL 8
#define MIN_WIDTH     8
#define MAX_WIDTH     78
#define NEAR_Y_MIN    46
#define FAR_Y_MAX     28

static unsigned char g_gray80[DH][DW];
static unsigned char g_bin[DH][DW];
static smartcar_line_result_t g_line;
static int g_prev_valid = 0;
static int g_prev_bottom_left = 0;
static int g_prev_bottom_right = 0;
static int g_prev_bottom_center = IMAGE_MID;
static int g_prev_width = 32;

static int clamp_i(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int abs_i(int x)
{
    return x < 0 ? -x : x;
}

static void clear_line_result(void)
{
    g_line.valid = 0;
    g_line.center_x = 0;
    g_line.error = 0;
    g_line.raw_center_x = 0;
    g_line.near_center_x = 0;
    g_line.far_center_x = 0;
    g_line.preview_center_x = 0;
    g_line.trend = 0;
    g_line.left_x = 0;
    g_line.right_x = 0;
    g_line.row_y = 0;
    g_line.near_row_y = 0;
    g_line.far_row_y = 0;
    g_line.threshold = 0;
    g_line.width = 0;
    g_line.height = 0;
    g_line.used_rows = 0;
    g_line.line_width = 0;
    g_line.near_width = 0;
    g_line.far_width = 0;
    g_line.left_found_rows = 0;
    g_line.right_found_rows = 0;
    g_line.debug_rows = 0;
    for (int i = 0; i < SMARTCAR_LINE_DEBUG_ROWS; i++)
    {
        g_line.debug_left_x[i] = 0;
        g_line.debug_right_x[i] = 0;
        g_line.debug_center_x[i] = 0;
        g_line.debug_valid_row[i] = 0;
    }
}

static void downsample_to_80x60(const unsigned char *src, int sw, int sh)
{
    for (int y = 0; y < DH; y++)
    {
        int sy = y * sh / DH;
        int sy2 = sy + 1;
        if (sy2 >= sh) sy2 = sh - 1;

        for (int x = 0; x < DW; x++)
        {
            int sx = x * sw / DW;
            int sx2 = sx + 1;
            if (sx2 >= sw) sx2 = sw - 1;

            int v = src[sy * sw + sx]
                  + src[sy * sw + sx2]
                  + src[sy2 * sw + sx]
                  + src[sy2 * sw + sx2];

            g_gray80[y][x] = (unsigned char)(v / 4);
        }
    }
}

static int otsu_threshold_80x60(void)
{
    int hist[256];
    for (int i = 0; i < 256; i++) hist[i] = 0;

    const int total = DW * DH;
    long sum_all = 0;

    for (int y = 0; y < DH; y++)
    {
        for (int x = 0; x < DW; x++)
        {
            int v = g_gray80[y][x];
            hist[v]++;
            sum_all += v;
        }
    }

    long sum_b = 0;
    int w_b = 0;
    double max_var = -1.0;
    int threshold = 100;

    for (int t = 0; t < 255; t++)
    {
        w_b += hist[t];
        if (w_b == 0) continue;

        int w_f = total - w_b;
        if (w_f == 0) break;

        sum_b += (long)t * hist[t];

        double m_b = (double)sum_b / (double)w_b;
        double m_f = (double)(sum_all - sum_b) / (double)w_f;
        double diff = m_b - m_f;
        double var_between = (double)w_b * (double)w_f * diff * diff;

        if (var_between > max_var)
        {
            max_var = var_between;
            threshold = t;
        }
    }

    if (threshold < 55) threshold = 55;
    if (threshold > 210) threshold = 210;
    return threshold;
}

static void binarize_80x60(int threshold)
{
    for (int y = 0; y < DH; y++)
    {
        for (int x = 0; x < DW; x++)
        {
            g_bin[y][x] = (g_gray80[y][x] > threshold) ? 1 : 0;
        }
    }

    for (int y = 2; y < DH - 2; y++)
    {
        for (int x = 2; x < DW - 2; x++)
        {
            if (g_bin[y][x] == 0)
            {
                int cnt = g_bin[y - 1][x]
                        + g_bin[y + 1][x]
                        + g_bin[y][x - 1]
                        + g_bin[y][x + 1];

                if (cnt >= 3)
                {
                    g_bin[y][x] = 1;
                }
            }
        }
    }

    for (int x = 0; x < DW; x++)
    {
        g_bin[0][x] = 0;
        g_bin[DH - 1][x] = 0;
    }

    for (int y = 0; y < DH; y++)
    {
        g_bin[y][0] = 0;
        g_bin[y][DW - 1] = 0;
    }
}

static int find_bottom_edges(int *left, int *right, int *center)
{
    const int y = BOTTOM_Y;
    int seed = -1;
    int seed_center = IMAGE_MID;

    if (g_prev_valid)
    {
        seed_center = clamp_i(g_prev_bottom_center, 2, DW - 3);

        for (int d = 0; d <= 24; d++)
        {
            int xl = seed_center - d;
            int xr = seed_center + d;

            if (xl > 1 && g_bin[y][xl])
            {
                seed = xl;
                break;
            }

            if (xr < DW - 2 && g_bin[y][xr])
            {
                seed = xr;
                break;
            }
        }
    }

    for (int d = 0; seed < 0 && d < IMAGE_MID; d++)
    {
        int xl = IMAGE_MID - d;
        int xr = IMAGE_MID + d;

        if (xl > 1 && g_bin[y][xl])
        {
            seed = xl;
            break;
        }

        if (xr < DW - 2 && g_bin[y][xr])
        {
            seed = xr;
            break;
        }
    }

    if (seed < 0)
    {
        return 0;
    }

    int l = seed;
    while (l > 1 && g_bin[y][l - 1])
    {
        l--;
    }

    int r = seed;
    while (r < DW - 2 && g_bin[y][r + 1])
    {
        r++;
    }

    int width = r - l;
    if (width < MIN_WIDTH || width > MAX_WIDTH)
    {
        return 0;
    }

    if (g_prev_valid)
    {
        int prev_width = g_prev_bottom_right - g_prev_bottom_left;
        if (prev_width >= MIN_WIDTH && prev_width <= MAX_WIDTH)
        {
            int width_diff = abs_i(width - prev_width);
            int center_shift = abs_i(((l + r) / 2) - g_prev_bottom_center);
            if (width_diff > 28 && center_shift > 18)
            {
                return 0;
            }
        }
    }

    *left = l;
    *right = r;
    *center = (l + r) / 2;
    return 1;
}

static int search_left_near_prev(int y, int prev_left, int *out_left)
{
    int low = clamp_i(prev_left - SCAN_INTERVAL, 1, DW - 2);
    int high = clamp_i(prev_left + SCAN_INTERVAL, 1, DW - 2);

    for (int x = high; x >= low + 1; x--)
    {
        if (g_bin[y][x] == 1 && g_bin[y][x - 1] == 0)
        {
            *out_left = x;
            return 1;
        }
    }

    int mid = (low + high) / 2;
    if (g_bin[y][mid])
    {
        *out_left = prev_left;
        return 2;
    }

    *out_left = prev_left;
    return 0;
}

static int search_right_near_prev(int y, int prev_right, int *out_right)
{
    int low = clamp_i(prev_right - SCAN_INTERVAL, 1, DW - 2);
    int high = clamp_i(prev_right + SCAN_INTERVAL, 1, DW - 2);

    for (int x = low; x <= high - 1; x++)
    {
        if (g_bin[y][x] == 1 && g_bin[y][x + 1] == 0)
        {
            *out_right = x;
            return 1;
        }
    }

    int mid = (low + high) / 2;
    if (g_bin[y][mid])
    {
        *out_right = prev_right;
        return 2;
    }

    *out_right = prev_right;
    return 0;
}

void line_init(void)
{
    clear_line_result();
    g_prev_valid = 0;
    g_prev_bottom_left = 0;
    g_prev_bottom_right = 0;
    g_prev_bottom_center = IMAGE_MID;
    g_prev_width = 32;
}

int line_update(void)
{
    clear_line_result();

    const unsigned char *gray = (const unsigned char *)camera_get_gray();
    const int sw = camera_get_width();
    const int sh = camera_get_height();

    g_line.width = sw;
    g_line.height = sh;

    if (!gray || sw <= 0 || sh <= 0)
    {
        return 0;
    }

    downsample_to_80x60(gray, sw, sh);

    int threshold = otsu_threshold_80x60();
    binarize_80x60(threshold);
    g_line.threshold = threshold;

    int left = 0;
    int right = 0;
    int center = 0;
    if (!find_bottom_edges(&left, &right, &center))
    {
        g_prev_valid = 0;
        return 0;
    }

    int centers[DH];
    int lefts[DH];
    int rights[DH];
    int valid_row[DH];

    for (int i = 0; i < DH; i++)
    {
        centers[i] = 0;
        lefts[i] = 0;
        rights[i] = DW - 1;
        valid_row[i] = 0;
    }

    centers[BOTTOM_Y] = center;
    lefts[BOTTOM_Y] = left;
    rights[BOTTOM_Y] = right;
    valid_row[BOTTOM_Y] = 1;

    int prev_left = left;
    int prev_right = right;
    int last_center = center;
    int base_width = right - left;

    int used = 1;
    int off_line = TOP_Y;
    int left_found_rows = 1;
    int right_found_rows = 1;

    for (int y = BOTTOM_Y - 1; y >= TOP_Y; y--)
    {
        int l = prev_left;
        int r = prev_right;

        int lf = search_left_near_prev(y, prev_left, &l);
        int rf = search_right_near_prev(y, prev_right, &r);

        if (lf != 0) left_found_rows++;
        if (rf != 0) right_found_rows++;

        if (lf == 0 && rf == 0)
        {
            off_line = y + 1;
            break;
        }

        int prev_width = prev_right - prev_left;
        if (prev_width < MIN_WIDTH || prev_width > MAX_WIDTH)
        {
            prev_width = base_width;
        }

        if (lf == 0 && rf != 0)
        {
            l = r - prev_width;
        }
        else if (rf == 0 && lf != 0)
        {
            r = l + prev_width;
        }

        l = clamp_i(l, 1, DW - 2);
        r = clamp_i(r, 1, DW - 2);

        int width_row = r - l;
        if (width_row < MIN_WIDTH || width_row > MAX_WIDTH)
        {
            off_line = y + 1;
            break;
        }

        int c = (l + r) / 2;
        c = (last_center + 2 * c) / 3;

        centers[y] = c;
        lefts[y] = l;
        rights[y] = r;
        valid_row[y] = 1;

        prev_left = l;
        prev_right = r;
        last_center = c;
        used++;
    }

    g_line.used_rows = used;
    if (used < 4)
    {
        return 0;
    }

    long sum = 0;
    long wsum = 0;
    long near_sum = 0;
    long near_wsum = 0;
    long far_sum = 0;
    long far_wsum = 0;
    long near_width_sum = 0;
    long near_width_count = 0;
    long far_width_sum = 0;
    long far_width_count = 0;

    for (int y = BOTTOM_Y; y >= off_line && y >= TOP_Y; y--)
    {
        if (!valid_row[y]) continue;

        int weight = 1 + (y - TOP_Y) / 6;
        sum += centers[y] * weight;
        wsum += weight;

        int width_row = rights[y] - lefts[y];
        if (y >= NEAR_Y_MIN)
        {
            int near_weight = 1 + (y - NEAR_Y_MIN);
            near_sum += centers[y] * near_weight;
            near_wsum += near_weight;
            near_width_sum += width_row;
            near_width_count++;
        }
        else if (y <= FAR_Y_MAX)
        {
            int far_weight = 1 + (FAR_Y_MAX - y);
            far_sum += centers[y] * far_weight;
            far_wsum += far_weight;
            far_width_sum += width_row;
            far_width_count++;
        }
    }

    if (wsum <= 0)
    {
        return 0;
    }

    int raw_center80 = clamp_i((int)(sum / wsum), 0, DW - 1);
    int near_center80 = raw_center80;
    int far_center80 = raw_center80;
    int near_width80 = rights[BOTTOM_Y] - lefts[BOTTOM_Y];
    int far_width80 = near_width80;

    if (near_wsum > 0)
    {
        near_center80 = clamp_i((int)(near_sum / near_wsum), 0, DW - 1);
    }

    if (far_wsum > 0)
    {
        far_center80 = clamp_i((int)(far_sum / far_wsum), 0, DW - 1);
    }

    if (near_width_count > 0)
    {
        near_width80 = (int)(near_width_sum / near_width_count);
    }

    if (far_width_count > 0)
    {
        far_width80 = (int)(far_width_sum / far_width_count);
    }

    int trend80 = far_center80 - near_center80;
    int preview_center80 = raw_center80;

    if (near_wsum > 0 && far_wsum > 0)
    {
        if (abs_i(trend80) >= 3)
        {
            preview_center80 = (near_center80 * 2 + far_center80 * 3) / 5;
            preview_center80 += trend80 / 2;
        }
        else
        {
            preview_center80 = (near_center80 * 3 + far_center80 * 2) / 5;
        }
    }

    if (trend80 <= -5 && far_width80 < near_width80 - 4)
    {
        preview_center80 -= 2;
    }
    else if (trend80 >= 5 && far_width80 < near_width80 - 4)
    {
        preview_center80 += 2;
    }

    preview_center80 = clamp_i(preview_center80, 0, DW - 1);

    g_line.valid = 1;
    g_line.raw_center_x = raw_center80 * 2 + 1;
    g_line.near_center_x = near_center80 * 2 + 1;
    g_line.far_center_x = far_center80 * 2 + 1;
    g_line.preview_center_x = preview_center80 * 2 + 1;
    g_line.center_x = g_line.preview_center_x;
    g_line.error = g_line.center_x - sw / 2;
    g_line.trend = trend80 * 2;
    g_line.left_x = lefts[BOTTOM_Y] * 2;
    g_line.right_x = rights[BOTTOM_Y] * 2;
    g_line.row_y = BOTTOM_Y * 2;
    g_line.near_row_y = NEAR_Y_MIN * 2;
    g_line.far_row_y = FAR_Y_MAX * 2;
    g_line.line_width = (rights[BOTTOM_Y] - lefts[BOTTOM_Y]) * 2;
    g_line.near_width = near_width80 * 2;
    g_line.far_width = far_width80 * 2;
    g_line.left_found_rows = left_found_rows;
    g_line.right_found_rows = right_found_rows;
    g_line.debug_rows = DH;
    for (int y = 0; y < DH && y < SMARTCAR_LINE_DEBUG_ROWS; y++)
    {
        g_line.debug_left_x[y] = lefts[y] * 2;
        g_line.debug_right_x[y] = rights[y] * 2;
        g_line.debug_center_x[y] = centers[y] * 2 + 1;
        g_line.debug_valid_row[y] = valid_row[y];
    }

    g_prev_valid = 1;
    g_prev_bottom_left = left;
    g_prev_bottom_right = right;
    g_prev_bottom_center = center;
    g_prev_width = right - left;

    return 1;
}

smartcar_line_result_t line_get_result(void)
{
    return g_line;
}

int line_is_valid(void)
{
    return g_line.valid;
}

int line_get_center_x(void)
{
    return g_line.center_x;
}

int line_get_error(void)
{
    return g_line.error;
}

void line_print(void)
{
    printf("[line] valid=%d center=%d raw=%d near=%d far=%d trend=%d error=%d used=%d row=%d L=%d R=%d width=%d nearW=%d farW=%d LF=%d RF=%d thr=%d img=%dx%d\n",
           g_line.valid,
           g_line.center_x,
           g_line.raw_center_x,
           g_line.near_center_x,
           g_line.far_center_x,
           g_line.trend,
           g_line.error,
           g_line.used_rows,
           g_line.row_y,
           g_line.left_x,
           g_line.right_x,
           g_line.line_width,
           g_line.near_width,
           g_line.far_width,
           g_line.left_found_rows,
           g_line.right_found_rows,
           g_line.threshold,
           g_line.width,
           g_line.height);
}
