// Qt adaptation of FlowCV ImageWriter Controls; computation stays Qt-free.
#include "image_writer.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QtCore/QTimer>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace DSPatch::DSPatchables {
class ImageWriterQt final : public ImageWriter, public VisionFlow::NodeUiProvider
{
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override
    { return role == VisionFlow::NodeUiRole::Controls; }

    QWidget* createNodeUi(VisionFlow::NodeUiRole role, QWidget* parent) override
    {
        if (!hasNodeUi(role)) return nullptr;
        auto* panel = new QWidget(parent);
        panel->setObjectName("ImageWriterControls");
        auto* layout = new QVBoxLayout(panel);
        auto* write = new QPushButton("Write Image", panel);
        write->setObjectName("ImageWriterWrite");
        auto* choose = new QPushButton("Set Output Path", panel);
        choose->setObjectName("ImageWriterSetPath");
        auto* path = new QLabel(panel);
        path->setObjectName("ImageWriterPath");
        path->setWordWrap(true);
        path->setTextFormat(Qt::PlainText);
        auto* status = new QLabel(panel);
        status->setObjectName("ImageWriterStatus");
        status->setWordWrap(true);
        status->setTextFormat(Qt::PlainText);
        layout->addWidget(write);
        layout->addWidget(choose);
        layout->addWidget(new QLabel("Save Image Path:", panel));
        layout->addWidget(path);
        layout->addWidget(status);
        const std::weak_ptr<internal::ImageWriterState> weak = state_;
        QObject::connect(write, &QPushButton::clicked, panel, [weak] {
            if (const auto state = weak.lock()) state->RequestWrite();
        });
        QObject::connect(choose, &QPushButton::clicked, panel, [weak, panel] {
            const auto state = weak.lock();
            if (!state) return;
            auto* dialog = new QFileDialog(panel, "Save Image",
                QString::fromUtf8(state->Snapshot().outputPath.c_str()));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setAcceptMode(QFileDialog::AcceptSave);
            dialog->setFileMode(QFileDialog::AnyFile);
            dialog->setNameFilter("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp);;All files (*)");
            QObject::connect(dialog, &QFileDialog::fileSelected, panel,
                [weak](const QString& selected) {
                    if (const auto value = weak.lock()) value->SetOutputPath(selected.toUtf8().toStdString());
                });
            dialog->open();
        });
        auto refresh = [weak, path, status] {
            if (const auto state = weak.lock()) {
                const auto value = state->Snapshot();
                path->setText(value.outputPath.empty() ? "[None]" : QString::fromUtf8(value.outputPath.c_str()));
                status->setText(!value.lastError.empty() ? QString::fromUtf8(value.lastError.c_str())
                    : value.pending ? "Waiting for an image" : value.completedWrites ? "Image written" : "");
            }
        };
        auto* timer = new QTimer(panel);
        QObject::connect(timer, &QTimer::timeout, panel, refresh);
        timer->start(100);
        refresh();
        return panel;
    }
};
EXPORT_PLUGIN(ImageWriterQt)
}
