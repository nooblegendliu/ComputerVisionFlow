#pragma once

#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QPalette>

namespace VisionFlow {
enum class ThemeMode { System, Dark, Light };

inline QString themeModeName(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Dark: return QStringLiteral("dark");
    case ThemeMode::Light: return QStringLiteral("light");
    default: return QStringLiteral("system");
    }
}

// Header-only palette access also works in dynamically loaded Qt plugins.
struct ThemeColors {
    bool dark;
    QColor window, panel, input, raised, text, secondary, disabled, border;
    QColor accent, hover, pressed, selection, selectionText, error, warning;
    QColor canvas, gridFine, gridCoarse, node, nodeBorder, nodeDisabled;
    QColor connection, connectionSelected, flowIndicator;
};

inline ThemeColors themeColors(bool dark)
{
    if (dark) return {true,
        "#20242b", "#252c35", "#333c48", "#303946", "#dfe5ed", "#a4b3c1", "#687584", "#536070",
        "#44a6c3", "#405366", "#2c647a", "#35566a", "#ffffff", "#ff7773", "#e3a344",
        "#20262f", "#2b333d", "#394452", "#272e38", "#647485", "#262626",
        "#87aac8", "#ffaa28", "#dcf0ff"};
    return {false,
        "#f3f5f7", "#ffffff", "#ffffff", "#e8edf2", "#202833", "#596777", "#75818d", "#c6ced8",
        "#087f9c", "#dceaf0", "#bcdce7", "#cbe6f0", "#163d4b", "#ba2929", "#946000",
        "#edf1f5", "#e0e6ed", "#cbd5df", "#ffffff", "#8595a5", "#e1e5e9",
        "#486e8d", "#a55b00", "#075875"};
}

inline ThemeColors themeColors(const QPalette& palette)
{
    return themeColors(palette.color(QPalette::Window).lightness() < 128);
}
} // namespace VisionFlow
