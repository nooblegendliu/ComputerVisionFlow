#include "video_capture.hpp"
#include <NodeUi/NodeUiProvider.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
namespace DSPatch::DSPatchables {
class VideoCaptureQt final : public VideoCapture,public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override { return role==VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role,QWidget* parent) override {
        if(!hasNodeUi(role)) return nullptr;
        auto* panel=new QWidget(parent); panel->setObjectName("VideoCaptureControls"); auto* layout=new QFormLayout(panel);
        auto* refreshButton=new QPushButton("Refresh Cap List",panel); refreshButton->setObjectName("VideoCaptureRefresh");
        auto* force=new QCheckBox("Force Use Index",panel); force->setObjectName("VideoCaptureForceIndex");
        auto* cameras=new QComboBox(panel); cameras->setObjectName("VideoCaptureCamera");
        auto* modes=new QComboBox(panel); modes->setObjectName("VideoCaptureMode");
        for(const auto& m:CameraModes()) modes->addItem(QString("%1 x %2").arg(m.width).arg(m.height));
        auto* fps=new QComboBox(panel); fps->setObjectName("VideoCaptureFps");
        for(int v:{1,3,5,10,15,20,25,30,60,120}) fps->addItem(QString::number(v),v);
        auto* external=new QPushButton("Open External Settings",panel); external->setObjectName("VideoCaptureExternal");
        auto* restore=new QCheckBox("Restore Settings on Load",panel); restore->setObjectName("VideoCaptureRestore");
        auto* manual=new QGroupBox("Manual Settings",panel); auto* properties=new QFormLayout(manual);
        auto* readProperties=new QPushButton("Refresh Settings",panel); readProperties->setObjectName("VideoCaptureRefreshSettings");
        auto* status=new QLabel(panel); status->setWordWrap(true); status->setTextFormat(Qt::PlainText);
        layout->addRow(refreshButton); layout->addRow(force); layout->addRow("Camera",cameras); layout->addRow("Video Resolution",modes);
        layout->addRow("Video FPS",fps); layout->addRow(external); layout->addRow(restore); layout->addRow(manual); layout->addRow(readProperties); layout->addRow(status);
        const std::weak_ptr<internal::VideoCaptureSession> weak=session_;
        QObject::connect(refreshButton,&QPushButton::clicked,panel,[weak] {if(auto s=weak.lock()) s->Refresh();});
        QObject::connect(force,&QCheckBox::toggled,panel,[weak](bool value) {if(auto s=weak.lock()) s->Restore(nlohmann::json{{"force_index",value}}.dump());});
        QObject::connect(cameras,&QComboBox::currentIndexChanged,panel,[weak,cameras](int index) {
            if(index>=0) if(auto s=weak.lock()) s->Restore(nlohmann::json{{"list_index",index},{"src_index",cameras->itemData(index).toInt()}}.dump());
        });
        QObject::connect(modes,&QComboBox::currentIndexChanged,panel,[weak](int index) {
            if(index>=0) if(auto s=weak.lock()) s->Restore(nlohmann::json{{"src_mode",index}}.dump());
        });
        QObject::connect(fps,&QComboBox::currentIndexChanged,panel,[weak,fps](int index) {
            if(index>=0) if(auto s=weak.lock()) s->Restore(nlohmann::json{{"fps_index",index},{"fps",fps->itemData(index).toInt()}}.dump());
        });
        QObject::connect(restore,&QCheckBox::toggled,panel,[weak](bool value) {
            if(auto s=weak.lock()) s->ConfigureRestore(value);
        });
        QObject::connect(external,&QPushButton::clicked,panel,[weak] {if(auto s=weak.lock()) s->Settings();});
        QObject::connect(readProperties,&QPushButton::clicked,panel,[weak] {if(auto s=weak.lock()) s->ReadProperties();});
        auto propertyEditors=std::make_shared<std::map<int,QDoubleSpinBox*>>();
        const auto refresh=[weak,panel,force,cameras,modes,fps,restore,manual,properties,propertyEditors,status] {
            if(auto s=weak.lock()) {
                const auto v=s->Snapshot(); QSignalBlocker a(force),b(cameras),c(modes),d(fps),e(restore);
                force->setChecked(v.forceIndex); modes->setCurrentIndex(v.mode); fps->setCurrentIndex(v.fpsIndex); restore->setChecked(v.restoreSettings);
                bool same=cameras->count()==static_cast<int>(v.cameras.size());
                if(same) for(int i=0;i<cameras->count();++i) if(cameras->itemText(i)!=QString::fromUtf8(v.cameras[i].second.c_str())||cameras->itemData(i).toInt()!=v.cameras[i].first) {same=false;break;}
                if(!same) {cameras->clear(); for(const auto& [index,name]:v.cameras) cameras->addItem(QString::fromUtf8(name.c_str()),index);}
                cameras->setCurrentIndex(v.listIndex);
                if(propertyEditors->size()!=v.properties.size()) {
                    while(properties->rowCount()) properties->removeRow(0); propertyEditors->clear();
                    for(const auto& [id,value]:v.properties) {
                        auto* edit=new QDoubleSpinBox(manual); edit->setRange(-1e12,1e12); edit->setDecimals(3); edit->setSingleStep(1);
                        edit->setObjectName(QString("VideoCaptureProperty%1").arg(id)); (*propertyEditors)[id]=edit;
                        properties->addRow(QString::fromStdString(value.name),edit);
                        QObject::connect(edit,&QDoubleSpinBox::valueChanged,panel,[weak,id](double value) {if(auto state=weak.lock()) state->SetProperty(id,static_cast<float>(value));});
                    }
                }
                for(const auto& [id,value]:v.properties) {
                    if(const auto it=propertyEditors->find(id);it!=propertyEditors->end()&&!it->second->hasFocus()) {
                        QSignalBlocker blocker(it->second); it->second->setValue(value.value);
                    }
                }
                status->setText(v.error.empty()?(v.opened?"Camera opened":"No camera opened"):QString::fromUtf8(v.error.c_str()));
            }
        };
        auto* timer=new QTimer(panel); QObject::connect(timer,&QTimer::timeout,panel,refresh); timer->start(200); refresh(); return panel;
    }
};
EXPORT_PLUGIN(VideoCaptureQt)
}
