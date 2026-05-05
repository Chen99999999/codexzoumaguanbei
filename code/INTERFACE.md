# 模块中间接口说明

## 目标
视觉模块负责输出识别结果。
控制模块负责读取识别结果并执行动作。

视觉模块不直接控制电机。
控制模块不关心视觉内部具体如何实现。

---

## 当前统一数据结构（建议）
```cpp
enum class TargetClass
{
    UNKNOWN = 0,
    WEAPON  = 1,
    SUPPLY  = 2,
    VEHICLE = 3
};

struct VisionResult
{
    bool red_marker_found;   // 是否找到红色引导块
    bool target_valid;       // 是否满足识别触发条件
    TargetClass target_class;
    int roi_center_x;        // ROI中心横坐标
    int roi_center_y;        // ROI中心纵坐标
    float confidence;        // 识别置信度
};