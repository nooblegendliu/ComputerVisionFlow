# 工业定位演示插件

`DemoWaferLocator` 已作为独立 `.fp` 接入构建，组件名为 `Demo_Wafer_Locator`，由 Studio 的插件部署步骤复制到 `Plugins`。它是原 FlowCV 71 项基准之外的演示扩展，尚无独立的实际样本精度验收。

## 接口与使用

| 方向 | 端口 | 类型 | 内容 |
| --- | --- | --- | --- |
| 输入 | `in` | CvMat | 非空 8 位灰度、BGR 或 BGRA 图像 |
| 输出 | `out` | CvMat | 灰度转 BGR 的显示图，可绘制四边形、中心和角度 |
| 输出 | `pose` | JSON | `data_type=wafer_pose`、`success`、`status`、`unit=pixel`、`ref_frame`；成功时包含 center、angle_deg、corners、area_px2 |

可手动搭建 `Image_Loader → Demo_Wafer_Locator → Viewer`，并把 `pose` 接到 `Json_Viewer`。在 Properties 调整暗/亮目标、面积门槛、边缘筛选比例、拟合容差和结果绘制。定位使用连通域和边线拟合；参数需要按实际输入核对。

缺失或空输入不产生输出；禁用时返回显示图和 `success=false, status=disabled`。计算异常返回 `success=false` 与错误 status，不提供成功坐标。消费者应检查本次输出和 success，不复用上次定位结果。

## 样本

原仓库根目录的两张图片已归入 `samples/`：

- [图像示例](samples/test_cast_.png)。
- [二值图示例](samples/test_case_bin.png)。

这两张图是现有演示资料，没有独立标注真值、允许误差或经过确认的算法期望结果；保存样本不代表测试通过。

## 当前边界

插件不包含完整工业流程、真实相机触发、标定、设备通信、run/session 身份、条件分支调度、结果去重或超时协调。输出为像素位置和图像角度，不能直接作为设备运动补偿量。后续工业视觉流程按 [演进路线](../../docs/plans/industrial-vision-roadmap.md) 规划。
