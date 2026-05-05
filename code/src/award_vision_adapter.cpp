#include "award_vision_adapter.hpp"

#include <string.h>

#include "image.hpp"

AwardGrayImage Gray_image;

static int g_award_vision_inited = 0;

static void award_vision_reset_state(void)
{
    memset(&ImageStatus, 0, sizeof(ImageStatus));
    memset(&ImageData, 0, sizeof(ImageData));
    memset(&SystemData, 0, sizeof(SystemData));
    memset(&ImageFlag, 0, sizeof(ImageFlag));
    memset(ImageDeal, 0, sizeof(ImageDeal));
    memset(&Gray_image, 0, sizeof(Gray_image));

    Gray_image.rows = 60;
    Gray_image.cols = 80;

    ImageStatus.MiddleLine = 45;
    ImageStatus.TowPoint_Gain = 0.2f;
    ImageStatus.TowPoint_Offset_Max = 5;
    ImageStatus.TowPoint_Offset_Min = -2;
    ImageStatus.TowPointAdjust_v = 160;
    ImageStatus.Det_all_k = 0.7f;
    ImageStatus.CirquePass = 'F';
    ImageStatus.IsCinqueOutIn = 'F';
    ImageStatus.CirqueOut = 'F';
    ImageStatus.CirqueOff = 'F';
    ImageStatus.Barn_Flag = 0;
    ImageStatus.straight_acc = 0;
    ImageStatus.TowPoint = 26;
    ImageStatus.OFFLine = 8;
    ImageStatus.Threshold_static = 70;
    ImageStatus.Threshold_detach = 180;
    ImageStatus.variance_acc = 25;
    ImageStatus.newblue_flag = 0;

    ImageScanInterval = 2;
    ImageScanInterval_Cross = 2;
    SystemData.Stop = 1;
}

void award_vision_init(void)
{
    award_vision_reset_state();
    g_award_vision_inited = 1;
}

static void award_vision_downsample_2x2(const uint8_t *gray, int width, int height)
{
    for (int y = 0; y < Gray_image.rows; ++y)
    {
        const int src_y = y * height / Gray_image.rows;
        const int src_y2 = (src_y + 1 < height) ? (src_y + 1) : src_y;

        for (int x = 0; x < Gray_image.cols; ++x)
        {
            const int src_x = x * width / Gray_image.cols;
            const int src_x2 = (src_x + 1 < width) ? (src_x + 1) : src_x;

            const int p00 = gray[src_y * width + src_x];
            const int p01 = gray[src_y * width + src_x2];
            const int p10 = gray[src_y2 * width + src_x];
            const int p11 = gray[src_y2 * width + src_x2];

            Gray_image.pixels[y][x] = (uint8_t)((p00 + p01 + p10 + p11) / 4);
        }
    }
}

bool award_vision_process_from_gray(const uint8_t *gray, int width, int height, AwardSignal *signal)
{
    if (!g_award_vision_inited)
    {
        award_vision_init();
    }

    if (!gray || width <= 0 || height <= 0)
    {
        return false;
    }

    award_vision_downsample_2x2(gray, width, height);
    ImageProcess();

    if (signal)
    {
        award_signal_capture(signal);
    }

    return true;
}
