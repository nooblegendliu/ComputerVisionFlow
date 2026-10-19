#include "ThemeManager.h"
#include "ThemeStyleSheet.h"
#include "GraphTheme.h"
#include <QtCore/QThread>
#include <QtGui/QStyleHints>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>

// Keep the resource initializer in the global namespace for static linking.
static void initializeThemeResources() { Q_INIT_RESOURCE(theme); }

namespace VisionFlow {
ThemeManager::ThemeManager(QObject* parent, SystemSchemeReader reader)
    : QObject(parent), systemScheme_(std::move(reader))
{
    initializeThemeResources();
    if (!systemScheme_)
        systemScheme_ = [] { return QGuiApplication::styleHints()->colorScheme(); };
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        this, &ThemeManager::refreshSystemTheme);
    apply();
}

ThemeManager& ThemeManager::instance()
{
    static auto* manager = new ThemeManager(qApp);
    return *manager;
}

void ThemeManager::setMode(ThemeMode mode)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    const bool changed = mode_ != mode;
    mode_ = mode;
    apply();
    if (changed) Q_EMIT modeChanged(mode_);
}

void ThemeManager::refreshSystemTheme()
{
    if (mode_ == ThemeMode::System && !applying_) apply();
}

void ThemeManager::apply()
{
    if (applying_) return;
    applying_ = true;
    auto* hints = QGuiApplication::styleHints();
    if (mode_ == ThemeMode::System) hints->unsetColorScheme();
    else hints->setColorScheme(mode_ == ThemeMode::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

    auto next = mode_;
    if (next == ThemeMode::System) {
        const auto system = systemScheme_();
        next = system == Qt::ColorScheme::Dark ? ThemeMode::Dark
            : system == Qt::ColorScheme::Light ? ThemeMode::Light : effective_;
    }
    if (applied_ && effective_ == next) { applying_ = false; return; }
    effective_ = next;
    const auto c = colors();
    QPalette palette;
    palette.setColor(QPalette::Window, c.window);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.input);
    palette.setColor(QPalette::AlternateBase, c.raised);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::Button, c.raised);
    palette.setColor(QPalette::ButtonText, c.text);
    palette.setColor(QPalette::Highlight, c.selection);
    palette.setColor(QPalette::HighlightedText, c.selectionText);
    palette.setColor(QPalette::ToolTipBase, c.panel);
    palette.setColor(QPalette::ToolTipText, c.text);
    palette.setColor(QPalette::PlaceholderText, c.secondary);
    palette.setColor(QPalette::Link, c.accent);
    palette.setColor(QPalette::LinkVisited, c.accent);
    palette.setColor(QPalette::Light, c.raised.lighter(120));
    palette.setColor(QPalette::Midlight, c.raised);
    palette.setColor(QPalette::Mid, c.border);
    palette.setColor(QPalette::Dark, c.border);
    palette.setColor(QPalette::Shadow, c.border.darker(150));
    palette.setColor(QPalette::Accent, c.accent);
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText,
            QPalette::PlaceholderText}) palette.setColor(QPalette::Disabled, role, c.disabled);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, c.raised);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, c.disabled);
    QApplication::setPalette(palette);
    applyGraphTheme(c);
    qApp->setStyleSheet(themeStyleSheet(c));
    applied_ = true;
    applying_ = false;
    Q_EMIT themeChanged();
}
} // namespace VisionFlow
