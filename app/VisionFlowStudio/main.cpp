#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <opencv2/core.hpp>

#include <flowcv_manager.h>
#include <Viewer/Viewer.hpp>

namespace
{
	// -------------------------------------------------------------------------
	// Demo Configuration
	// -------------------------------------------------------------------------
	constexpr int kWidth = 640;
	constexpr int kHeight = 480;

	constexpr int kSolidValueA = 30;
	constexpr int kSolidValueB = 40;

	constexpr double kExpectedValue = 70.0;

	// -------------------------------------------------------------------------
	//	Check Helper
	// -------------------------------------------------------------------------
	void Require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}

	// -------------------------------------------------------------------------
	//	Configure Solid
	void ConfigureSolid(FlowCV::FlowCV_Manager& flow, std::uint64_t nodeId, int grayValue)
	{
		// Solid原版默认生成彩色图像，此处设置为单通道灰度图
		Require(
			flow.SetNodeProperty(
				nodeId, "is_color", false),
			"Failed to set Solid is_color"
		);
		Require(
			flow.SetNodeProperty(
				nodeId, "width", kWidth),
			"Failed to set Solid width"
		);

		Require(
			flow.SetNodeProperty(
				nodeId, "height", kHeight),
			"Failed to set Solid height"
		);
		Require(
			flow.SetNodeProperty(
				nodeId, "value", grayValue),
			"Failed to set Solid value"
		);
	}

}	// anonymous namespace

