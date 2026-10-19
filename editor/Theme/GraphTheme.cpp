#include "GraphTheme.h"
#include <QtNodes/StyleCollection>
#include <QtCore/QJsonObject>

namespace VisionFlow {
void applyGraphTheme(const ThemeColors& c)
{
    auto node = QtNodes::StyleCollection::nodeStyle();
    node.setBackgroundColor(c.node);
    node.NormalBoundaryColor = c.nodeBorder;
    node.SelectedBoundaryColor = c.accent;
    node.FontColor = c.text;
    node.FontColorFaded = c.secondary;
    node.ConnectionPointColor = c.accent;
    node.FilledConnectionPointColor = c.accent;
    node.ErrorColor = c.error;
    node.WarningColor = c.warning;
    node.PenWidth = 1.0f;
    node.HoveredPenWidth = 1.7f;
    node.Opacity = 1.0f;
    node.ShadowEnabled = false;
    QtNodes::StyleCollection::setNodeStyle(node);
    auto view = QtNodes::StyleCollection::flowViewStyle();
    view.BackgroundColor = c.canvas;
    view.FineGridColor = c.gridFine;
    view.CoarseGridColor = c.gridCoarse;
    QtNodes::StyleCollection::setGraphicsViewStyle(view);
    auto connection = QtNodes::StyleCollection::connectionStyle();
    auto json = connection.toJson();
    auto values = json["ConnectionStyle"].toObject();
    values["NormalColor"] = c.connection.name();
    values["ConstructionColor"] = c.secondary.name();
    values["SelectedColor"] = c.connectionSelected.name();
    values["SelectedHaloColor"] = c.accent.name();
    values["HoveredColor"] = c.accent.name();
    json["ConnectionStyle"] = values;
    connection.loadJson(json);
    QtNodes::StyleCollection::setConnectionStyle(connection);
}
} // namespace VisionFlow
