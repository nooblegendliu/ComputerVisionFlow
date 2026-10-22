#include "condition_check.hpp"

#include <inspectionResult.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace DSPatch::DSPatchables
{
	namespace
	{
        using json = nlohmann::json;

        // The default condition matches the existing A3 Line_Intersections output.
        // Any JSON measurement can be checked by changing the rules and expected type.
        constexpr const char* kDefaultRules =
            R"([{"name":"point_count","path":"/data","source":"array_size","op":"between","min":2,"max":2}])";

        struct Rule
        {
            std::string name;
            json::json_pointer path;
            bool arraySize;
            std::string op;
            double low;
            double high;
        };

        double ReadNumber(const json& value)
        {
            if (!value.is_number()) {
                throw std::invalid_argument("Condition requires a numeric value");
            }
            const double number = value.get<double>();
            if (!std::isfinite(number)) {
                throw std::invalid_argument("Condition requires a finite number");
            }
            return number;
        }

        std::vector<Rule> ParseRules(const std::string& serialized)
        {
            const json configuration = json::parse(serialized);
            if (!configuration.is_array() || configuration.empty() || configuration.size() > 32) {
                throw std::invalid_argument("Conditions must be a non-empty JSON array with at most 32 rules");
            }

            std::vector<Rule> rules;
            rules.reserve(configuration.size());

            for (const auto& value : configuration) {
                if (!value.is_object()) {
                    throw std::invalid_argument("Each condition must be a JSON object");
                }
                const std::string name = value.at("name").get<std::string>();
                const std::string pathText = value.at("path").get<std::string>();
                const std::string source = value.at("source").get<std::string>();
                const std::string op = value.at("op").get<std::string>();

                if (name.empty() || pathText.empty() || pathText.front() != '/') {
                    throw std::invalid_argument("Condition needs a name and JSON Pointer path beginning with /");
                }
                if (source != "number" && source != "array_size") {
                    throw std::invalid_argument("Condition source must be number or array_size");
                }
                const bool between = op == "between";
                if (!between && op != "eq" && op != "ne" &&
                    op != "gt" && op != "ge" && op != "lt" && op != "le") {
                    throw std::invalid_argument("Unsupported condition operator: " + op);
                }

                double low = 0.0;
                double high = 0.0;
                if (between) {
                    low = ReadNumber(value.at("min"));
                    high = ReadNumber(value.at("max"));
                    if (low > high) {
                        throw std::invalid_argument("Condition min must not exceed max");
                    }
                }
                else {
                    low = ReadNumber(value.at("value"));
                }

                rules.push_back({ name, json::json_pointer(pathText),
                                 source == "array_size", op, low, high });
            }
            return rules;
        }

        bool Matches(double measured, const Rule& rule)
        {
            if (rule.op == "between") return measured >= rule.low && measured <= rule.high;
            if (rule.op == "eq") return measured == rule.low;
            if (rule.op == "ne") return measured != rule.low;
            if (rule.op == "gt") return measured > rule.low;
            if (rule.op == "ge") return measured >= rule.low;
            if (rule.op == "lt") return measured < rule.low;
            return measured <= rule.low; // "le" (validated during ParseRules)
        }

        json Evaluate(const json& input, const std::vector<Rule>& rules,
            bool requireAll, const std::string& expectedType,
            int& decision)
        {
            if (!input.is_object()) {
                throw std::invalid_argument("Input must be a JSON object");
            }
            if (!expectedType.empty()) {
                if (!input.contains("data_type") || !input.at("data_type").is_string() ||
                    input.at("data_type").get<std::string>() != expectedType) {
                    throw std::invalid_argument("Unexpected input data_type");
                }
            }

            // For the A3 points contract, a count is meaningful only when every
            // point has valid coordinates. Do not classify malformed points as OK.
            if (expectedType == "points") {
                if (!input.contains("data") || !input.at("data").is_array()) {
                    throw std::invalid_argument("points.data must be an array");
                }
                for (const auto& point : input.at("data")) {
                    if (!point.is_object() || !point.contains("x") || !point.contains("y")) {
                        throw std::invalid_argument("points must contain x and y");
                    }
                    (void)ReadNumber(point.at("x"));
                    (void)ReadNumber(point.at("y"));
                }
            }

            json details = json::array();
            int passed = 0;
            for (const Rule& rule : rules) {
                const json& field = input.at(rule.path);
                double observed = 0.0;
                if (rule.arraySize) {
                    if (!field.is_array()) {
                        throw std::invalid_argument("Condition array_size source is not an array: " + rule.name);
                    }
                    observed = static_cast<double>(field.size());
                }
                else {
                    observed = ReadNumber(field);
                }
                const bool matched = Matches(observed, rule);
                if (matched) ++passed;
                details.push_back({ {"name", rule.name}, {"value", observed}, {"passed", matched} });
            }

            const bool ok = requireAll ? passed == static_cast<int>(rules.size()) : passed > 0;
            const json measurements = {
                {"condition_count", rules.size()},
                {"passed_count", passed},
                {"failed_count", rules.size() - static_cast<std::size_t>(passed)},
                {"conditions", std::move(details)}
            };

            FlowCV::Inspection::Result result = ok
                ? FlowCV::Inspection::MakeOK(measurements)
                : FlowCV::Inspection::MakeNG("CONDITION_NOT_MET",
                    "Configured conditions were not satisfied",
                    measurements);

            // Preserve genuine input provenance; never invent a frame identifier.
            if (input.contains("ref_frame")) result.refFrame = input.at("ref_frame");
            if (input.contains("frame_id")) {
                if (!input.at("frame_id").is_number_unsigned()) {
                    throw std::invalid_argument("frame_id must be an unsigned integer");
                }
                result.frameId = input.at("frame_id").get<std::uint64_t>();
            }

            const json output = FlowCV::Inspection::ToJson(result);
            decision = ok ? 1 : 0;
            return output;
        }

        json ErrorResult(const char* code, const std::string& message)
        {
            return FlowCV::Inspection::ToJson(FlowCV::Inspection::MakeError(code, message));
        }
	}   // anonymous namespace


    ConditionCheck::ConditionCheck()
        :FlowCV::Node(ProcessOrder::OutOfOrder)
    {
        SetComponentName_("Condition_Check");
        SetComponentCategory_(Category::Category_Utility);
        SetComponentAuthor_("ComputerVisionFlow");
        SetComponentVersion_("0.1.0");
        SetInputCount_(1, { "in" }, { IoType::Io_Type_JSON });
        SetOutputCount_(2, { "result", "decision" }, { IoType::Io_Type_JSON, IoType::Io_Type_Int });

        properties_.AddOption("logic_mode", "Logic", 0, { "All", "Any" });
        properties_.AddString("expected_data_type", "Expected Data Type", "points");
        properties_.AddString("conditions_json", "Conditions JSON", kDefaultRules);
        SetEnabled(true);
    }

    void ConditionCheck::Process_(SignalBus const& inputs, SignalBus& outputs)
    {
        // Missing input and disabled nodes do not produce a business verdict.
        // Such absence must not be misinterpreted as NG or OK by consumers.
        if (!IsEnabled()) return;
        const auto* input = inputs.GetValue<json>(0);
        if (!input) return;

        properties_.Sync();
        json output;
        int decision = -1; // extension: -1 = ERROR; VM's usual valid decisions are 0/1.
        try {
            const int logic = properties_.Get<int>("logic_mode");
            if (logic != 0 && logic != 1) {
                throw std::invalid_argument("Logic option must be All or Any");
            }
            const std::string expectedType = properties_.Get<std::string>("expected_data_type");
            const std::vector<Rule> rules =
                ParseRules(properties_.Get<std::string>("conditions_json"));
            try {
                output = Evaluate(*input, rules, logic == 0, expectedType, decision);
            }
            catch (const std::exception& error) {
                output = ErrorResult("INVALID_CONDITION_INPUT", error.what());
            }
        }
        catch (const std::exception& error) {
            output = ErrorResult("INVALID_CONFIGURATION", error.what());
        }

        outputs.SetValue(0, output);
        outputs.SetValue(1, decision);
    }


}	// namespace DSPatch::DSPatchables