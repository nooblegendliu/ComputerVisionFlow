#ifndef FLOWCV_PROPERTY_MANAGER_HPP_
#define FLOWCV_PROPERTY_MANAGER_HPP_

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

namespace FlowCV
{
    enum class PropertyDataTypes
    {
        kDataTypeUndefined = 0,
        kDataTypeBool,
        kDataTypeInt,
        kDataTypeFloat,
        kDataTypeOption,
        kDataTypeSeparator
    };

    using PropertyValue = std::variant<bool, int, float>;

    struct IntRange
    {
        int min;
        int max;
        // Editing/drag-speed hint, not an integer increment or quantization rule.
        float step;
    };

    struct FloatRange
    {
        float min;
        float max;
        float step;
    };

    using PropertyRange = std::variant<std::monostate, IntRange, FloatRange>;

    struct PropertySnapshot
    {
        std::string key;
        std::string desc;
        PropertyDataTypes type;
        PropertyValue value; // Editable value, not the value last applied by Sync().
        PropertyValue defaultValue;
        PropertyRange range;
        std::vector<std::string> options;
        bool visible;
    };

    struct PropertiesSnapshot
    {
        std::uint64_t revision;
        std::vector<PropertySnapshot> properties;
    };

    // UI-independent storage. Each public operation is synchronized; multiple
    // separate reads do not constitute one processing-cycle snapshot.
    class FlowCV_Properties
    {
    public:
        FlowCV_Properties() = default;

        void AddBool(const std::string& key, const std::string& desc, bool value, bool visible = true);
        void AddInt(const std::string& key, const std::string& desc, int value,
            int min = 0, int max = 100, float step = 0.5f, bool visible = true);
        void AddFloat(const std::string& key, const std::string& desc, float value,
            float min = 0.0f, float max = 100.0f, float step = 0.1f, bool visible = true);
        void AddOption(const std::string& key, const std::string& desc, int value,
            std::vector<std::string> options, bool visible = true);
        void Remove(const std::string& key);
        void RemoveAll();

        // Missing keys are ignored. Existing keys require an exact value type
        // (options use int). Numeric bounds are UI hints, not Set() clamps.
        void Set(const std::string& key, bool value);
        void Set(const std::string& key, int value);
        void Set(const std::string& key, float value);
        void SetMin(const std::string& key, int value);
        void SetMax(const std::string& key, int value);
        void SetMin(const std::string& key, float value);
        void SetMax(const std::string& key, float value);
        void SetStep(const std::string& key, float value);
        void SetVisibility(const std::string& key, bool show);
        void SetDescription(const std::string& key, const std::string& desc);
        void SetToDefault(const std::string& key);
        void SetAllToDefault();

        bool Exists(const std::string& key) const;
        // Pending processing change, consumed by Sync(); not a UI dirty flag.
        bool Changed(const std::string& key) const;
        void Sync();

        // Missing keys return T{}. Mismatched existing values throw
        // std::invalid_argument. Only bool, int and float are supported.
        template<typename T> T Get(const std::string& key) const;
        template<typename T> T GetW(const std::string& key) const;
        // No range/step returns T{}. Options expose int bounds [0, size - 1].
        // Both numeric property types expose their step as float.
        template<typename T> T GetMin(const std::string& key) const;
        template<typename T> T GetMax(const std::string& key) const;
        template<typename T> T GetStep(const std::string& key) const;
        std::vector<std::string> GetOptions(const std::string& key) const;

        // Revisions are local to this instance. Always take a snapshot when
        // binding a different instance. Sync() never consumes a UI revision.
        std::uint64_t Revision() const;
        PropertiesSnapshot Snapshot() const;

        // Save visible editable values without clearing unrelated JSON fields.
        void ToJson(nlohmann::json& j) const;
        // Validate before committing; apply to both editable and processing
        // values immediately, preserving all pre-existing pending-change flags.
        void FromJson(const nlohmann::json& j);

    private:
        struct PropertyRecord
        {
            std::string key;
            std::string desc;
            PropertyDataTypes type;
            PropertyValue defaultValue;
            PropertyValue writeValue;
            PropertyValue readValue;
            PropertyRange range;
            std::vector<std::string> options;
            bool visible = true;
            bool pendingChange = false;
        };

        enum class ValueField { Read, Write, Min, Max, Step };

        void AddRecord(PropertyRecord record);
        void SetValue(const std::string& key, const PropertyValue& value);
        void SetBoundary(const std::string& key, const PropertyValue& value, bool minimum);
        std::optional<PropertyValue> ReadValue(const std::string& key, ValueField field) const;
        template<typename T> T ReadAs(const std::string& key, ValueField field) const;

        // Find helpers require mutex_lock_ to be held by the caller.
        PropertyRecord* FindUnlocked(const std::string& key);
        const PropertyRecord* FindUnlocked(const std::string& key) const;
        static void ValidateValue(const PropertyRecord& record, const PropertyValue& value);

        std::vector<PropertyRecord> props_;
        std::unordered_map<std::string, std::size_t> prop_idx_;
        std::uint64_t revision_ = 0;
        mutable std::mutex mutex_lock_;
    };

    template<typename T>
    T FlowCV_Properties::ReadAs(const std::string& key, ValueField field) const
    {
        static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float>,
            "Properties support only bool, int and float");
        const auto value = ReadValue(key, field);
        if (!value)
            return T{};
        if (const auto* typed = std::get_if<T>(&*value))
            return *typed;
        throw std::invalid_argument("Property value type mismatch: " + key);
    }

    template<typename T> T FlowCV_Properties::Get(const std::string& key) const
    {
        return ReadAs<T>(key, ValueField::Read);
    }

    template<typename T> T FlowCV_Properties::GetW(const std::string& key) const
    {
        return ReadAs<T>(key, ValueField::Write);
    }

    template<typename T> T FlowCV_Properties::GetMin(const std::string& key) const
    {
        return ReadAs<T>(key, ValueField::Min);
    }

    template<typename T> T FlowCV_Properties::GetMax(const std::string& key) const
    {
        return ReadAs<T>(key, ValueField::Max);
    }

    template<typename T> T FlowCV_Properties::GetStep(const std::string& key) const
    {
        return ReadAs<T>(key, ValueField::Step);
    }
} // namespace FlowCV

#endif