// ----------------------------------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------------------------------
int main()
{
	try
	{
		std::cout
			<< "========================================\n"
			<< " VisionFlowStudio - First Runtime Flow\n"
			<< "========================================\n\n";

		// ----------------------------------------------------
		// 1. Create FlowCV Manager
		// ----------------------------------------------------

		std::cout
			<< "[1] Creating FlowCV Manager...\n";

		FlowCV::FlowCV_Manager flow;

		// 当前阶段显式选择无工作线程模式。
		// 配合 TickMode::Series 进行同步执行。
		flow.SetBufferCount(0);

		// ----------------------------------------------------
		// 2. Check registered nodes
		// ----------------------------------------------------
		std::cout
			<< "[2] Checking internal nodes...\n";

		Require(
			flow.internal_node_manager_ != nullptr,
			"InternalNodeManager is null"
		);

		Require(
			flow.internal_node_manager_->HasNode("Solid"),
			"Solid is not registered"
		);

		Require(
			flow.internal_node_manager_->HasNode("Add"),
			"Add is not registered"
		);

		Require(
			flow.internal_node_manager_->HasNode("Viewer"),
			"Viewer is not registered"
		);

		std::cout
			<< "    Registered node types: "
			<< flow.internal_node_manager_->NodeCount()
			<< '\n';

		// ----------------------------------------------------
		// 3. Create node instances
		// ----------------------------------------------------

		std::cout
			<< "[3] Creating node instances...\n";

		const std::uint64_t solidA =
			flow.CreateNewNodeInstance("Solid");

		const std::uint64_t solidB =
			flow.CreateNewNodeInstance("Solid");

		const std::uint64_t add =
			flow.CreateNewNodeInstance("Add");

		const std::uint64_t viewerId =
			flow.CreateNewNodeInstance("Viewer");

		Require(
			solidA != 0 &&
			solidB != 0 &&
			add != 0 &&
			viewerId != 0,
			"Failed to create node instances"
		);

        std::cout
            << "    Solid A ID: " << solidA << '\n'
            << "    Solid B ID: " << solidB << '\n'
            << "    Add ID:     " << add << '\n'
            << "    Viewer ID:  " << viewerId << '\n';

        Require(
            flow.GetNodeCount() == 4,
            "Unexpected node instance count"
        );

        // ----------------------------------------------------
        // 4. Configure Solid properties
        // ----------------------------------------------------

        std::cout
            << "[4] Configuring Solid nodes...\n";

        ConfigureSolid(
            flow, solidA, kSolidValueA
        );

        ConfigureSolid(
            flow, solidB, kSolidValueB
        );

        std::cout
            << "    Solid A: " << kSolidValueA << '\n'
            << "    Solid B: " << kSolidValueB << '\n'
            << "    Size:    "
            << kWidth << " x " << kHeight << '\n';

        // ----------------------------------------------------
        // 5. Connect nodes
        // ----------------------------------------------------

        std::cout
            << "[5] Connecting nodes...\n";

        // Solid A output[0] -> Add input[0]
        Require(
            flow.ConnectNodes(
                solidA, 0,
                add, 0),
            "Failed to connect Solid A -> Add"
        );

        // Solid B output[0] -> Add input[1]
        Require(
            flow.ConnectNodes(
                solidB, 0,
                add, 1),
            "Failed to connect Solid B -> Add"
        );

        // Add output[0] -> Viewer input[0]
        Require(
            flow.ConnectNodes(
                add, 0,
                viewerId, 0),
            "Failed to connect Add -> Viewer"
        );

        // Add input[2] 是可选 Mask，暂不连接。

        std::cout
            << "    Solid A -> Add [0]\n"
            << "    Solid B -> Add [1]\n"
            << "    Add     -> Viewer [0]\n";

        std::cout
            << "    Connection count: "
            << flow.GetWireCount()
            << '\n';

        Require(
            flow.GetWireCount() == 3,
            "Unexpected connection count"
        );

        // ----------------------------------------------------
        // 6. Execute flow
        // ----------------------------------------------------

        std::cout
            << "[6] Executing DSPatch flow...\n";

        // Series:
        // 同步执行，确保本次 Tick 完成后可以读取 Viewer。
        //
        // 不启动 StartAutoTick()。
        flow.Tick(
            DSPatch::Component::TickMode::Series
        );

        std::cout
            << "    Tick completed.\n";

        // ----------------------------------------------------
        // 7. Retrieve Viewer instance
        // ----------------------------------------------------

        std::cout
            << "[7] Retrieving Viewer frame...\n";

        FlowCV::NodeInfo viewerInfo{};

        Require(
            flow.GetNodeInfoById(
                viewerId, viewerInfo),
            "Failed to retrieve Viewer node info"
        );

        auto viewer =
            std::dynamic_pointer_cast<
            DSPatch::DSPatchables::Viewer>(
                viewerInfo.node_ptr
            );

        Require(
            static_cast<bool>(viewer),
            "Viewer instance type mismatch"
        );

        // 使用阶段二添加的线程安全深拷贝接口。
        const cv::Mat frame =
            viewer->GetFrameCopy();

        Require(
            !frame.empty(),
            "Viewer frame is empty"
        );

        // ----------------------------------------------------
        // 8. Validate result
        // ----------------------------------------------------

        std::cout
            << "[8] Checking output frame...\n";

        std::cout
            << "    Frame size: "
            << frame.cols << " x " << frame.rows
            << '\n';

        std::cout
            << "    Channels: "
            << frame.channels()
            << '\n';

        // Viewer 构造时本身有一张默认黑色图像，
        // 所以不能只检查 frame.empty()。
        //
        // 先检查尺寸和单通道类型，确认得到的是
        // 两个 Solid 经过 Add 生成的灰度图像。
        Require(
            frame.cols == kWidth &&
            frame.rows == kHeight,
            "Unexpected output frame size"
        );

        Require(
            frame.type() == CV_8UC1,
            "Output frame is not CV_8UC1"
        );

        // 读取整张图像的最小值、最大值和平均值。
        //
        // 仅检查平均值可能掩盖部分像素错误，
        // 因此使用最小值和最大值验证所有像素。
        double minValue = 0.0;
        double maxValue = 0.0;

        cv::minMaxLoc(
            frame,
            &minValue,
            &maxValue
        );

        const double meanValue =
            cv::mean(frame)[0];

        std::cout
            << "    Expected value: "
            << kExpectedValue << '\n'
            << "    Minimum value:  "
            << minValue << '\n'
            << "    Maximum value:  "
            << maxValue << '\n'
            << "    Mean value:     "
            << meanValue << '\n';

        Require(
            minValue == kExpectedValue &&
            maxValue == kExpectedValue,
            "Output pixels do not match expected value"
        );

        // ----------------------------------------------------
        // 9. Success
        // ----------------------------------------------------

        std::cout
            << "\n[PASS] First VisionFlow runtime "
            "closed loop completed.\n";

        // flow 离开作用域后由 FlowCV_Manager 析构，
        // 清理 Circuit 和节点资源。
        return 0;


	}
	catch (const std::exception& e)
	{
        std::cerr << "\n[FAIL] " << e.what() << '\n';

        return 1;
	}
}