#ifndef SMARTCAR_LINE_HPP
#define SMARTCAR_LINE_HPP

#define SMARTCAR_LINE_DEBUG_ROWS 60

typedef struct
{
    int valid;
    int center_x;
    int error;
    int raw_center_x;
    int near_center_x;
    int far_center_x;
    int preview_center_x;
    int trend;

    int left_x;
    int right_x;
    int row_y;
    int near_row_y;
    int far_row_y;

    int threshold;
    int width;
    int height;

    int used_rows;
    int line_width;
    int near_width;
    int far_width;
    int left_found_rows;
    int right_found_rows;

    int debug_rows;
    int debug_left_x[SMARTCAR_LINE_DEBUG_ROWS];
    int debug_right_x[SMARTCAR_LINE_DEBUG_ROWS];
    int debug_center_x[SMARTCAR_LINE_DEBUG_ROWS];
    int debug_valid_row[SMARTCAR_LINE_DEBUG_ROWS];
} smartcar_line_result_t;

void line_init(void);
int line_update(void);
smartcar_line_result_t line_get_result(void);

int line_is_valid(void);
int line_get_center_x(void);
int line_get_error(void);

void line_print(void);

#endif
