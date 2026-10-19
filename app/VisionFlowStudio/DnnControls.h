#pragma once
#include <memory>
namespace FlowCV { class Node; }
class QWidget;
QWidget* CreateDnnControls(std::shared_ptr<FlowCV::Node> node, QWidget* parent);
