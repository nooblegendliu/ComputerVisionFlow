#include "solid.hpp"	

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

#include <opencv2/core.hpp>
#include <nlohmann/json.hpp>

namespace DSPatch::DSPatchables
{
	namespace
	{
		// Fps Options
		constexpr std::array<int, 10> kFpsValues =
		{
			1, 3, 5, 10, 15, 20, 25, 30, 60, 120
		};

		int GetFpsFromIndex(int index)
		{
			return kFpsValues.at(static_cast<std::size_t>(index));
		}
	}

	// -------------------------------------------------------------------------
	//	Constructor
	// -------------------------------------------------------------------------
	Solid::Solid() : FlowCV::Node(DSPatch::Component::ProcessOrder::InOrder)
	{
		// 保留FlowCV节点信息
		SetComponentName_("Solid");

		SetComponentCategory_(DSPatch::Category::Category_Source);

		SetComponentAuthor_("Ljk");

		SetComponentVersion_("0.1.0");

		// 0 inputs
		SetInputCount_(0);

		// 1 output
		SetOutputCount_(
			1,
			{ "out" },
			{ DSPatch::IoType::Io_Type_CvMat }
		);

		// -----------------------------------------------
		// Properties
		// -----------------------------------------------
		properties_.AddInt("width", "Width", 640, 2, 10000, 0.5f);
		properties_.AddInt("height", "Height", 480, 2, 10000, 0.5f);
		properties_.AddBool("is_color", "Color", true);
		properties_.AddBool("has_alpha", "Has Alpha", true);

		// 使用Float代替ImVec4, 默认0 0 0 1
		properties_.AddFloat("color_r", "Red", 0.0f, 0.0f, 1.0f, 0.01f);
		properties_.AddFloat("color_g", "Green", 0.0f, 0.0f, 1.0f, 0.01f);
		properties_.AddFloat("color_b", "Blue", 0.0f, 0.0f, 1.0f, 0.01f);
		properties_.AddFloat("color_a", "Alpha", 1.0f, 0.0f, 1.0f, 0.01f);

		// 原版灰度值 grey_value_ = 0
		properties_.AddInt("value", "Solid Value", 0, 0, 255, 1.0f);

		properties_.AddOption("fps_index", "Output FPS", 7,
			{
				"1", "3", "5", "10", "15", "20", "25", "30", "60", "120"
			});

		// 与原版一致
		last_time_ = std::chrono::steady_clock::now();

		RefreshProperties();
		
		SetEnabled(true);
	}

	// ---------------------------------------------------------------
	// RefreshProperties
	// ---------------------------------------------------------------
	void Solid::RefreshProperties()
	{
		const bool isColor = properties_.GetW<bool>("is_color");
		const bool hasAlpha = properties_.GetW<bool>("has_alpha");

		// 彩色模式下显示Alpha选项
		properties_.SetVisibility("has_alpha", isColor);

		// 彩色模式下显示RGB
		properties_.SetVisibility("color_r", isColor);
		properties_.SetVisibility("color_g", isColor);
		properties_.SetVisibility("color_b", isColor);

		// 启用Alpha时显示Alpha参数
		properties_.SetVisibility("color_a", isColor && hasAlpha);

		// 灰度模式下显示灰度值
		properties_.SetVisibility("value", !isColor);
	}


	// ------------------------------------------------------------------
	// Process
	// ------------------------------------------------------------------
	void Solid::Process_(SignalBus const& inputs, SignalBus& outputs)
	{
		std::unique_lock<std::mutex> lock(io_mutex_, std::try_to_lock);

		if (!lock.owns_lock()) return;

		// 编辑参数进行同步
		properties_.Sync();

		// ---------------------- Read Property -----------------------
		const int width = std::clamp(properties_.Get<int>("width"), 2, 10000);
		const int height = std::clamp(properties_.Get<int>("height"), 2, 10000);

		const bool isColor = properties_.Get<bool>("is_color");
		const bool hasAlpha = properties_.Get<bool>("has_alpha");

		const float red = std::clamp(properties_.Get<float>("color_r"), 0.0f, 1.0f);
		const float green = std::clamp(properties_.Get<float>("color_g"), 0.0f, 1.0f);
		const float blue = std::clamp(properties_.Get<float>("color_b"), 0.0f, 1.0f);
		const float alpha = std::clamp(properties_.Get<float>("color_a"), 0.0f, 1.0f);

		const int grayValue = std::clamp(properties_.Get<int>("value"), 0, 255);

		const int fpsIndex = properties_.Get<int>("fps_index");
		// 与原版fps_/fps_time_的计算方式相同
		const int fps = GetFpsFromIndex(fpsIndex);
		const float fpsTime = (1.0f / static_cast<float>(fps)) * 1000.0f;

		// ---------------------- fps control ----------------------------
		// 保留原版忙等待方式，暂不更改调度原则
		bool shouldWait = true;

		while (shouldWait) {
			const auto currentTime = std::chrono::steady_clock::now();

			const auto delta = std::chrono::duration_cast
				<std::chrono::milliseconds>(currentTime - last_time_).count();

			if (delta >= static_cast<uint32_t>(fpsTime)) {
				shouldWait = false;
			}
		}

		// -------------------------- Generate Frame ------------------------------
		cv::Mat frame;

		if (isColor)
		{
			if (hasAlpha)
			{
				// 原版颜色存储为RGBA，OpenCV的Scalar采取BGRA的顺序存储
				frame = cv::Mat(height, width, CV_8UC4, 
					cv::Scalar(blue * 255.0f, green * 255.0f, red * 255.0f, alpha * 255.0f));
			}
			else
			{
				frame = cv::Mat(height, width, CV_8UC3,
					cv::Scalar(blue * 255.0f, green * 255.0f, red * 255.0f));
			}
		}
		else
		{
			frame = cv::Mat(height, width, CV_8UC1, cv::Scalar(grayValue));
		}

		// ----------------------------- Output ---------------------------------
		// 保留原版DSPatch输出方式
		outputs.SetValue(0, frame);
		
		// 保留原版更新时间点的时间
		last_time_ = std::chrono::steady_clock::now();
	}



