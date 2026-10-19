#include "StudioSettings.h"
#include "MainWindow.h"
#include <Theme/ThemeManager.h>
#include <Theme/ThemeStyleSheet.h>
#include <VisionFlowEditor.h>
#include <Graph/VisionFlowGraphModel.hpp>
#include <flowcv_manager.h>
#include <logging/logger.h>
#include <QtNodes/GraphicsView>
#include <QtNodes/BasicGraphicsScene>
#include <QtNodes/StyleCollection>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QSettings>
#include <QPushButton>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

using VisionFlow::ThemeMode;
using VisionFlow::ThemeManager;
namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
QByteArray Read(const QString& path) {
    QFile file(path); Check(file.open(QIODevice::ReadOnly), "Cannot read fixture"); return file.readAll();
}
void Write(const QString& path, const QByteArray& bytes) {
    QFile file(path); Check(file.open(QIODevice::WriteOnly), "Cannot write fixture");
    Check(file.write(bytes) == bytes.size(), "Incomplete fixture write");
}
double Luminance(QColor color) {
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}
double Contrast(QColor a, QColor b) {
    const auto x = Luminance(a), y = Luminance(b);
    return (std::max(x, y) + .05) / (std::min(x, y) + .05);
}
void ResolutionTest() {
    auto system = Qt::ColorScheme::Unknown;
    ThemeManager themes(nullptr, [&] { return system; });
    Check(themes.effectiveMode() == ThemeMode::Dark, "Unknown initial system scheme must fall back to dark");
    int changes = 0;
    QObject::connect(&themes, &ThemeManager::themeChanged, [&] { ++changes; });
    system = Qt::ColorScheme::Light; themes.refreshSystemTheme();
    Check(themes.effectiveMode() == ThemeMode::Light, "System light was not applied");
    system = Qt::ColorScheme::Unknown; themes.refreshSystemTheme();
    Check(themes.effectiveMode() == ThemeMode::Light && changes == 1, "Unknown system lost last effective theme");
    system = Qt::ColorScheme::Dark; themes.refreshSystemTheme();
    Check(themes.effectiveMode() == ThemeMode::Dark, "System dark was not applied");
    themes.setMode(ThemeMode::Light);
    themes.refreshSystemTheme();
    Check(themes.effectiveMode() == ThemeMode::Light, "Explicit light followed system");
    themes.setMode(ThemeMode::Dark);
    system = Qt::ColorScheme::Light; themes.refreshSystemTheme();
    Check(themes.effectiveMode() == ThemeMode::Dark, "Explicit dark followed system");
    themes.setMode(ThemeMode::System);
    Check(themes.effectiveMode() == ThemeMode::Light, "Returning to System used stale scheme");
    const auto unchanged = changes;
    themes.setMode(ThemeMode::System); themes.refreshSystemTheme();
    Check(changes == unchanged, "Unchanged theme was unnecessarily reapplied");
    for (auto mode : {ThemeMode::Dark, ThemeMode::Light}) {
        themes.setMode(mode);
        const auto c = themes.colors();
        Check(QApplication::palette().color(QPalette::Window) == c.window, "Application palette mismatch");
        Check(Contrast(c.text, c.panel) >= 4.5 && Contrast(c.secondary, c.panel) >= 4.5,
            "Text contrast is insufficient");
        Check(Contrast(c.selectionText, c.selection) >= 4.5, "Selected text contrast is insufficient");
        Check(Contrast(c.error, c.panel) >= 4.5, "Error text contrast is insufficient");
        Check(Contrast(c.connection, c.canvas) >= 3.0, "Connection contrast is insufficient");
        Check(!qApp->styleSheet().contains('@'), "Unresolved stylesheet token");
        const auto icon = QString(":/visionflow/theme/%1/spin-up.svg").arg(c.dark ? "dark" : "light");
        Check(QFile::exists(icon), "Theme icon resource missing");
    }
}
void PersistenceTest(const QTemporaryDir& temp) {
    const auto path = temp.filePath("theme.cfg");
    Check(StudioSettings::load(path).themeMode == ThemeMode::System, "New user must follow system");
    Write(path, R"({"buffer_count":3,"foreign_key":"preserved"})");
    Check(StudioSettings::load(path).themeMode == ThemeMode::Dark, "Legacy appearance must remain dark");
    for (const auto& value : {"17", "null", "\"invalid\"", "{}", "[]"}) {
        Write(path, QByteArray("{\"buffer_count\":3,\"theme_mode\":") + value + "}");
        const auto settings = StudioSettings::load(path);
        Check(settings.themeMode == ThemeMode::Dark && settings.bufferCount == 3 && settings.loadError.isEmpty(),
            "Invalid theme discarded unrelated settings");
    }
    Write(path, R"({"buffer_count":3,"foreign_key":"preserved"})");
    auto settings = StudioSettings::load(path);
    for (auto mode : {ThemeMode::System, ThemeMode::Dark, ThemeMode::Light}) {
        settings.themeMode = mode; settings.save();
        Check(StudioSettings::load(path).themeMode == mode, "Theme persistence failed");
        const auto json = nlohmann::json::parse(Read(path).toStdString());
        Check(json.at("foreign_key") == "preserved" && !json.contains("effective_theme"),
            "Theme save lost unknown settings or persisted derived state");
    }
}
void DialogTest(StudioSettings& settings, bool save, bool failSave = false) {
    auto& themes = ThemeManager::instance();
    themes.setMode(settings.themeMode);
    const auto before = failSave ? QByteArray() : Read(settings.configPath);
    const auto previous = settings.themeMode;
    const auto preview = previous == ThemeMode::Light ? ThemeMode::Dark : ThemeMode::Light;
    bool interacted = false, saveErrorSeen = false, timedOut = false;
    std::exception_ptr error;
    QTimer driver, watchdog, errorCloser;
    QObject::connect(&errorCloser, &QTimer::timeout, [&] {
        if (auto* message = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            saveErrorSeen = true; message->accept();
        }
    });
    errorCloser.start(10);
    QObject::connect(&driver, &QTimer::timeout, [&] {
        if (auto* message = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            saveErrorSeen = true; message->accept(); return;
        }
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog || interacted) return;
        interacted = true;
        try {
            auto* combo = dialog->findChild<QComboBox*>("ThemeMode");
            Check(combo, "Theme setting missing");
            combo->setCurrentIndex(combo->findData(int(preview)));
            Check(themes.mode() == preview && themes.effectiveMode() == preview, "Preview did not apply immediately");
            Check(settings.themeMode == previous, "Preview changed committed settings");
            if (!failSave) Check(Read(settings.configPath) == before, "Preview wrote configuration");
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            buttons->button(save ? QDialogButtonBox::Save : QDialogButtonBox::Cancel)->click();
            if (failSave) {
                Check(saveErrorSeen && dialog->isVisible(), "Save failure closed settings dialog");
                dialog->reject();
            }
        } catch (...) { error = std::current_exception(); dialog->reject(); }
    });
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [&] {
        timedOut = true;
        for (auto* widget : QApplication::topLevelWidgets())
            if (auto* dialog = qobject_cast<QDialog*>(widget)) dialog->reject();
    });
    driver.start(1); watchdog.start(5000);
    const bool accepted = EditStudioSettings(settings, nullptr);
    if (error) std::rethrow_exception(error);
    Check(interacted && !timedOut, "Theme dialog automation timed out");
    Check(accepted == (save && !failSave), "Unexpected settings result");
    if (accepted) {
        Check(settings.themeMode == preview && StudioSettings::load(settings.configPath).themeMode == preview,
            "Saved preview not persisted");
    } else {
        Check(settings.themeMode == previous && themes.mode() == previous, "Cancel did not restore theme choice");
        if (!failSave) Check(Read(settings.configPath) == before, "Cancel changed configuration");
        if (previous == ThemeMode::System) {
            const auto system = QGuiApplication::styleHints()->colorScheme();
            if (system != Qt::ColorScheme::Unknown)
                Check(themes.effectiveMode() == (system == Qt::ColorScheme::Dark ? ThemeMode::Dark : ThemeMode::Light),
                    "Cancel restored stale effective system theme");
        }
    }
}
void CanvasTest(const QString& plugin, const QString& artifacts) {
    auto& themes = ThemeManager::instance();
    FlowCV::FlowCV_Manager manager;
    manager.plugin_manager_->LoadPlugins(QFileInfo(plugin).absolutePath().toLocal8Bit().constData(), false);
    QWidget window;
    auto* layout = new QVBoxLayout(&window);
    auto* text = new QLineEdit("Theme preview", &window); layout->addWidget(text);
    auto* error = new QLabel("Example error", &window); error->setProperty("severity", "error"); layout->addWidget(error);
    auto* editor = new VisionFlowEditor(manager, &window); layout->addWidget(editor);
    window.resize(900, 600); window.show();
    const auto id = editor->addNode("Image_Loader");
    // Source nodes intentionally re-enable themselves on tick. Use a processing
    // node for the disabled appearance and runtime-state preservation check.
    const auto second = editor->addNode("Blur");
    auto* model = editor->graphModel();
    Check(model->nodeExists(id) && model->nodeExists(second), "Fixture plugin did not load");
    model->setNodeData(id, QtNodes::NodeRole::Position, QPointF(10, 20));
    model->setNodeData(second, QtNodes::NodeRole::Position, QPointF(280, 20));
    model->setNodeEnabled(second, false);
    auto* item = editor->graphicsScene()->nodeGraphicsObject(id);
    Check(item, "Missing graphics object");
    item->setSelected(true);
    const auto before = manager.GetState();
    const auto positions = editor->getEditorState();
    manager.StartAutoTick();
    try {
        for (int iteration = 0; iteration < 4; ++iteration) {
            const auto mode = iteration % 2 ? ThemeMode::Light : ThemeMode::Dark;
            themes.setMode(mode); QApplication::processEvents();
            const auto c = themes.colors();
            Check(editor->graphicsView()->backgroundBrush().color() == c.canvas, "Existing canvas retained stale background");
            Check(QtNodes::StyleCollection::nodeStyle().FontColor == c.text, "Node style did not update");
            Check(item == editor->graphicsScene()->nodeGraphicsObject(id) && item->isSelected(), "Theme rebuilt nodes or lost selection");
            Check(editor->getEditorState() == positions && manager.GetState() == before, "Theme changed flow or layout");
            const auto snapshot = window.grab();
            if (!artifacts.isEmpty() && iteration < 2) {
                QDir().mkpath(artifacts);
                Check(snapshot.save(artifacts + "/theme-" + VisionFlow::themeModeName(mode) + ".png"), "Cannot save theme preview");
            }
        }
        const auto newId = editor->addNode("Image_Loader");
        Check(model->nodeExists(newId), "Could not create node after switching");
    } catch (...) { manager.StopAutoTick(); throw; }
    manager.StopAutoTick();
}
void StudioTest(const QTemporaryDir& temp, const QString& plugin, const QString& artifacts) {
    auto settings = StudioSettings::load(temp.filePath("studio.cfg"));
    settings.themeMode = ThemeMode::Dark;
    settings.save();
    FlowCV::FlowCV_Manager manager;
    settings.configureRuntime(manager, QFileInfo(plugin).absolutePath());
    QDir pluginRoot(QFileInfo(plugin).absolutePath());
    Check(pluginRoot.cdUp(), "Cannot resolve plugin root");
    for (const auto* directory : {"ImageWriter", "ShapeColorizer"}) {
        const auto native = pluginRoot.filePath(directory).toLocal8Bit();
        manager.plugin_manager_->LoadPlugins(native.constData(), false);
    }
    MainWindow window(manager, nullptr, &settings);
    window.show();
    auto* editor = window.findChild<VisionFlowEditor*>();
    const auto source = editor->addNode("Image_Loader");
    const auto viewer = editor->addNode("Viewer");
    const auto writer = editor->addNode("Image_Writer");
    const auto colorizer = editor->addNode("Shape_Colorizer");
    auto* model = editor->graphModel();
    Check(model->nodeExists(source) && model->nodeExists(viewer), "Studio theme fixture nodes unavailable");
    Check(model->nodeExists(writer) && model->nodeExists(colorizer), "Custom Qt plugin fixture nodes unavailable");
    model->setNodeData(source, QtNodes::NodeRole::Position, QPointF(0, 0));
    model->setNodeData(colorizer, QtNodes::NodeRole::Position, QPointF(240, 0));
    model->setNodeData(viewer, QtNodes::NodeRole::Position, QPointF(500, 0));
    model->setNodeData(writer, QtNodes::NodeRole::Position, QPointF(240, 160));
    model->addConnection({source, 0, colorizer, 0});
    model->addConnection({colorizer, 0, viewer, 0});
    model->addConnection({source, 0, writer, 0});
    QApplication::processEvents();
    editor->zoomFitAll();
    const auto path = temp.filePath("theme-project.flow");
    Check(window.saveProjectToFile(path), "Could not save theme integration project");
    QApplication::processEvents();
    const auto runtime = manager.GetState();
    const auto positions = editor->getEditorState();
    const auto project = Read(path);
    const auto title = window.windowTitle();
    const auto docks = window.saveState();
    const auto transform = editor->graphicsView()->transform();
    manager.StartAutoTick();
    auto& themes = ThemeManager::instance();
    for (auto mode : {ThemeMode::Light, ThemeMode::Dark, ThemeMode::System}) {
        auto* action = window.findChild<QAction*>("Theme_" + VisionFlow::themeModeName(mode));
        Check(action, "Missing Theme menu action");
        action->trigger(); QApplication::processEvents();
        Check(action->isChecked() && themes.mode() == mode && settings.themeMode == mode,
            "Theme menu selection is out of sync");
        Check(StudioSettings::load(settings.configPath).themeMode == mode, "Theme menu was not persisted");
        Check(manager.IsAutoTickRunning() && manager.GetState() == runtime && editor->getEditorState() == positions,
            "Menu theme switch affected running flow");
        Check(Read(path) == project && window.windowTitle() == title, "Theme changed project file or dirty state");
        Check(window.saveState() == docks, "Theme changed dock layout");
        Check(editor->graphicsView()->transform() == transform, "Theme changed canvas zoom");
        auto* pluginError = window.findChild<QLabel*>("ShapeColorizerError");
        Check(pluginError && VisionFlow::themeColors(pluginError->palette()).dark == themes.colors().dark,
            "Qt plugin widget did not inherit theme");
        if (!artifacts.isEmpty() && mode != ThemeMode::System) {
            Check(window.grab().save(artifacts + "/studio-" + VisionFlow::themeModeName(mode) + ".png"),
                "Cannot save Studio theme preview");
        }
    }
    themes.setMode(ThemeMode::Light);
    Check(window.findChild<QAction*>("Theme_light")->isChecked(), "Menu did not reflect a settings preview");
    themes.setMode(settings.themeMode);
    const auto committed = settings.themeMode;
    const auto config = settings.configPath;
    settings.configPath = temp.path(); // An existing directory cannot be a config file.
    bool failureShown = false;
    QTimer errorCloser;
    QObject::connect(&errorCloser, &QTimer::timeout, [&] {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            failureShown = true; box->accept();
        }
    });
    errorCloser.start(10);
    window.findChild<QAction*>("Theme_light")->trigger();
    Check(failureShown && themes.mode() == committed && settings.themeMode == committed,
        "Failed menu save changed theme preference");
    Check(window.findChild<QAction*>("Theme_" + VisionFlow::themeModeName(committed))->isChecked(),
        "Failed menu save lost check mark");
    settings.configPath = config;
    manager.StopAutoTick();
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
#ifdef Q_OS_WIN
        QApplication::setFont(QFont(QStringLiteral("Segoe UI"), 9));
