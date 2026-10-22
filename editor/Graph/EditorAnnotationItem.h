#pragma once

#include <cstdint>
#include <functional>
#include <utility>

#include <QtCore/QPoint>
#include <QtCore/QSizeF>
#include <QtCore/QVariant>
#include <QtGui/QPainterPath>
#include <QtCore/QString>
#include <QtWidgets/QGraphicsObject>

class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

// Editor-only decoration. No DSPatch/FlowCV node is created for this item.
class EditorAnnotationItem final : public QGraphicsObject
{
public:
    enum class Kind { Group, Comment };
    using ModifiedHandler = std::function<void()>;
    using MenuHandler = std::function<void(EditorAnnotationItem*, const QPoint&)>;
    using EditHandler = std::function<void(EditorAnnotationItem*)>;

    EditorAnnotationItem(Kind kind, std::uint64_t uid, QString text,
                         QSizeF size, QGraphicsItem* parent = nullptr);

    Kind kind() const noexcept { return kind_; }
    std::uint64_t uid() const noexcept { return uid_; }
    const QString& text() const noexcept { return text_; }
    QSizeF annotationSize() const noexcept { return size_; }

    void setText(const QString& value);
    void setAnnotationSize(QSizeF value);
    void setModifiedHandler(ModifiedHandler handler) { modified_ = std::move(handler); }
    void setMenuHandler(MenuHandler handler) { menu_ = std::move(handler); }
    void setEditHandler(EditHandler handler) { edit_ = std::move(handler); }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    static QSizeF minimumSize(Kind kind);
    QRectF resizeHandle() const;
    QRectF titleArea() const;
    bool hitResizeHandle(const QPointF& point) const;
    void changed();

    Kind kind_;
    std::uint64_t uid_;
    QString text_;
    QSizeF size_;
    bool resizing_ = false;
    QPointF dragStartScene_;
    QPointF pressStartPos_;
    QSizeF pressStartSize_;
    ModifiedHandler modified_;
    MenuHandler menu_;
    EditHandler edit_;
};
