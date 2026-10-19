#include "ThemeStyleSheet.h"
#include <QtCore/QFile>

namespace VisionFlow {
QString themeStyleSheet(const ThemeColors& c)
{
    QFile file(QStringLiteral(":/visionflow/theme/widgets.qss"));
    if (!file.open(QIODevice::ReadOnly)) qFatal("Missing theme stylesheet resource");
    auto result = QString::fromUtf8(file.readAll());
    result.replace(QStringLiteral("@accent@"), c.accent.name());
    result.replace(QStringLiteral("@border@"), c.border.name());
    result.replace(QStringLiteral("@disabled@"), c.disabled.name());
    result.replace(QStringLiteral("@error@"), c.error.name());
    result.replace(QStringLiteral("@hover@"), c.hover.name());
    result.replace(QStringLiteral("@input@"), c.input.name());
    result.replace(QStringLiteral("@panel@"), c.panel.name());
    result.replace(QStringLiteral("@pressed@"), c.pressed.name());
    result.replace(QStringLiteral("@raised@"), c.raised.name());
    result.replace(QStringLiteral("@secondary@"), c.secondary.name());
    result.replace(QStringLiteral("@selection@"), c.selection.name());
    result.replace(QStringLiteral("@selectionText@"), c.selectionText.name());
    result.replace(QStringLiteral("@text@"), c.text.name());
    result.replace(QStringLiteral("@warning@"), c.warning.name());
    result.replace(QStringLiteral("@window@"), c.window.name());
    result.replace(QStringLiteral("@iconMode@"), c.dark ? QStringLiteral("dark") : QStringLiteral("light"));
    return result;
}
} // namespace VisionFlow
