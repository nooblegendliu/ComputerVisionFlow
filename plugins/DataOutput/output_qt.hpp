// Optional QWidget Controls; all I/O and serialization stay in the original node classes.
#include <NodeUi/NodeUiProvider.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <algorithm>
namespace DSPatch::DSPatchables {
class DataOutputQt final : public OUTPUT_CLASS, public VisionFlow::NodeUiProvider {
public:
    bool hasNodeUi(VisionFlow::NodeUiRole role) const override { return role==VisionFlow::NodeUiRole::Controls; }
    QWidget* createNodeUi(VisionFlow::NodeUiRole role,QWidget* parent) override {
        if(!hasNodeUi(role)) return nullptr;
        auto* panel=new QWidget(parent); panel->setObjectName(QString::fromStdString(GetComponentName())+"Controls");
        auto* form=new QFormLayout(panel);
        auto* status=new QLabel(panel); status->setObjectName("DataOutputStatus");status->setWordWrap(true);
        const std::weak_ptr<OutputUiBridge> weak=UiHandle();
        using Json=nlohmann::json;
        const auto apply=[weak,status](Json patch) {
            if(auto bridge=weak.lock()) { std::lock_guard lock(bridge->mutex); if(!bridge->node) return;
                try { bridge->updateControls(patch); status->setText(QString::fromStdString(bridge->status())); }
                catch(const std::exception& e) { status->setText(QString::fromUtf8(e.what())); }
            }
        };
        std::vector<std::function<void(const Json&)>> updates;
        const auto line=[&](const char* label,const char* key) {
            auto* edit=new QLineEdit(panel);edit->setObjectName(QString("DataOutput_")+key);form->addRow(label,edit);
            QObject::connect(edit,&QLineEdit::editingFinished,panel,[edit,key=std::string(key),apply] { apply(Json{{key,edit->text().toUtf8().toStdString()}}); });
            updates.emplace_back([edit,key=std::string(key)](const Json& state){ if(!edit->hasFocus()) {QSignalBlocker block(edit); edit->setText(QString::fromUtf8(state.at(key).get<std::string>()));} });
            return edit;
        };
        const auto integer=[&](const char* label,const char* key,int low,int high) {
            auto* spin=new QSpinBox(panel); spin->setObjectName(QString("DataOutput_")+key);spin->setRange(low,high);form->addRow(label,spin);
            QObject::connect(spin,&QSpinBox::valueChanged,panel,[key=std::string(key),apply](int value){apply(Json{{key,value}});});
            updates.emplace_back([spin,key=std::string(key),low,high](const Json& state){ if(!spin->hasFocus()){QSignalBlocker block(spin);const int value=state.at(key).get<int>();spin->setRange((std::min)(value,low),(std::max)(value,high));spin->setValue(value);} });
            return spin;
        };
        const auto check=[&](const char* label,const char* key) {
            auto* box=new QCheckBox(panel);box->setObjectName(QString("DataOutput_")+key);form->addRow(label,box);
            QObject::connect(box,&QCheckBox::toggled,panel,[key=std::string(key),apply](bool value){apply(Json{{key,value}});});
            updates.emplace_back([box,key=std::string(key)](const Json& state){QSignalBlocker block(box);box->setChecked(state.at(key).get<bool>());});return box;
        };
        const auto combo=[&](const char* label,const char* key,const QStringList& options) {
            auto* box=new QComboBox(panel);box->setObjectName(QString("DataOutput_")+key);box->addItems(options);form->addRow(label,box);
            QObject::connect(box,&QComboBox::currentIndexChanged,panel,[key=std::string(key),apply](int value){if(value>=0)apply(Json{{key,value}});});
            updates.emplace_back([box,key=std::string(key)](const Json& state){QSignalBlocker block(box);box->setCurrentIndex(state.at(key).get<int>());});return box;
        };
        const auto initial=Json::parse(GetState());
        if(initial.contains("csv_file_path")) {
            auto* path=line("CSV File","csv_file_path");
            auto* browse=new QPushButton("Save CSV File…",panel);browse->setObjectName("DataOutputBrowse");form->addRow(browse);
            QObject::connect(browse,&QPushButton::clicked,panel,[panel,path,apply] {
                auto* dialog=new QFileDialog(panel,"Save CSV",path->text(),"CSV files (*.csv)");
                dialog->setAcceptMode(QFileDialog::AcceptSave);dialog->setDefaultSuffix("csv");dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog,&QFileDialog::fileSelected,panel,[path,apply](const QString& name){path->setText(name);apply(Json{{"csv_file_path",name.toUtf8().toStdString()}});});dialog->open();
            });
            check("Save Timestamp","save_timestamp");check("Save Index Count","save_counter");check("Limit File Size","limit_file_size");
            auto* size=integer("Max Size KB","max_file_size_kb",100,INT_MAX);auto* rotate=check("Use Rotating Files","rotating_files");auto* count=integer("Num Files","num_files",2,100);
            updates.emplace_back([form,size,rotate,count](const Json& state){const bool limited=state.at("limit_file_size");form->setRowVisible(size,limited);form->setRowVisible(rotate,limited);form->setRowVisible(count,limited&&state.at("rotating_files").get<bool>());});
        } else {
            if(initial.contains("port_str")) {
                auto* refreshPorts=new QPushButton("Refresh Serial List",panel);refreshPorts->setObjectName("DataOutputRefreshPorts");form->addRow(refreshPorts);
                auto* ports=new QComboBox(panel);ports->setObjectName("DataOutput_port_idx");form->addRow("Port",ports);
                const auto refresh=[weak,ports]{if(auto bridge=weak.lock()){std::lock_guard lock(bridge->mutex);if(!bridge->node)return;QSignalBlocker block(ports);ports->clear();for(const auto& p:bridge->ports())ports->addItem(QString::fromStdString(p));ports->setCurrentIndex(Json::parse(bridge->node->GetState()).at("port_idx").get<int>());}};
                QObject::connect(refreshPorts,&QPushButton::clicked,panel,[weak,refresh]{if(auto bridge=weak.lock()){{std::lock_guard lock(bridge->mutex);if(bridge->node)bridge->refreshPorts();}refresh();}});
                QObject::connect(ports,&QComboBox::currentIndexChanged,panel,[ports,apply](int index){if(index>=0)apply(Json{{"port_idx",index},{"port_str",ports->itemText(index).toStdString()}});});
                updates.emplace_back([ports](const Json& state){QSignalBlocker block(ports);ports->setCurrentIndex(state.at("port_idx").get<int>());});refresh();
                combo("Baud Rate","baud_rate_idx",{"300","600","1200","1800","2400","4800","7200","9600","14400","19200","28800","33600","38400","57600","115200","128000","230400","256000","460800","921600"});
                combo("Data Bits","data_bits_idx",{"5","6","7","8"});combo("Parity","parity_idx",{"None","Odd","Even"});combo("Stop Bits","stop_bits_idx",{"1","1.5","2"});combo("Flow Control","flow_ctrl_idx",{"None","Software","Hardware"});
            } else {
                line("IP Address","ip_addr");integer("Port","port",0,65535);
                if(GetComponentName()!="TCP_Send") {auto* multicast=new QCheckBox(panel);multicast->setEnabled(false);multicast->setObjectName("DataOutputMulticast");form->addRow("Is Multicast",multicast);updates.emplace_back([weak,multicast](const Json&){if(auto bridge=weak.lock())if(bridge->multicast)multicast->setChecked(bridge->multicast());});}
            }
            if(initial.contains("data_mode"))combo("JSON Data Mode","data_mode",{"Text","BSON","CBOR","MessagePack","UBJSON"});
            combo("Rate (Hz)","data_rate",{"Fastest","60","30","20","15","10","5","2","1"});
            if(initial.contains("eol_seq")) {combo("EOL Sequence","eol_seq",{"None","<CR>","<LF>","<CR><LF>","<SPACE>","<TAB>","<COMMA>"});check("Send As Binary","send_binary");}
            if(initial.contains("osc_addr"))line("Address Prefix","osc_addr");
        }
        form->addRow("Status",status);
        const auto refresh=[weak,panel,updates,status] {
            if(auto bridge=weak.lock()) {std::lock_guard lock(bridge->mutex);if(bridge->node) {const auto state=Json::parse(bridge->node->GetState());for(const auto& update:updates)update(state);status->setText(QString::fromStdString(bridge->status()));return;}}
            panel->setEnabled(false);
        };
        auto* timer=new QTimer(panel);QObject::connect(timer,&QTimer::timeout,panel,refresh);timer->start(100);refresh();
        return panel;
    }
};
EXPORT_PLUGIN(DataOutputQt)
}
