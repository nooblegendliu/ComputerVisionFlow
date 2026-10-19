#pragma once

#include <flowcv_manager.h>
#include <QtCore/QElapsedTimer>
#include <QtGui/QPainterPathStroker>
#include <QtNodes/internal/AbstractConnectionPainter.hpp>
#include <QtNodes/internal/AbstractNodeGeometry.hpp>
#include <QtNodes/internal/DefaultNodePainter.hpp>
#include <QtNodes/internal/BasicGraphicsScene.hpp>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include "VisionFlowGraphModel.hpp"
#include "Theme/ThemeColors.h"
#include <QtWidgets/QApplication>
#include <cmath>

inline QColor categoryColor(DSPatch::Category category)
{
    using C = DSPatch::Category;
    switch (category) {
    case C::Category_Output: return {191,191,0,128};
    case C::Category_Draw: return {2,145,153,128};
    case C::Category_Color: return {122,168,255,128};
    case C::Category_Filter: return {153,61,99,128};
    case C::Category_Merge: return {76,94,198,128};
    case C::Category_Transform: return {179,76,25,128};
    case C::Category_Views: return {76,128,51,128};
    case C::Category_Feature_Detection: return {204,128,76,128};
    case C::Category_DNN: return {187,119,255,128};
    case C::Category_OpenVino: return {74,196,202,128};
    case C::Category_Utility: return {111,46,158,128};
    case C::Category_Other: case C::Category_Experimental: return {191,191,191,128};
    default: return {100,100,100,128};
    }
}

class FlowNodePainter final : public QtNodes::DefaultNodePainter
{
public:
    explicit FlowNodePainter(FlowCV::FlowCV_Manager& manager) : manager_(manager) {}
    void paint(QPainter* painter, QtNodes::NodeGraphicsObject& node) const override
    {
        using namespace QtNodes;
        painter->save();
        drawNodeRect(painter, node);
        auto& model = static_cast<VisionFlowGraphModel&>(node.graphModel());
        FlowCV::NodeInfo info{};
        const auto size = node.nodeScene()->nodeGeometry().size(node.nodeId());
        if (manager_.GetNodeInfoById(model.flowNodeId(node.nodeId()), info)) {
            painter->fillRect(QRectF(1, 1, size.width()-2,
                node.nodeScene()->nodeGeometry().captionRect(node.nodeId()).height()+8),
                [&] {
                    auto color = categoryColor(info.desc.category);
                    if (!VisionFlow::themeColors(QApplication::palette()).dark) color.setAlpha(55);
                    return color;
                }());
        }
        drawConnectionPoints(painter, node);
        drawFilledConnectionPoints(painter, node);
        drawNodeCaption(painter, node);
        drawEntryLabels(painter, node);
        if (info.node_ptr && !info.node_ptr->IsEnabled()) {
            painter->setPen(QPen(VisionFlow::themeColors(QApplication::palette()).warning, 1.5));
            painter->drawLine(QPointF(0,0), QPointF(size.width(),size.height()));
            painter->drawLine(QPointF(0,size.height()), QPointF(size.width(),0));
        }
        painter->restore();
    }
private:
    FlowCV::FlowCV_Manager& manager_;
};

// Direction indicators only: animation does not sample DSPatch traffic.
class FlowConnectionPainter final : public QtNodes::AbstractConnectionPainter
{
public:
    FlowConnectionPainter() { time_.start(); }
    void setAnimated(bool value) { animated_ = value; }
    void paint(QPainter* painter, const QtNodes::ConnectionGraphicsObject& c) const override
    {
        const auto path = curve(c);
        painter->save();
        painter->setBrush(Qt::NoBrush);
        const auto colors = VisionFlow::themeColors(QApplication::palette());
        painter->setPen(QPen(c.isSelected() ? colors.connectionSelected
            : c.connectionState().hovered() ? colors.accent : colors.connection,
            c.connectionState().hovered() ? 3.5 : 2.2));
        if (c.connectionState().requiresPort()) {
            auto pen = painter->pen(); pen.setStyle(Qt::DashLine); painter->setPen(pen);
        }
        painter->drawPath(path);
        if (animated_ && !c.connectionState().requiresPort()) {
            const double phase = std::fmod(time_.elapsed()/2000.0, 1.0);
            painter->setPen(Qt::NoPen);
            painter->setBrush(colors.flowIndicator);
            for (int i=0; i<3; ++i) {
                const double t = std::fmod(phase+i/3.0, 1.0);
                painter->save();
                painter->translate(path.pointAtPercent(t));
                painter->rotate(-path.angleAtPercent(t));
                painter->drawPolygon(QPolygonF{QPointF(5,0),QPointF(-4,-3),QPointF(-4,3)});
                painter->restore();
            }
        }
        painter->restore();
    }
    QPainterPath getPainterStroke(const QtNodes::ConnectionGraphicsObject& c) const override
    {
        QPainterPathStroker stroker;
        stroker.setWidth(10.0);
        return stroker.createStroke(curve(c));
    }
private:
    static QPainterPath curve(const QtNodes::ConnectionGraphicsObject& c)
    {
        QPainterPath path(c.out());
        auto controls = c.pointsC1C2();
        path.cubicTo(controls.first,controls.second,c.in());
        return path;
    }
    bool animated_ = false;
    QElapsedTimer time_;
};
