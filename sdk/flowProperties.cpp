#include "flowProperties.h"

#include <cmath>
#include <limits>
#include <utility>

namespace FlowCV
{
    namespace
    {
        void ValidateStep(float step)
        {
            if (!std::isfinite(step) || step <= 0.0f)
                throw std::invalid_argument("Property step must be finite and positive");
        }

        void ValidateRange(const IntRange& range)
        {
            if (range.min > range.max)
                throw std::invalid_argument("Property minimum exceeds maximum");
            ValidateStep(range.step);
        }

        void ValidateRange(const FloatRange& range)
        {
            if (!std::isfinite(range.min) || !std::isfinite(range.max) || range.min > range.max)
                throw std::invalid_argument("Property range must be finite and ordered");
            ValidateStep(range.step);
        }

        // Check before conversion: json::get<int>() alone can narrow or truncate.
        int ParseInteger(const nlohmann::json& value)
        {
            if (value.is_number_unsigned()) {
                const auto number = value.get<nlohmann::json::number_unsigned_t>();
                if (number > static_cast<nlohmann::json::number_unsigned_t>((std::numeric_limits<int>::max)()))
                    throw std::invalid_argument("Property integer is out of range");
                return static_cast<int>(number);
            }
            if (value.is_number_integer()) {
                const auto number = value.get<nlohmann::json::number_integer_t>();
                if (number < (std::numeric_limits<int>::min)() || number > (std::numeric_limits<int>::max)())
                    throw std::invalid_argument("Property integer is out of range");
                return static_cast<int>(number);
            }
            throw std::invalid_argument("Property requires a JSON integer");
        }

        float ParseFloat(const nlohmann::json& value)
        {
            // Integer JSON literals are valid representations of float parameters.
            if (!value.is_number())
                throw std::invalid_argument("Property requires a JSON number");
            const auto number = value.get<double>();
            const double limit = (std::numeric_limits<float>::max)();
            if (!std::isfinite(number) || number < -limit || number > limit)
                throw std::invalid_argument("Property float must be finite and representable");
            return static_cast<float>(number);
        }
    }

    void FlowCV_Properties::AddBool(const std::string& key, const std::string& desc, bool value, bool visible)
    {
        AddRecord({key, desc, PropertyDataTypes::kDataTypeBool,
            value, value, value, std::monostate{}, {}, visible});
    }

    void FlowCV_Properties::AddInt(const std::string& key, const std::string& desc, int value,
        int min, int max, float step, bool visible)
    {
        AddRecord({key, desc, PropertyDataTypes::kDataTypeInt,
            value, value, value, IntRange{min, max, step}, {}, visible});
    }

    void FlowCV_Properties::AddFloat(const std::string& key, const std::string& desc, float value,
        float min, float max, float step, bool visible)
    {
        AddRecord({key, desc, PropertyDataTypes::kDataTypeFloat,
            value, value, value, FloatRange{min, max, step}, {}, visible});
    }

    void FlowCV_Properties::AddOption(const std::string& key, const std::string& desc, int value,
        std::vector<std::string> options, bool visible)
    {
        AddRecord({key, desc, PropertyDataTypes::kDataTypeOption,
            value, value, value, std::monostate{}, std::move(options), visible});
    }

    void FlowCV_Properties::AddRecord(PropertyRecord record)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        if (prop_idx_.find(record.key) != prop_idx_.end())
            throw std::invalid_argument("Duplicate property key: " + record.key);
        if (const auto* range = std::get_if<IntRange>(&record.range))
            ValidateRange(*range);
        if (const auto* range = std::get_if<FloatRange>(&record.range))
            ValidateRange(*range);
        if (record.type == PropertyDataTypes::kDataTypeOption) {
            if (record.options.empty()
                || record.options.size() - 1 > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
                throw std::invalid_argument("Property options must have representable, nonempty indices");
        }
        ValidateValue(record, record.writeValue);

        // Keep the index and ordered records consistent if allocation fails.
        const auto index = props_.size();
        const auto entry = prop_idx_.emplace(record.key, index).first;
        try {
            props_.push_back(std::move(record));
        }
        catch (...) {
            prop_idx_.erase(entry);
            throw;
        }
        ++revision_;
    }

    FlowCV_Properties::PropertyRecord* FlowCV_Properties::FindUnlocked(const std::string& key)
    {
        const auto it = prop_idx_.find(key);
        return it == prop_idx_.end() ? nullptr : &props_[it->second];
    }

    const FlowCV_Properties::PropertyRecord* FlowCV_Properties::FindUnlocked(const std::string& key) const
    {
        const auto it = prop_idx_.find(key);
        return it == prop_idx_.end() ? nullptr : &props_[it->second];
    }

