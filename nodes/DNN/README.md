# DNN 内部节点接入说明

本目录迁移自本机 FlowCV `b09c48e` 的 `Internal_Nodes/DNN`。计算代码逐节点保留原预处理、`forward`／`detect`／`recognize`、后处理及状态键；Qt Controls 位于 `app/VisionFlowStudio/DnnControls.cpp`，计算节点及 SDK 不依赖 Qt。

**验证状态：** 2026-10-19，Release 构建中的 `stage6_dnn` 专项通过（1/1，0.39 秒），执行了七类节点的确定性 CPU 推理、状态及 Qt Controls 检查。全项目整体验收仍由公共验收记录汇总；该结果不代替真实模型或 GPU 验证。

## 端口与默认参数

所有节点分类为 `Category_DNN`，作者 `Richard`，版本 `0.1.0`，默认启用、执行顺序 `OutOfOrder`。除 Text_Recognition 外都只有图像输入 `in1`；除 Image_Processing 外都输出图像 `out` 和结构化结果 `json`。

| 节点 / C++ 类 | 输入 | 输出 | 主要默认参数 |
| --- | --- | --- | --- |
| Classification / `Classification` | `in1: CvMat` | `out: CvMat`, `json: JSON` | 224×224，scale=1，mean/std_dev=[0,0,0]，conf_thresh=0.5，RGB/Softmax/Crop/Preprocess Resize/Draw Class 均关闭 |
| Object_Detection / `ObjectDetection` | `in1: CvMat` | `out: CvMat`, `json: JSON` | 416×416，scale=1，mean=[0,0,0]，conf_thresh=0.5，NMS=0.4，RGB 开启，Crop/Preprocess Resize/Draw Class 关闭 |
| Segmentation / `Segmentation` | `in1: CvMat` | `out: CvMat`, `json: JSON` | 224×224，scale=1，mean=[0,0,0]，RGB/Preprocess Resize 关闭 |
| Human_Pose / `HumanPose` | `in1: CvMat` | `out: CvMat`, `json: JSON` | 416×416，scale=1，mean=[0,0,0]，conf_thresh=0.5，COCO，RGB 开启，强制单人/Preprocess Resize/Draw Joints 关闭 |
| Text_Detection / `TextDetection` | `in1: CvMat` | `out: CvMat`, `json: JSON` | 736×736，scale=1/255，mean≈[122.67891434,116.66876762,104.00698793]，binary=0.3，polygon=0.5，unclip=2，max_candidates=200，RGB 开启，Preprocess Resize/Draw Detections 关闭 |
| Text_Recognition / `TextRecognition` | `in1: CvMat`, `json: JSON` | `out: CvMat`, `json: JSON` | 100×32，scale=1/127.5，mean=[127.5,127.5,127.5]，RGB 开启，Preprocess Resize/Draw Detections 关闭，CTC-greedy |
| Image_Processing / `ImageProcessing` | `in1: CvMat` | `out: CvMat` | 224×224，scale=1，mean=[0,0,0]，RGB 关闭，Style Transfer |

模型及辅助文件路径默认均为空；后端默认 `DNN_BACKEND_DEFAULT`，目标默认 `DNN_TARGET_CPU`。Classification/Object_Detection 的标注默认位置为 (10,40)，Text_Recognition 为 (0,0)；文本 scale=0.5、thickness=2、颜色 RGB=(0,1,0)。Text_Detection 边框默认厚度 2、同样为绿色。原库仅有 `HERSHEY_SIMPLEX` 标注字体，不新增字体参数。

## 模型和结果契约

### Classification

需要模型和逐行类别文件。先按原选项执行预缩放，再 `blobFromImage`，保留 scale、mean、swap_rb、crop；仅三个标准差都非零时才执行原 `cv::divide`。输出必须是可展平为类别分数的 `CV_32F` 张量，可选稳定 Softmax，再选最大值。

类别编号须能对应类别文件中的条目。满足 `confidence >= conf_thresh` 时产生 `data:[{"class":...,"conf":...}]`；无符合结果时省略 `data`。结果类型为 `classification`，`ref_frame` 保存原图宽高。可选标注绘制在图像副本上。

### Object_Detection

保持原库按输出层类型分支的支持范围：

- `Region`：逐行读取归一化中心、宽高及类别分数；选择最大类别分数、按原图尺寸恢复框，并执行 NMS。
- `DetectionOutput`：每条 7 个浮点数，保留绝对／归一化框判断及去除背景类后编号减一的逻辑。多输出层时另做 NMS。
- 带 `im_info` 输入的 Faster-RCNN/R-FCN 路径仍保留尺寸输入。

这不意味着任意现代 YOLO ONNX 输出布局都可直接使用；原库未知输出层类型分支仍停止本次输出，不增加新的 YOLO 解码器。类别文件可为空，空时仍输出 confidence/bbox。结果为 `data_type="objects"`，条目含可选 `class`、`conf`、`bbox:{x,y,w,h}`；无检测时省略 `data`。原框和文字背景绘制逻辑保留。

### Segmentation

