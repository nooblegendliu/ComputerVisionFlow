#include "EditorAnnotationItem.h"

#include <algorithm>
#include <utility>

#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPalette>
#include <QtGui/QPolygonF>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsSceneContextMenuEvent>
#include <QtWidgets/QGraphicsSceneHoverEvent>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QStyleOptionGraphicsItem>

namespace {
constexpr qreal kHeaderHeight = 34.0;
constexpr qreal kBorderHit = 7.0;
constexpr qreal kHandleWidth = 17.0;
}

EditorAnnotationItem::EditorAnnotationItem(
    Kind kind, std::uint64_t uid, QString text, QSizeF size, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , kind_(kind)
    , uid_(uid)
    , text_(std::move(text))
    , size_(size.expandedTo(minimumSize(kind)))
{
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setZValue(kind_ == Kind::Group ? -100.0 : -50.0);
    setToolTip(kind_ == Kind::Group
        ? QStringLiteral("Drag the title to move; drag the corner to resize; double-click to rename.")
        : QStringLiteral("Drag to move; drag the corner to resize; double-click to edit text."));
}

QSizeF EditorAnnotationItem::minimumSize(Kind kind)
{
    return kind == Kind::Group ? QSizeF(180.0, 100.0) : QSizeF(160.0, 70.0);
}

QRectF EditorAnnotationItem::boundingRect() const
{
    return QRectF(QPointF(0.0, 0.0), size_);
}

QRectF EditorAnnotationItem::titleArea() const
{
    return QRectF(0, 0, size_.width(), kHeaderHeight);
}

QRectF EditorAnnotationItem::resizeHandle() const
{
    return QRectF(size_.width() - kHandleWidth,
                  size_.height() - kHandleWidth,
                  kHandleWidth, kHandleWidth);
}

bool EditorAnnotationItem::hitResizeHandle(const QPointF& point) const
{
    return resizeHandle().contains(point);
}

QPainterPath EditorAnnotationItem::shape() const
{
    QPainterPath path;
    const QRectF bounds = boundingRect();
    if (kind_ == Kind::Comment) {
        path.addRoundedRect(bounds, 8.0, 8.0);
    } else {
        // Only Group title, border and resize handle take mouse hits.
        // Interior stays transparent for selecting/moving FlowCV nodes and wires.
        path.addRect(titleArea());
        path.addRect(QRectF(0, 0, kBorderHit, bounds.height()));
        path.addRect(QRectF(bounds.width() - kBorderHit, 0,
                            kBorderHit, bounds.height()));
        path.addRect(QRectF(0, bounds.height() - kBorderHit,
                            bounds.width(), kBorderHit));
        path.addRect(resizeHandle());
    }
    return path;
}

void EditorAnnotationItem::setText(const QString& value)
{
    if (text_ == value) return;
    text_ = value;
    update();
    changed();
}

void EditorAnnotationItem::setAnnotationSize(QSizeF value)
{
    value = value.expandedTo(minimumSize(kind_));
    if (value == size_) return;
    prepareGeometryChange();
    size_ = value;
    update();
}

void EditorAnnotationItem::paint(QPainter* painter,
                                  const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    const QColor fg = dark ? QColor(232, 236, 244) : QColor(37, 48, 63);
    const QColor accent = dark ? QColor(92, 175, 235) : QColor(35, 113, 189);
    const QRectF bounds = boundingRect();
    const QPen outline(isSelected() ? accent :
        (dark ? QColor(100, 140, 173, 190) : QColor(80, 127, 158, 160)),
        isSelected() ? 2.0 : 1.4,
        kind_ == Kind::Group ? Qt::DashLine : Qt::SolidLine);

    if (kind_ == Kind::Group) {
        painter->setPen(outline);
        painter->setBrush(dark ? QColor(71, 122, 159, 20) : QColor(72, 144, 195, 18));
        painter->drawRoundedRect(bounds.adjusted(1, 1, -1, -1), 8.0, 8.0);
        painter->setPen(Qt::NoPen);
        painter->setBrush(dark ? QColor(57, 92, 122, 160) : QColor(103, 155, 187, 120));
        painter->drawRoundedRect(titleArea().adjusted(1, 1, -1, 0), 8, 8);
        painter->setPen(fg);
        QFont font = painter->font();
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(titleArea().adjusted(12, 0, -24, 0),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(text_, Qt::ElideRight,
                                                             std::max(0, int(size_.width()-40))));
    } else {
        painter->setPen(outline);
        painter->setBrush(dark ? QColor(135, 114, 50, 112) : QColor(255, 229, 150, 200));
        painter->drawRoundedRect(bounds.adjusted(1, 1, -1, -1), 8.0, 8.0);
        painter->setPen(fg);
        const QRectF textRect = bounds.adjusted(12, 10, -12, -19);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text_);
    }

    painter->setPen(isSelected() ? accent : QColor(fg.red(), fg.green(), fg.blue(), 145));
    const QPointF corner(bounds.right()-5, bounds.bottom()-5);
    painter->drawLine(corner - QPointF(9, 0), corner - QPointF(0, 9));
    painter->drawLine(corner - QPointF(5, 0), corner - QPointF(0, 5));
    painter->restore();
}

QVariant EditorAnnotationItem::itemChange(GraphicsItemChange change,
                                           const QVariant& value)
{
    // Position changes are reported on mouse release, not every pixel.
    return QGraphicsObject::itemChange(change, value);
}

void EditorAnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        pressStartPos_ = pos();
        pressStartSize_ = size_;
        dragStartScene_ = event->scenePos();
        resizing_ = hitResizeHandle(event->pos());
        if (resizing_) {
            setSelected(true);
            event->accept();
            return;
        }
    }
    QGraphicsObject::mousePressEvent(event);
}

void EditorAnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (resizing_) {
        const QPointF delta = event->scenePos() - dragStartScene_;
        setAnnotationSize(QSizeF(pressStartSize_.width() + delta.x(),
                                 pressStartSize_.height() + delta.y()));
        event->accept();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void EditorAnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (resizing_ && event->button() == Qt::LeftButton) {
        resizing_ = false;
        if (size_ != pressStartSize_) changed();
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton && pos() != pressStartPos_) changed();
}

void EditorAnnotationItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (edit_) edit_(this);
    event->accept();
}

void EditorAnnotationItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(hitResizeHandle(event->pos())
        ? Qt::SizeFDiagCursor : Qt::OpenHandCursor);
    QGraphicsObject::hoverMoveEvent(event);
}

void EditorAnnotationItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    unsetCursor();
    QGraphicsObject::hoverLeaveEvent(event);
}

void EditorAnnotationItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if (menu_) menu_(this, event->screenPos());
    event->accept();
}

void EditorAnnotationItem::changed()
{
    if (modified_) modified_();
}
