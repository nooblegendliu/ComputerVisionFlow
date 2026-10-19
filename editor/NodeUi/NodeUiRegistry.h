#pragma once
#include "NodeUiProvider.h"
#include <DSPatch.h>
#include <functional>
#include <map>
#include <string>

namespace VisionFlow {
class NodeUiRegistry
{
public:
    using Factory = std::function<QWidget*(const DSPatch::Component::SPtr&, QWidget*)>;
    void registerFactory(const std::string& name, NodeUiRole role, Factory factory);
    bool hasUi(const DSPatch::Component::SPtr& node, NodeUiRole role) const;
    QWidget* create(const DSPatch::Component::SPtr& node, NodeUiRole role, QWidget* parent) const;
private:
    std::map<std::pair<std::string, NodeUiRole>, Factory> factories_;
};
}