    void FlowCV_Properties::ValidateValue(const PropertyRecord& record, const PropertyValue& value)
    {
        bool matches = false;
        switch (record.type) {
        case PropertyDataTypes::kDataTypeBool:
            matches = std::holds_alternative<bool>(value);
            break;
        case PropertyDataTypes::kDataTypeInt:
        case PropertyDataTypes::kDataTypeOption:
            matches = std::holds_alternative<int>(value);
            break;
        case PropertyDataTypes::kDataTypeFloat:
            matches = std::holds_alternative<float>(value);
            break;
        default:
            break;
        }
        if (!matches)
            throw std::invalid_argument("Property value type mismatch: " + record.key);
        if (const auto* number = std::get_if<float>(&value)) {
            if (!std::isfinite(*number))
                throw std::invalid_argument("Property float must be finite: " + record.key);
        }
        if (record.type == PropertyDataTypes::kDataTypeOption) {
            const auto index = std::get<int>(value);
            if (index < 0 || static_cast<std::size_t>(index) >= record.options.size())
                throw std::invalid_argument("Property option index is out of range: " + record.key);
        }
    }

    void FlowCV_Properties::Remove(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        const auto it = prop_idx_.find(key);
        if (it == prop_idx_.end())
            return;
        const auto index = it->second;
        props_.erase(props_.begin() + static_cast<std::ptrdiff_t>(index));
        prop_idx_.erase(it);
        for (std::size_t i = index; i < props_.size(); ++i)
            prop_idx_.at(props_[i].key) = i;
        ++revision_;
    }

