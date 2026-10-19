#include "StudioSettings.h"
#include <flowcv_manager.h>
#include <logging/logger.h>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <stdexcept>

namespace {
std::string Utf8(const QString& value) { return value.toUtf8().toStdString(); }
int ReadInt(const nlohmann::json& j, const char* key, int fallback, int minimum, int maximum)
{
    if (!j.contains(key)) return fallback;
    const auto& value = j.at(key);
    if (!value.is_number_integer() || value < minimum || value > maximum)
        throw std::invalid_argument(std::string("Invalid setting: ") + key);
    return value.get<int>();
}
QStringList ReadPaths(const nlohmann::json& j, const char* key)
{
    QStringList paths;
    if (!j.contains(key)) return paths;
    if (!j.at(key).is_array()) throw std::invalid_argument(std::string("Invalid setting: ") + key);
    for (const auto& value : j.at(key)) paths.append(QString::fromStdString(value.get<std::string>()));
    return paths;
}
}
StudioSettings StudioSettings::load(const QString& path)
{
    StudioSettings settings;
    settings.configPath = path;
    try {
        QFile file(path);
        if (!file.exists()) return settings;
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(Utf8(file.errorString()));
        const auto bytes = file.readAll();
        const auto j = nlohmann::json::parse(bytes.constBegin(), bytes.constEnd());
        if (!j.is_object()) throw std::invalid_argument("Settings must be a JSON object");
        settings.extra_ = j;
        settings.recentLimit = ReadInt(j, "recent_size", 8, 1, 1000);
        settings.bufferCount = ReadInt(j, "buffer_count", 1, 1, 256);
        settings.logLevel = ReadInt(j, "log_level", 1, 0, 3);
        settings.showFps = j.value("show_fps", false);
        settings.pluginPaths = ReadPaths(j, "ext_plugin_dir");
        settings.recentFiles = ReadPaths(j, "recent_files");
        while (settings.recentFiles.size() > settings.recentLimit) settings.recentFiles.removeFirst();
        return settings;
    }
    catch (const std::exception& error) {
        // Like FlowCV, a damaged config must not prevent the editor from opening.
        // Discard partially read values so dependent settings remain consistent.
        StudioSettings defaults;
        defaults.configPath = path;
        defaults.loadError = QString::fromUtf8(error.what());
        LOG_ERROR("Unable to load settings '{}': {}. Using defaults.", Utf8(path), error.what());
        return defaults;
    }
}
void StudioSettings::save() const
{
    auto j = extra_;
    j["recent_size"] = recentLimit;
    j["buffer_count"] = bufferCount;
    j["log_level"] = logLevel;
    j["show_fps"] = showFps;
    j["ext_plugin_dir"] = nlohmann::json::array();
    j["recent_files"] = nlohmann::json::array();
    for (const auto& path : pluginPaths) j["ext_plugin_dir"].push_back(Utf8(path));
    for (const auto& path : recentFiles) j["recent_files"].push_back(Utf8(path));
    if (!QDir().mkpath(QFileInfo(configPath).absolutePath()))
        throw std::runtime_error("Cannot create settings directory");
    QSaveFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error(Utf8(file.errorString()));
    const auto text = j.dump(4) + "\n";
    if (file.write(text.data(), static_cast<qint64>(text.size())) != static_cast<qint64>(text.size()) || !file.commit())
        throw std::runtime_error(Utf8(file.errorString()));
}
void StudioSettings::recordRecent(const QString& path)
{
    recentFiles.append(QFileInfo(path).absoluteFilePath());
    while (recentFiles.size() > recentLimit) recentFiles.removeFirst();
    save();
}
void StudioSettings::applyLogLevel() const
{
    constexpr FlowCV::FlowLogger::Level levels[] = {FlowCV::FlowLogger::debug,
        FlowCV::FlowLogger::info, FlowCV::FlowLogger::warn, FlowCV::FlowLogger::off};
    FlowCV::FlowLogger::setLevel(levels[logLevel]);
}
void StudioSettings::configureRuntime(FlowCV::FlowCV_Manager& manager, const QString& defaultPluginDir) const
{
    manager.SetBufferCount(bufferCount > 1 ? bufferCount : 0);
    const auto nativeDefault = QDir::toNativeSeparators(defaultPluginDir).toLocal8Bit();
    manager.plugin_manager_->LoadPlugins(nativeDefault.constData(), true);
    for (const auto& path : pluginPaths) {
        const auto native = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath()).toLocal8Bit();
        manager.plugin_manager_->LoadPlugins(native.constData(), false);
    }
}
bool EditStudioSettings(StudioSettings& settings, QWidget* parent)
{
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("StudioSettingsDialog"));
    dialog.setWindowTitle(QStringLiteral("Application Settings"));
    auto* layout = new QFormLayout(&dialog);
    auto* recent = new QSpinBox(&dialog);
    recent->setObjectName(QStringLiteral("RecentLimit"));
    recent->setRange(1, 1000); recent->setValue(settings.recentLimit);
    layout->addRow(QStringLiteral("Recent File History Size"), recent);
    auto* buffers = new QSpinBox(&dialog);
    buffers->setObjectName(QStringLiteral("FlowBufferCount"));
    buffers->setRange(1, 256); buffers->setValue(settings.bufferCount);
    layout->addRow(QStringLiteral("Flow Buffer Count"), buffers);
    auto* level = new QComboBox(&dialog);
    level->addItems({"Debug", "Info", "Warning", "Off"}); level->setCurrentIndex(settings.logLevel);
    layout->addRow(QStringLiteral("Log Level"), level);
    auto* fps = new QCheckBox(QStringLiteral("Show UI FPS"), &dialog);
    fps->setChecked(settings.showFps); layout->addRow(fps);
    auto* paths = new QListWidget(&dialog);
    paths->addItems(settings.pluginPaths); layout->addRow(QStringLiteral("Extra Plugin Paths"), paths);
    auto* pathButtons = new QHBoxLayout;
    auto* add = new QPushButton(QStringLiteral("Add Plugin Path"), &dialog);
    auto* remove = new QPushButton(QStringLiteral("Remove Selected Path"), &dialog);
    pathButtons->addWidget(add); pathButtons->addWidget(remove); layout->addRow(pathButtons);
    QObject::connect(add, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getExistingDirectory(&dialog, QStringLiteral("Select Plugin Directory"));
        if (!path.isEmpty()) paths->addItem(path);
    });
    QObject::connect(remove, &QPushButton::clicked, &dialog, [&] { delete paths->takeItem(paths->currentRow()); });
    layout->addRow(new QLabel(QStringLiteral("Plugin paths and buffer count take effect after restarting Studio."), &dialog));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto updated = settings;
        updated.recentLimit = recent->value(); updated.bufferCount = buffers->value();
        updated.logLevel = level->currentIndex(); updated.showFps = fps->isChecked();
        updated.pluginPaths.clear();
        for (int i = 0; i < paths->count(); ++i) updated.pluginPaths.append(paths->item(i)->text());
        while (updated.recentFiles.size() > updated.recentLimit) updated.recentFiles.removeFirst();
        try { updated.save(); settings = std::move(updated); settings.applyLogLevel(); dialog.accept(); }
        catch (const std::exception& error) { QMessageBox::warning(&dialog, "Settings Error", QString::fromUtf8(error.what())); }
    });
    return dialog.exec() == QDialog::Accepted;
}
