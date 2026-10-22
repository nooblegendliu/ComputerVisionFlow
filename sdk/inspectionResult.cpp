#include "inspectionResult.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace FlowCV::Inspection
{
	namespace
	{
        const char* StatusText(Status status)
        {
            switch (status) {
            case Status::OK: return "OK";
            case Status::NG: return "NG";
            case Status::ERROR: return "ERROR";
            }
            throw std::invalid_argument("Unknown inspection status");
        }

        Status ParseStatus(const std::string& text)
        {
            if (text == "OK") return Status::OK;
            if (text == "NG") return Status::NG;
            if (text == "ERROR") return Status::ERROR;
            throw std::invalid_argument("Unknown inspection status: " + text);
        }

        bool AllNumbersFinite(const nlohmann::json& value)
        {
            if (value.is_number_float()) {
                return std::isfinite(value.get<double>());
            }
            if (value.is_array() || value.is_object()) {
                for (const auto& child : value) {
                    if (!AllNumbersFinite(child)) return false;
                }
            }
            return true;
        }
	}   // anonymous namespace


    Result MakeOK(nlohmann::json measurements)
    {
        Result result;
        result.status = Status::OK;
        result.reasonCode = "NONE";
        result.measurements = std::move(measurements);
        Validate(result);
        return result;
    }

    Result MakeNG(std::string reasonCode, std::string message,
        nlohmann::json measurements)
    {
        Result result;
        result.status = Status::NG;
        result.reasonCode = std::move(reasonCode);
        result.message = std::move(message);
        result.measurements = std::move(measurements);
        Validate(result);
        return result;
    }

    Result MakeError(std::string reasonCode, std::string message)
    {
        Result result;
        result.status = Status::ERROR;
        result.reasonCode = std::move(reasonCode);
        result.message = std::move(message);
        Validate(result);
        return result;
    }

    void Validate(const Result& result)
    {
        // Confirm enum validity before any serialization or acceptance.
        (void)StatusText(result.status);

        if (result.status == Status::OK) {
            if (result.reasonCode != "NONE") {
                throw std::invalid_argument("OK inspection requires reason_code NONE");
            }
        }
        else if (result.reasonCode.empty() || result.reasonCode == "NONE") {
            throw std::invalid_argument("NG/ERROR inspection requires a failure reason_code");
        }

        if (!result.measurements.is_object() ||
            !AllNumbersFinite(result.measurements)) {
            throw std::invalid_argument("Inspection measurements must be a JSON object with finite numbers");
        }

        if (result.refFrame &&
            (!result.refFrame->is_object() || !AllNumbersFinite(*result.refFrame))) {
            throw std::invalid_argument("Inspection ref_frame must be a JSON object with finite numbers");
        }
    }

    nlohmann::json ToJson(const Result& result)
    {
        Validate(result);

        nlohmann::json value = {
            {"data_type", kDataType},
            {"schema_version", kSchemaVersion},
            {"status", StatusText(result.status)},
            {"valid", result.IsValid()},
            {"reason_code", result.reasonCode},
            {"message", result.message},
            {"measurements", result.measurements}
        };

        if (result.refFrame) value["ref_frame"] = *result.refFrame;
        if (result.frameId) value["frame_id"] = *result.frameId;
        return value;
    }

    Result FromJson(const nlohmann::json& value)
    {
        if (!value.is_object()) {
            throw std::invalid_argument("Inspection result must be a JSON object");
        }
        if (!value.contains("data_type") || !value.at("data_type").is_string() ||
            value.at("data_type").get<std::string>() != kDataType) {
            throw std::invalid_argument("Input is not an inspection_result");
        }
        if (!value.contains("schema_version") || !value.at("schema_version").is_number_integer() ||
            value.at("schema_version").get<int>() != kSchemaVersion) {
            throw std::invalid_argument("Unsupported inspection_result schema_version");
        }
        if (!value.contains("status") || !value.at("status").is_string() ||
            !value.contains("valid") || !value.at("valid").is_boolean() ||
            !value.contains("reason_code") || !value.at("reason_code").is_string() ||
            !value.contains("message") || !value.at("message").is_string() ||
            !value.contains("measurements")) {
            throw std::invalid_argument("Invalid or missing inspection_result fields");
        }

        Result result;
        result.status = ParseStatus(value.at("status").get<std::string>());
        result.reasonCode = value.at("reason_code").get<std::string>();
        result.message = value.at("message").get<std::string>();
        result.measurements = value.at("measurements");

        if (value.contains("ref_frame")) {
            result.refFrame = value.at("ref_frame");
        }
        if (value.contains("frame_id")) {
            const auto& id = value.at("frame_id");
            if (!id.is_number_unsigned()) {
                throw std::invalid_argument("Inspection frame_id must be an unsigned integer");
            }
            result.frameId = id.get<std::uint64_t>();
        }

        Validate(result);
        if (value.at("valid").get<bool>() != result.IsValid()) {
            throw std::invalid_argument("Inspection valid/status mismatch");
        }
        return result;
    }

}   // namespace FlowCV::Inspection