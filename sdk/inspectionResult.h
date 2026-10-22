#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace FlowCV::Inspection
{
	inline constexpr int kSchemaVersion = 1;
	inline constexpr const char* kDataType = "inspection_result";

	enum class Status
	{
		OK,
		NG,
		ERROR
	};

	struct Result
	{
		// Safe default: never imply that an unevaluated result is OK
		Status status = Status::ERROR;
		std::string reasonCode = "NOT_EVALUATED";
		std::string message;
		nlohmann::json measurements = nlohmann::json::object();

		// Optional provenance. Do not fabricate an image identity.
		std::optional<nlohmann::json> refFrame;
		std::optional<std::uint64_t> frameId;

		// valid means "the business verdict is valid" rather than "OK".
		// NG is a valid business verdict; ERROR is not.
		bool IsValid() const noexcept { return status != Status::ERROR; }
	};

	// Factory helpers for inspection logic in future B2+ nodes.
	Result MakeOK(nlohmann::json measurements = nlohmann::json::object());
	Result MakeNG(std::string reasonCode, std::string message = {},
		nlohmann::json measurements = nlohmann::json::object());
	Result MakeError(std::string reasonCode, std::string message = {});

	// Throw std::invalid_argument on invalid status/reason/measurement content.
	// These functions do not silently reinterpret old "points" JSON as results.
	void Validate(const Result& result);
	nlohmann::json ToJson(const Result& result);
	Result FromJson(const nlohmann::json& value);

}	// namespace Inspection