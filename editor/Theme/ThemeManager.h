#pragma once

#include "ThemeColors.h"
#include <QtCore/QObject>
#include <functional>

namespace VisionFlow {
class ThemeManager final : public QObject {
    Q_OBJECT
public:
    using SystemSchemeReader = std::function<Qt::ColorScheme()>;
    explicit ThemeManager(QObject* parent = nullptr, SystemSchemeReader reader = {});
    static ThemeManager& instance();
    ThemeMode mode() const { return mode_; }
    ThemeMode effectiveMode() const { return effective_; }
    ThemeColors colors() const { return themeColors(effective_ == ThemeMode::Dark); }
    void setMode(ThemeMode mode);

public Q_SLOTS:
    void refreshSystemTheme();

Q_SIGNALS:
    void modeChanged(VisionFlow::ThemeMode mode);
    void themeChanged();

private:
    void apply();
    SystemSchemeReader systemScheme_;
    ThemeMode mode_ = ThemeMode::System;
    ThemeMode effective_ = ThemeMode::Dark;
    bool applied_ = false;
    bool applying_ = false;
};
} // namespace VisionFlow
