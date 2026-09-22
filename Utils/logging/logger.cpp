#include "logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace FlowCV
{
	FlowLogger::FlowLogger()
	{
		// 初始化异步日志线程池：(队列容量, 后台线程数量)
		spdlog::init_thread_pool(8192, 1);

		// 创建控制台sink(spdlog的输出目标), 支持颜色、多线程
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

		// 创建每日日志sink, 参数(文件路径,小时时间,分钟时间)
		auto daily_sink = 
			std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/log.log", 2, 30);
		
		// 组合多个sink, 创建输出列表
		std::vector<spdlog::sink_ptr> sinks{ console_sink, daily_sink };

		// 创建异步logger
		// “flow": 日志器名称
		// begin(),end(): 使用哪些输出目标
		// thread_pool(): 异步,使用线程池
		// block: 队列满时阻塞而非丢弃日志
		auto logger = std::make_shared<spdlog::async_logger>(
			"flow",
			sinks.begin(),
			sinks.end(),
			spdlog::thread_pool(),
			spdlog::async_overflow_policy::block);

		// 注册并设置默认logger
		spdlog::register_logger(logger);
		spdlog::set_default_logger(logger);
	}


	FlowLogger::Level FlowLogger::getLevel()
	{
		switch (spdlog::get_level()) {
		case spdlog::level::level_enum::debug:
			return Level::debug;

		case spdlog::level::level_enum::info:
			return Level::info;

		case spdlog::level::level_enum::warn:
			return Level::warn;

		case spdlog::level::level_enum::err:
			return Level::err;

		case spdlog::level::level_enum::off:
			return Level::off;

		default:
			break;
		}
		return Level::debug;
	}

	void FlowLogger::setLevel(Level l)
	{
		auto level = spdlog::level::level_enum::debug;

		switch (l) {
		case Level::debug:
			level = spdlog::level::level_enum::debug;
			LOG_INFO("set log level to debug");
			break;

		case Level::info:
			level = spdlog::level::level_enum::info;
			LOG_INFO("set log level to info");
			break;

		case Level::warn:
			level = spdlog::level::level_enum::warn;
			LOG_INFO("set log level to warn");
			break;

		case Level::err:
			level = spdlog::level::level_enum::err;
			LOG_INFO("set log level to error");
			break;

		case Level::off:
			level = spdlog::level::level_enum::off;
			LOG_INFO("set log level to off");
			break;

		default:
			break;
		}

		spdlog::set_level(level);
	}

}	// namespace FlowCV