#endif
        Check(QFontMetrics(app.font()).inFont(QChar('A')), "Offscreen test font is unavailable");
        Check(argc >= 2, "Expected ImageLoader path");
        const auto plugin = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
        const auto artifacts = argc > 2 ? QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath() : QString();
        QTemporaryDir temp; Check(temp.isValid(), "Missing temporary directory");
        const auto oldPath = QDir::currentPath();
        struct Cleanup { QString path; ~Cleanup() { spdlog::shutdown(); QDir::setCurrent(path); } } cleanup{oldPath};
        Check(QDir::setCurrent(temp.path()), "Cannot isolate theme test logs");
        FlowCV::FlowLogger logger;
        QApplication::setOrganizationName("VisionFlowTests");
        QApplication::setApplicationName("ThemeTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp.path());
        ResolutionTest();
        PersistenceTest(temp);
        auto settings = StudioSettings::load(temp.filePath("dialog.cfg"));
        settings.themeMode = ThemeMode::Dark; settings.save();
        DialogTest(settings, false);
        DialogTest(settings, true);
        settings.themeMode = ThemeMode::System; settings.save(); DialogTest(settings, false);
        settings.configPath = temp.path(); DialogTest(settings, true, true);
        CanvasTest(plugin, artifacts);
        StudioTest(temp, plugin, artifacts);
        std::cout << "Theme resolution, persistence, preview/cancel/save failures and live canvas checks passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
