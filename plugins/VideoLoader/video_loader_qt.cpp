#include "video_loader.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
namespace DSPatch::DSPatchables {
class VideoLoaderQt final : public VideoLoader,public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override { return role==VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role,QWidget* parent) override {
        if(!hasNodeUi(role)) return nullptr;
        auto* panel=new QWidget(parent); panel->setObjectName("VideoLoaderControls"); auto* layout=new QVBoxLayout(panel);
        auto* choose=new QPushButton("Load Movie File",panel); choose->setObjectName("VideoLoaderLoad");
        auto* path=new QLabel(panel); path->setWordWrap(true); path->setTextFormat(Qt::PlainText);
        auto* loop=new QCheckBox("Loop Playback",panel); loop->setObjectName("VideoLoaderLoop");
        auto* fps=new QCheckBox("Use Movie FPS",panel); fps->setObjectName("VideoLoaderUseFps");
        auto* frame=new QSlider(Qt::Horizontal,panel); frame->setObjectName("VideoLoaderFrame");
        auto* info=new QLabel(panel); info->setTextFormat(Qt::PlainText); info->setWordWrap(true);
        layout->addWidget(choose); layout->addWidget(path); layout->addWidget(loop); layout->addWidget(fps); layout->addWidget(frame);
        auto* row=new QHBoxLayout; layout->addLayout(row); layout->addWidget(info);
        const std::weak_ptr<internal::VideoLoaderSession> weak=session_;
        const auto add=[&](const QString& label,const char* name,auto action) {
            auto* button=new QPushButton(label,panel); button->setObjectName(name); row->addWidget(button);
            QObject::connect(button,&QPushButton::clicked,panel,[weak,action] { if(auto s=weak.lock()) action(*s); });
        };
        add("|<","VideoLoaderFirst",[](auto& s) {s.Seek(1);});
        add("<<","VideoLoaderPrevious",[](auto& s) {s.Seek(s.Snapshot().frame-1);});
        add("Stop","VideoLoaderStop",[](auto& s) {s.Play(false);});
        add("Play","VideoLoaderPlay",[](auto& s) {s.Play(true);});
        add(">>","VideoLoaderNext",[](auto& s) {s.Seek(s.Snapshot().frame+1);});
        add(">|","VideoLoaderLast",[](auto& s) {s.Seek(s.Snapshot().frameCount);});
        QObject::connect(loop,&QCheckBox::toggled,panel,[weak](bool value) {if(auto s=weak.lock()) s->Restore(nlohmann::json{{"looping",value}}.dump());});
        QObject::connect(fps,&QCheckBox::toggled,panel,[weak](bool value) {if(auto s=weak.lock()) s->Restore(nlohmann::json{{"use_fps_speed",value}}.dump());});
        QObject::connect(frame,&QSlider::valueChanged,panel,[weak](int value) {if(auto s=weak.lock()) s->Seek(value);});
        QObject::connect(choose,&QPushButton::clicked,panel,[weak,panel] {
            if(auto s=weak.lock()) {
                auto* dialog=new QFileDialog(panel,"Load Movie",QString::fromUtf8(s->Snapshot().path.c_str()));
                dialog->setAttribute(Qt::WA_DeleteOnClose); dialog->setFileMode(QFileDialog::ExistingFile);
                dialog->setNameFilter("Movies (*.avi *.mpg *.mp4 *.mkv *.webm);;All files (*)");
                QObject::connect(dialog,&QFileDialog::fileSelected,panel,[weak](const QString& name) {
                    if(auto state=weak.lock()) state->Restore(nlohmann::json{{"movie_path",name.toUtf8().toStdString()}}.dump());
                }); dialog->open();
            }
        });
        const auto refresh=[weak,path,loop,fps,frame,info] {
            if(auto s=weak.lock()) {
                const auto value=s->Snapshot(); QSignalBlocker a(loop),b(fps),c(frame);
                path->setText(value.path.empty()?"[None]":QString::fromUtf8(value.path.c_str())); loop->setChecked(value.looping); fps->setChecked(value.useFps);
                frame->setRange(1,std::max(1,value.frameCount)); frame->setEnabled(value.opened);
                if(!frame->isSliderDown()) frame->setValue(std::max(1,value.frame));
                info->setText(value.error.empty()?QString("%1 | Frame %2 / %3 | %4 FPS").arg(value.playing?"Playing":"Stopped").arg(value.frame).arg(value.frameCount).arg(value.fps):QString::fromUtf8(value.error.c_str()));
            }
        };
        auto* timer=new QTimer(panel); QObject::connect(timer,&QTimer::timeout,panel,refresh); timer->start(100); refresh(); return panel;
    }
};
EXPORT_PLUGIN(VideoLoaderQt)
}