	// ------------------------------------------------------------------
	// GetState
	// ------------------------------------------------------------------
	std::string Solid::GetState()
	{
		using nlohmann::json;

		json state = json::object();

		// 使用编辑值
		state["width"] = properties_.GetW<int>("width");
		state["height"] = properties_.GetW<int>("height");

		// 保留原版颜色json
		json solidColor = json::object();

		solidColor["R"] = properties_.GetW<float>("color_r");
		solidColor["G"] = properties_.GetW<float>("color_g");
		solidColor["B"] = properties_.GetW<float>("color_b");
		solidColor["A"] = properties_.GetW<float>("color_a");

		state["solid_color"] = std::move(solidColor);

		state["value"] = properties_.GetW<int>("value");

		const int fpsIndex = properties_.GetW<int>("fps_index");
		state["fps"] = GetFpsFromIndex(fpsIndex);
		state["fps_index"] = fpsIndex;
		state["is_color"] = properties_.GetW<bool>("is_color");
		state["has_alpha"] = properties_.GetW<bool>("has_alpha");

		// 保留原版格式化序列行为
		return state.dump(4);
	}


	// --------------------------------------------------------------------
	//	SetState
	// --------------------------------------------------------------------
	void Solid::SetState(std::string&& serialized)
	{
		if (serialized.empty())
			return;

		using nlohmann::json;

		const json state = json::parse(serialized);

		if (!state.is_object())
			return;

		// 将原版Json字段映射到properties
		json updates = json::object();

		// 复制名称相同的属性
		const auto copyProperty = [&](const char* stateKey, const char* propertyKey)
			{
				const auto it = state.find(stateKey);

				if (it != state.end()) {
					updates[propertyKey] = *it;
				}
			};

		copyProperty("width", "width");
		copyProperty("height", "height");
		copyProperty("value", "value");
		copyProperty("is_color", "is_color");
		copyProperty("has_alpha", "has_alpha");

		// ------------------------- Restore Color --------------------------------
		if (state.contains("solid_color"))
		{
			const auto& solidColor = state.at("solid_color");

			if (!solidColor.is_object())
			{
				throw std::invalid_argument("solid_color must be an object");
			}

			const auto copyColor =
				[&](const char* channel, const char* propertyKey)
				{
					const auto it = solidColor.find(channel);

					if (it != solidColor.end())
					{
						updates[propertyKey] = *it;
					}
				};

			copyColor("R", "color_r");
			copyColor("G", "color_g");
			copyColor("B", "color_b");
			copyColor("A", "color_a");
		}

		// ---------------------------- Restore fps -----------------------------------
		if (state.contains("fps_index"))
		{
			updates["fps_index"] = state.at("fps_index");
		}
		else if (state.contains("fps"))
		{
			// 兼容只有fps字段的状态文件
			const int fps = state.at("fps").get<int>();

			const auto it = std::find(kFpsValues.begin(), kFpsValues.end(), fps);

			if (it == kFpsValues.end())
			{
				throw std::invalid_argument("Unsupported Solid FPS value");
			}

			updates["fps_index"] = static_cast<int>(std::distance(kFpsValues.begin(), it));
		}

		// ------------------------------ Apply Properties -------------------------------
		properties_.FromJson(updates);

		// 恢复可见性等依赖属性
		RefreshProperties();
	}


}	// namespace DSPatch::DSPatchables