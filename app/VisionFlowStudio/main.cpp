#include <exception>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QCommandLineParser>
#include <QtCore/QStandardPaths>
#include <logging/logger.h>
#include "StudioSettings.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>

#include <flowcv_manager.h>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("VisionFlowStudio"));
	QApplication::setOrganizationName(QStringLiteral("VisionFlow"));

	try {
		QCommandLineParser parser;
		parser.addHelpOption();
		QCommandLineOption configOption({"c", "cfg"}, "Default Config File Override", "path");
		parser.addOption(configOption);
		parser.process(app);
		const QString configPath = parser.isSet(configOption) ? parser.value(configOption)
			: QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/flowcv_editor.cfg";
		FlowCV::FlowLogger logger;
		auto settings = StudioSettings::load(configPath);
		settings.applyLogLevel();
		// Manager必须比主窗口、画布和图模型存活更久
		FlowCV::FlowCV_Manager manager;

		// ------------------- Load Dynamic Plugin ---------------------
		const QString pluginDirectory = QDir(QCoreApplication::applicationDirPath())
			.filePath(QStringLiteral("Plugins"));

		settings.configureRuntime(manager, pluginDirectory);


		// -------------------- Create Main Window ------------------------
		MainWindow window(manager, nullptr, &settings);

		// 与原版FlowCV一致，使用DSPatch内部自动执行机制
		manager.StartAutoTick();

		window.show();
		if (!settings.loadError.isEmpty())
			window.statusBar()->showMessage(
				QStringLiteral("Could not load settings; using defaults: %1").arg(settings.loadError), 15000);

		const int result = app.exec();

		// new add
		manager.StopAutoTick();

		return result;
	}
	catch (std::exception const& error) {

		QMessageBox::critical(
			nullptr, 
			QStringLiteral("VisionFlowStudio"),
			QString::fromUtf8(error.what()));

		return 1;
	}
}
