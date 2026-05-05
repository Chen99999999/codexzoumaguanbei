# A车赛道前检查清单

## 当前主线分支
official-motor-base

## 上电前
1. 车轮悬空
2. 电池电压确认
3. 驱动板电源线确认
4. 摄像头插好
5. 屏幕排线确认
6. 核心板能 SSH
7. 确认 RUN_MODE，不要误开电机循迹

## 当前推荐 RUN_MODE
去赛道前默认：
MODE_LINE_DEBUG

作用：
只采图，只算 center/error，不控车。

## 到赛道后的测试顺序

### 1. 采图
切换到 MODE_CAMERA_CAPTURE。
采集：
- 直道
- 左弯
- 右弯
- 车偏左
- 车偏右
- 阴影
- 强光
- 红色引导块
- 目标板附近

图片保存目录：
/home/root/capture/

### 2. 只看 error
切换到 MODE_LINE_DEBUG。

检查：
- 车头对准赛道中心时 error 接近 0
- 车头偏左/偏右时 error 正负有明显变化
- valid 稳定为 1
- used_rows 不为 0

### 3. 低速循迹
切换到 MODE_LINE_FOLLOW。
先保持 ENABLE_MOTOR_FOLLOW = 0，只看目标速度计算是否正常。
确认后再改为 ENABLE_MOTOR_FOLLOW = 1。

### 4. 安全原则
- valid=0 必须停车
- 连续丢帧必须停车
- error 过大必须停车
- Ctrl+C 必须停车
- 第一次落地测试 base 不要太大
