#pragma once

class QWidget;

namespace VisionFlow {
enum class NodeUiRole { Controls, Main };

// Optional Qt-only interface. DSPatch's Create ABI and the core SDK stay unchanged.
// Widgets are constructed on the GUI thread. Their node stays alive until they die.
class NodeUiProvider
{
public:
    virtual ~NodeUiProvider() = default;
    virtual bool hasNodeUi(NodeUiRole role) const = 0;
    virtual QWidget* createNodeUi(NodeUiRole role, QWidget* parent) = 0;
};
}