模型输出为 `CV_32F` 的 `[1,C,H,W]` 分数图；本实现保留原 8 位类别索引，支持 1～256 类。逐像素取最大分数类别，以类别颜色表生成 BGR 图，再使用最近邻恢复原图尺寸。类别文件逐行读取；可选颜色文件每行是三个以空格分开的 B、G、R 值。未提供颜色表时保留原随机颜色生成逻辑。

输出图像是分割着色图。`data_type="segmentation"`；颜色数和类别名数一致时，`data` 是完整类别颜色图例，条目为 `name` 和 `color:{b,g,r}`，不是检测框或像素计数。

### Human_Pose

保留 COCO、MPII、Hand 三种关键点名称和连线表，以及原 Concat／双层输出解释逻辑。网络具有 Concat 层时取 `forward()` 并分解 NCHW 通道；否则沿用原库对两层输出的获取与拼接。输出通道须容纳所选数据集关键点。

保留 PAF 多人配对、单人热图峰值、坐标换算和关节绘制。输出通道不足 57 时按原库强制单人模式。`force_single` 控件按 PAF 可用性显示。结果为 `data_type="poses"`，`data` 含 `key_count` 及按原关键点名字组织的 `keypoints`；低置信度或原边界规则排除的位置为 (-1,-1)。

### Text_Detection

使用 OpenCV `TextDetectionModel_DB`，保留 DB 阈值、polygon threshold、unclip ratio、候选数限制及预处理。所选模型须符合 OpenCV DB 接口的输出要求；本机 OpenCV 4.12 的 DB 后处理接收二维概率图。

输出 `data_type="objects"`，每条 `points` 是四边形顶点 `{x,y}` 数组。开启预缩放时按原逻辑将顶点换算回原图。可选边框绘制后恢复显示图尺寸。

### Text_Recognition

使用 `TextRecognitionModel`、逐行 vocabulary 和 `CTC-greedy`。输入 JSON 须为 `data_type="objects"` 且各条目含四个 `points`。沿用原四点透视变换，先裁剪为固定 100×32，再按模型输入参数识别。CTC 输出为 `[T,B,C]` 浮点序列，含 blank 类，词表顺序必须与模型一致。

输出 `data_type="text"`，条目包含识别 `text` 及四边形包围盒 `bbox`。原库没有缺少检测 JSON 时的整幅图识别回退；没有可用四边形时仍输出图像及不含 `data` 的结果头。不是通用任意 OCR 模型适配器。

### Image_Processing

原库实际提供的模式仅 `Style Transfer`，已保留完整推理路径。执行 `blobFromImage`，网络输出 `[1,3,H,W]` 浮点图；按原 CHW 顺序逐像素加回 mean、饱和转换为 8 位、重建 BGR 图。尺寸和模式在初始化时锁定，修改后须重新初始化。此节点没有 JSON 输出，也不增加原库没有的超分辨率等模式。

## 状态格式与 Qt Controls

所有节点公共键：`model_path`、`mean:[m0,m1,m2]`、`scale`、`model_res:{w,h}`、`swap_rb`、`backend`、`target`。`backend` 和 `target` 保存 OpenCV 枚举值，不保存下拉框索引。

| 节点 | 额外持久化键 |
| --- | --- |
| Classification | `config_path`、`classes_path`、`std_dev`、`crop`、`conf_thresh`、`preproc`、`softmax`，条件文本样式 |
| Object_Detection | `config_path`、`classes_path`、`crop`、`conf_thresh`、`preproc`，条件文本样式 |
| Segmentation | `config_path`、`classes_path`、`colors_path`、`preproc` |
| Human_Pose | `config_path`、`conf_thresh`、`preproc`、`force_single`、`dataset_mode`、`draw_joints` |
| Text_Detection | `bin_thresh`、`poly_thresh`、`unclip_ratio`、`max_candidates`、`preproc`，条件边框样式 |
| Text_Recognition | `voc_path`、`preproc`，条件文本样式 |
| Image_Processing | `config_path` |

条件文本样式为 `txt_position:{x,y}`、`txt_scale`、`txt_thickness`、`txt_color:{R,G,B}`，仅在启用文字/类别标注时保存；恢复时通过 `txt_color` 是否出现恢复绘制开关。Text_Detection 同样通过 `bbox_color` 恢复绘制，开启时保存 `bbox_color:{R,G,B}` 与 `bbox_thickness`。Text_Detection/Text_Recognition 在对现有实例应用不含样式的部分状态时，保持原库“保留既有绘制开关”的语义；Qt checkbox 单独改变运行时开关，不能以删去样式字段替代关闭动作。不新增 `draw_class` 或 `draw_text` 序列化键。

Object_Detection 的 NMS 阈值和 Image_Processing 的模式，原库可通过界面修改但未写入状态；这里保持该行为。专属 Controls 快照中的 `_nms_thresh`、`_processing_mode`、`_draw` 仅服务界面，不进入 `GetState()`。类别/词表/调色板内容、网络对象、错误、初始化状态同样不进入工程状态。

