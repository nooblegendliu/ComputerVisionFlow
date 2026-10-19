#include "video_writer.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
namespace DSPatch::DSPatchables {
class VideoWriterQt final : public VideoWriter,public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override { return role==VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role,QWidget* parent) override {
        if(!hasNodeUi(role)) return nullptr;
        auto* panel=new QWidget(parent); panel->setObjectName("VideoWriterControls"); auto* layout=new QFormLayout(panel);
        auto* start=new QPushButton("Start Write",panel); start->setObjectName("VideoWriterStart");
        auto* stop=new QPushButton("Stop Write",panel); stop->setObjectName("VideoWriterStop");
        auto* codec=new QComboBox(panel); codec->setObjectName("VideoWriterCodec");
        for(const auto& value:VideoCodecs()) codec->addItem(QString::fromStdString(value.first));
        auto* fps=new QComboBox(panel); fps->setObjectName("VideoWriterFps");
        for(int value:{1,3,5,10,15,20,25,30,60,120}) fps->addItem(QString::number(value),value);
        auto* ext=new QCheckBox("Auto File Extension",panel); ext->setObjectName("VideoWriterAutoExtension");
        auto* choose=new QPushButton("Save Movie File",panel); choose->setObjectName("VideoWriterSave");
        auto* path=new QLabel(panel); path->setWordWrap(true); path->setTextFormat(Qt::PlainText);
        auto* status=new QLabel(panel); status->setWordWrap(true); status->setTextFormat(Qt::PlainText);
        layout->addRow(start,stop); layout->addRow("Video File Codec",codec); layout->addRow("Video File FPS",fps);
        layout->addRow(ext); layout->addRow(choose); layout->addRow("Movie File",path); layout->addRow(status);
        const std::weak_ptr<internal::VideoWriterSession> weak=session_;
        QObject::connect(start,&QPushButton::clicked,panel,[weak] {if(auto s=weak.lock()) s->Start();});
        QObject::connect(stop,&QPushButton::clicked,panel,[weak] {if(auto s=weak.lock()) s->Stop();});
        QObject::connect(codec,&QComboBox::currentIndexChanged,panel,[weak](int index) {
            if(index<0) return;
            if(auto s=weak.lock()) s->Restore(nlohmann::json{{"codec",index},{"codec_str",VideoCodecs().at(index).first}}.dump());
        });
        QObject::connect(fps,&QComboBox::currentIndexChanged,panel,[weak,fps](int index) {
            if(index>=0) if(auto s=weak.lock()) s->Restore(nlohmann::json{{"fps_index",index},{"fps",fps->itemData(index).toInt()}}.dump());
        });
        QObject::connect(ext,&QCheckBox::toggled,panel,[weak](bool value) {if(auto s=weak.lock()) s->Restore(nlohmann::json{{"auto_ext",value}}.dump());});
        QObject::connect(choose,&QPushButton::clicked,panel,[weak,panel] {
            if(auto s=weak.lock()) {
                auto* dialog=new QFileDialog(panel,"Save Movie",QString::fromUtf8(s->Snapshot().filename.c_str()));
                dialog->setAttribute(Qt::WA_DeleteOnClose); dialog->setAcceptMode(QFileDialog::AcceptSave); dialog->setFileMode(QFileDialog::AnyFile);
                dialog->setNameFilter("Movies (*.avi *.mpg *.mp4 *.mjpg *.mkv *.wmv);;All files (*)");
                dialog->setDefaultSuffix(QString::fromStdString(VideoCodecs().at(s->Snapshot().codec).second.substr(1)));
                QObject::connect(dialog,&QFileDialog::fileSelected,panel,[weak](const QString& name) {
                    if(auto state=weak.lock()) state->Restore(nlohmann::json{{"filename",name.toUtf8().toStdString()}}.dump());
                }); dialog->open();
            }
        });
        const auto refresh=[weak,codec,fps,ext,path,status] {
            if(auto s=weak.lock()) {
                const auto v=s->Snapshot(); QSignalBlocker a(codec),b(fps),c(ext);
                codec->setCurrentIndex(v.codec); fps->setCurrentIndex(v.fpsIndex); fps->setEnabled(!v.hasFpsInput); ext->setChecked(v.autoExtension);
                path->setText(v.filename.empty()?"[None]":QString::fromUtf8(v.filename.c_str()));
                status->setText(v.error.empty()?QString("%1 | %2 frames").arg(v.writing?"Writing":"Not Writing").arg(v.framesWritten):QString::fromUtf8(v.error.c_str()));
            }
        };
        auto* timer=new QTimer(panel); QObject::connect(timer,&QTimer::timeout,panel,refresh); timer->start(100); refresh(); return panel;
    }
};
EXPORT_PLUGIN(VideoWriterQt)
}
