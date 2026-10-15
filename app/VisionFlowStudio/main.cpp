#include <exception>

#include <QtCore/QString>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <flowcv_manager.h>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("VisionFlowStudio"));
	QApplication::setOrganizationName(QStringLiteral("VisionFlow"));

	try {
		// Manager必须比主窗口、画布和图模型存活更久
		FlowCV::FlowCV_Manager manager;

		manager.SetBufferCount(0);

		MainWindow window(manager);

		// 与原版FlowCV一致，使用DSPatch内部自动执行机制
		manager.StartAutoTick();

		window.show();

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