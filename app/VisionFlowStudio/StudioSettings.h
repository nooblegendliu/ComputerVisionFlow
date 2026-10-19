#pragma once
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <nlohmann/json.hpp>
#include <Theme/ThemeColors.h>
class QWidget;
namespace FlowCV { class FlowCV_Manager; }

struct StudioSettings
{
    QString configPath;
    QString loadError; // Runtime feedback only; never serialized.
    QStringList pluginPaths;
    QStringList recentFiles; // Oldest first, as in FlowCV.
    int recentLimit = 8;
    int bufferCount = 1;
    int logLevel = 1; // Original config: debug=0, info=1, warning=2, off=3.
    bool showFps = false;
    VisionFlow::ThemeMode themeMode = VisionFlow::ThemeMode::System;

    static StudioSettings load(const QString& path);
    void save() const;
    void recordRecent(const QString& path);
    void applyLogLevel() const;
    void configureRuntime(FlowCV::FlowCV_Manager& manager, const QString& defaultPluginDir) const;
private:
    nlohmann::json extra_ = nlohmann::json::object();
};

bool EditStudioSettings(StudioSettings& settings, QWidget* parent);