    void FlowCV_Properties::RemoveAll()
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        if (props_.empty())
            return;
        props_.clear();
        prop_idx_.clear();
        ++revision_;
    }

    bool FlowCV_Properties::Exists(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        return FindUnlocked(key) != nullptr;
    }

    bool FlowCV_Properties::Changed(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        const auto* record = FindUnlocked(key);
        return record && record->pendingChange;
    }

    void FlowCV_Properties::Sync()
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        for (auto& record : props_) {
            if (record.pendingChange) {
                record.readValue = record.writeValue;
                record.pendingChange = false;
            }
        }
    }

    std::optional<PropertyValue> FlowCV_Properties::ReadValue(const std::string& key, ValueField field) const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        const auto* record = FindUnlocked(key);
        if (!record)
            return std::nullopt;
        if (field == ValueField::Read)
            return record->readValue;
        if (field == ValueField::Write)
            return record->writeValue;

        if (const auto* range = std::get_if<IntRange>(&record->range)) {
            if (field == ValueField::Step)
                return PropertyValue{range->step};
            return PropertyValue{field == ValueField::Min ? range->min : range->max};
        }
        if (const auto* range = std::get_if<FloatRange>(&record->range)) {
            if (field == ValueField::Step)
                return PropertyValue{range->step};
            return PropertyValue{field == ValueField::Min ? range->min : range->max};
        }
        if (record->type == PropertyDataTypes::kDataTypeOption && field != ValueField::Step)
            return PropertyValue{field == ValueField::Min ? 0 : static_cast<int>(record->options.size() - 1)};
        return std::nullopt;
    }

    std::vector<std::string> FlowCV_Properties::GetOptions(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        const auto* record = FindUnlocked(key);
        if (!record)
            return {};
        if (record->type != PropertyDataTypes::kDataTypeOption)
            throw std::invalid_argument("Property is not an option: " + key);
        return record->options;
    }

    void FlowCV_Properties::Set(const std::string& key, bool value)
    {
        SetValue(key, PropertyValue{value});
    }

    void FlowCV_Properties::Set(const std::string& key, int value)
    {
        SetValue(key, PropertyValue{value});
    }

    void FlowCV_Properties::Set(const std::string& key, float value)
    {
        SetValue(key, PropertyValue{value});
    }

    void FlowCV_Properties::SetValue(const std::string& key, const PropertyValue& value)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (!record)
            return;
        ValidateValue(*record, value);
        if (record->writeValue != value) {
            record->writeValue = value;
            ++revision_;
        }
        record->pendingChange = true;
    }

    void FlowCV_Properties::SetMin(const std::string& key, int value)
    {
        SetBoundary(key, PropertyValue{value}, true);
    }

    void FlowCV_Properties::SetMax(const std::string& key, int value)
    {
        SetBoundary(key, PropertyValue{value}, false);
    }

    void FlowCV_Properties::SetMin(const std::string& key, float value)
    {
        SetBoundary(key, PropertyValue{value}, true);
    }

    void FlowCV_Properties::SetMax(const std::string& key, float value)
    {
        SetBoundary(key, PropertyValue{value}, false);
    }

    void FlowCV_Properties::SetBoundary(const std::string& key, const PropertyValue& value, bool minimum)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (!record)
            return;
        if (auto* range = std::get_if<IntRange>(&record->range)) {
            const auto* number = std::get_if<int>(&value);
            if (!number)
                throw std::invalid_argument("Property boundary type mismatch: " + key);
            auto updated = *range;
            (minimum ? updated.min : updated.max) = *number;
            ValidateRange(updated);
            if (range->min != updated.min || range->max != updated.max) {
                *range = updated;
                ++revision_;
            }
        }
        else if (auto* range = std::get_if<FloatRange>(&record->range)) {
            const auto* number = std::get_if<float>(&value);
            if (!number)
                throw std::invalid_argument("Property boundary type mismatch: " + key);
            auto updated = *range;
            (minimum ? updated.min : updated.max) = *number;
            ValidateRange(updated);
            if (range->min != updated.min || range->max != updated.max) {
                *range = updated;
                ++revision_;
            }
        }
        else {
            throw std::invalid_argument("Property has no editable numeric range: " + key);
        }
    }

    void FlowCV_Properties::SetStep(const std::string& key, float value)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (!record)
            return;
        ValidateStep(value);
        float* step = nullptr;
        if (auto* range = std::get_if<IntRange>(&record->range))
            step = &range->step;
        else if (auto* range = std::get_if<FloatRange>(&record->range))
            step = &range->step;
        else
            throw std::invalid_argument("Property has no numeric step: " + key);
        if (*step != value) {
            *step = value;
            ++revision_;
        }
    }

    void FlowCV_Properties::SetVisibility(const std::string& key, bool show)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (record && record->visible != show) {
            record->visible = show;
            ++revision_;
        }
    }

    void FlowCV_Properties::SetDescription(const std::string& key, const std::string& desc)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (record && record->desc != desc) {
            record->desc = desc;
            ++revision_;
        }
    }

    void FlowCV_Properties::SetToDefault(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        auto* record = FindUnlocked(key);
        if (!record)
            return;
        if (record->writeValue != record->defaultValue) {
            record->writeValue = record->defaultValue;
            ++revision_;
        }
        record->pendingChange = true;
    }

    void FlowCV_Properties::SetAllToDefault()
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        bool changed = false;
        for (auto& record : props_) {
            if (record.writeValue != record.defaultValue) {
                record.writeValue = record.defaultValue;
                changed = true;
            }
            record.pendingChange = true;
        }
        if (changed)
            ++revision_;
    }

    std::uint64_t FlowCV_Properties::Revision() const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        return revision_;
    }

    PropertiesSnapshot FlowCV_Properties::Snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        PropertiesSnapshot snapshot{revision_, {}};
        snapshot.properties.reserve(props_.size());
        for (const auto& record : props_) {
            snapshot.properties.push_back({record.key, record.desc, record.type,
                record.writeValue, record.defaultValue, record.range, record.options, record.visible});
        }
        return snapshot;
    }

    void FlowCV_Properties::ToJson(nlohmann::json& j) const
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        for (const auto& record : props_) {
            if (record.visible)
                std::visit([&](auto value) { j[record.key] = value; }, record.writeValue);
        }
    }

    void FlowCV_Properties::FromJson(const nlohmann::json& j)
    {
        std::lock_guard<std::mutex> lock(mutex_lock_);
        // Preserve the old no-op behavior when no named fields are present.
        if (!j.is_object())
            return;

        std::vector<std::pair<std::size_t, PropertyValue>> updates;
        updates.reserve(props_.size());
        for (std::size_t i = 0; i < props_.size(); ++i) {
            const auto& record = props_[i];
            const auto it = j.find(record.key);
            if (it == j.end())
                continue;
            PropertyValue value;
            switch (record.type) {
            case PropertyDataTypes::kDataTypeBool:
                if (!it->is_boolean())
                    throw std::invalid_argument("Property requires a JSON boolean: " + record.key);
                value = it->get<bool>();
                break;
            case PropertyDataTypes::kDataTypeInt:
            case PropertyDataTypes::kDataTypeOption:
                value = ParseInteger(*it);
                break;
            case PropertyDataTypes::kDataTypeFloat:
                value = ParseFloat(*it);
                break;
            default:
                throw std::invalid_argument("Unsupported property type: " + record.key);
            }
            ValidateValue(record, value);
            updates.emplace_back(i, value);
        }

        // All parsing and allocation precede the non-throwing scalar commit.
        bool changed = false;
        for (const auto& [index, value] : updates) {
            auto& record = props_[index];
            changed = changed || record.writeValue != value;
            record.writeValue = value;
            record.readValue = value;
        }
        if (changed)
            ++revision_;
    }
} // namespace FlowCV