面板提供原文件选择/清除、后端/目标、初始化、数值、RGB、模式、阈值、颜色和标注控制。Object_Detection 仅在 Preprocess Resize 开启时显示 Crop；关闭预缩放同时清 Crop，与原界面一致。标注关闭时隐藏样式，不扩展通用属性类型。

数值控件的箭头和滚轮沿用原 Drag 控件范围，例如模型尺寸 224–1000、阈值 0–1、均值/标准差 0–500、Scale 0–10。原库允许显式文本输入越界；这里同样保留文本输入以及工程恢复的范围外值，打开面板不会改写状态。Classification 原本不限制的文字尺寸/厚度也不额外限值。

## 初始化、执行与生命周期

路径、后端、目标或需要重新初始化的尺寸/模式改变后显示重初始化提示；普通参数更新不在每次控件修改时重载模型。点击 Initialize/Reinitialize 才按修改后的资源建立网络，工程状态恢复时有必要路径则自动初始化。Classification/Segmentation 需要类别文件，Text_Recognition 需要词表；模型自包含时配置路径可空。

禁用、尚未初始化或模型加载失败时，非空图像按原逻辑通过 `out`，不制造 JSON 检测结果。推理或后处理失败则本次不输出，错误由 Controls 状态显示；恢复有效输入/参数后下一次执行可继续。输入缺失/空图像维持原无输出行为。

同一实例用互斥锁串行保护网络初始化、推理、参数及界面快照。计算线程不访问 QWidget；Controls 只在 GUI 线程创建和操作，通过弱引用访问节点。宿主保持现有“先释放界面，再释放节点”的生命周期；没有独立推理线程、远程调用或插件隔离。

## 必要适配与修复记录

1. 将原 ImGui 控件替换为可选 Qt Controls；使用无 Qt 的 `DnnControlInterface` 和颜色值结构传递控件快照，不改计算 SDK 的 GUI 依赖边界。
2. 补齐状态/控件更新与推理之间的同步；原数值控件直接写成员的方式不适用于独立 Qt GUI 线程与 AutoTick 并发。
3. 初始化开始时先废弃旧网络，文件加载、模型初始化失败及恢复空模型路径不会继续把旧网络标记为已初始化。原部分辅助文件失败会提前返回而保留旧标志。
4. Classification、Object_Detection、Human_Pose 的初始化不再强制非空 `config_path`，使 OpenCV 本来支持的自包含 ONNX 模型可用；仍把实际 model/config 路径原样交给 OpenCV，不新增模型格式转换。
5. 保留原后端/目标探测和枚举语义；默认/OpenCV 后端无枚举结果时补 CPU 目标，恢复不可用后端/目标时更新对应列表，避免空列表及失配下标。
6. 增加调度边界异常报告、张量维度/数据类型检查、DetectionOutput 的 7 元组长度检查、Human_Pose 关键点索引检查、Text_Recognition 四点数检查，避免坏模型或 JSON 导致越界或中止执行。
7. Segmentation 颜色文件中的临时 RGB/BGR 向量初始化为零；保留原解析及着色顺序，避免不完整颜色行读取未初始化字节。
8. Image_Processing 明确要求原实现实际使用的三通道 NCHW 输出；Segmentation 明确原 8 位类别索引上限，拒绝越界布局而非重新定义算法。

未加入现代 YOLO 解码、自动猜测模型参数、远程模型下载、训练或新的状态键。

## 确定性测试与验收边界

`tests/generate_dnn_fixtures.py` 使用 Python 标准库编码最小 ONNX protobuf 和 Darknet 权重，无需 onnx/torch/numpy，也不访问网络。已提交夹具位于 `tests/fixtures/dnn`。测试调用真实 OpenCV CPU 推理接口，不替换节点推理或注入模拟结果。

`stage6_dnn_test` 接收该夹具目录作为第一个参数，测试代码覆盖：

- 分类 GlobalAveragePool/Flatten 的实际分数、Softmax、阈值、输入不变及加载失败恢复。
- Darknet 卷积/Region 实际推理、框坐标换算、NMS 路径、类别对应及原 Crop 控件联动。
- Identity 分数图的分割 argmax、颜色表、最近邻恢复和 JSON 图例。
- 卷积/Concat 热图的单姿态解码与坐标；坏数据集选项无输出。
- DB 概率图的检测四边形接入透视裁剪/CTC-greedy 网络，验证文字结果；畸形四边形安全失败。
- Style Transfer 的 CHW→BGR 以及 mean 减除/加回顺序。
- 七类默认状态、枚举回退，以及可选 Qt 控件的路径/绘制开关、步进边界、范围外状态显示和释放。

`STAGE6_DNN_QT_TEST` 启用专属 Qt Controls 检查；未定义该宏时只依赖计算节点。当前夹具不证明真实训练模型的精度、完整数据集兼容性、GPU/CUDA/OpenCL/OpenVINO 执行、PAF 多人效果或 SSD/Faster-RCNN 分支的实际模型输出。上述算法分支保留原源码，真实模型和可用硬件的补充验收需分别记录，不能以合成夹具推理代替。
