#include "NodeUiRegistry.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QThread>
#include <stdexcept>

namespace VisionFlow {
namespace {
class NodeUiHost final : public QWidget
{
public:
    NodeUiHost(DSPatch::Component::SPtr node, QWidget* parent)
        : QWidget(parent), node_(std::move(node)) {}
    ~NodeUiHost() override
    {
        // Delete plugin-defined widgets before releasing the node/module lease.
        while (!children().isEmpty()) delete children().first();
    }
private:
    DSPatch::Component::SPtr node_;
};
}
void NodeUiRegistry::registerFactory(const std::string& name, NodeUiRole role, Factory factory)
{
    factories_[{name, role}] = std::move(factory);
}
bool NodeUiRegistry::hasUi(const DSPatch::Component::SPtr& node, NodeUiRole role) const
{
    if (!node) return false;
    if (auto* provider = dynamic_cast<NodeUiProvider*>(node.get());
        provider && provider->hasNodeUi(role)) return true;
    return factories_.contains({node->GetComponentName(), role});
}
QWidget* NodeUiRegistry::create(const DSPatch::Component::SPtr& node, NodeUiRole role, QWidget* parent) const
{
    if (!qApp || QThread::currentThread() != qApp->thread())
        throw std::logic_error("Node widgets must be created on the GUI thread");
    if (!node) return nullptr;
    auto host = std::make_unique<NodeUiHost>(node, parent);
    QWidget* widget = nullptr;
    if (auto* provider = dynamic_cast<NodeUiProvider*>(node.get());
        provider && provider->hasNodeUi(role)) {
        widget = provider->createNodeUi(role, host.get());
    } else if (const auto it = factories_.find({node->GetComponentName(), role}); it != factories_.end()) {
        widget = it->second(node, host.get());
    }
    if (!widget) return nullptr;
    auto* layout = new QVBoxLayout(host.get());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return host.release();
}
}
