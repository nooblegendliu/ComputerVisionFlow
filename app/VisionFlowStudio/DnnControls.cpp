#include "DnnControls.h"
#include <flowNode.h>
#include <DNN/dnn_controls.hpp>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>
#include <limits>
#include <map>

namespace {
// ImGui Drag controls bound dragging, but still accept explicit text input and
// restored values outside that interval. Keep the same distinction in Qt.
template<class Base, class Value>
class DnnStepSpinBox final : public Base {
public:
    DnnStepSpinBox(QWidget* parent, Value lower, Value upper)
        : Base(parent), lower_(lower), upper_(upper) {}
protected:
    void stepBy(int steps) override {
        if ((steps < 0 && this->value() <= lower_) || (steps > 0 && this->value() >= upper_)) return;
        const double next = static_cast<double>(this->value()) + static_cast<double>(steps) * this->singleStep();
        this->setValue(static_cast<Value>(std::clamp(next, static_cast<double>(lower_), static_cast<double>(upper_))));
    }
    QAbstractSpinBox::StepEnabled stepEnabled() const override {
        QAbstractSpinBox::StepEnabled result = QAbstractSpinBox::StepNone;
        if (!this->isReadOnly()) {
            if (this->value() < upper_) result |= QAbstractSpinBox::StepUpEnabled;
            if (this->value() > lower_) result |= QAbstractSpinBox::StepDownEnabled;
        }
        return result;
    }
private:
    Value lower_, upper_;
};
}

QWidget* CreateDnnControls(std::shared_ptr<FlowCV::Node> node, QWidget* parent)
{
    using Json = nlohmann::json;
    using Interface = DSPatch::DSPatchables::DnnControlInterface;
    if (!node || !dynamic_cast<Interface*>(node.get())) return nullptr;
    const std::weak_ptr<FlowCV::Node> weak = node;
    auto* panel = new QWidget(parent); panel->setObjectName("dnnControls");
    auto* layout = new QVBoxLayout(panel); layout->setAlignment(Qt::AlignTop);
    auto* status = new QLabel(panel); status->setObjectName("networkStatus"); status->setWordWrap(true);
    auto* initialize = new QPushButton("Initialize Network", panel); initialize->setObjectName("initializeNetwork");
    auto* clear = new QPushButton("Clear Paths", panel); clear->setObjectName("clearPaths");
    layout->addWidget(status); layout->addWidget(initialize); layout->addWidget(clear);
    auto* form = new QFormLayout; form->setRowWrapPolicy(QFormLayout::WrapLongRows); layout->addLayout(form);
    auto* timer = new QTimer(panel);
    using Refresh = std::function<void(const Json&)>;
    auto refreshers = std::make_shared<std::vector<Refresh>>();
    const auto apply = [weak, status](const Json& patch) {
        if (auto current = weak.lock()) try { dynamic_cast<Interface*>(current.get())->UpdateControlState(patch); }
        catch (const std::exception& error) { status->setText(QString::fromUtf8(error.what())); }
    };
    const auto initial = dynamic_cast<Interface*>(node.get())->GetControlState();
    const std::map<std::string, QString> labels{
        {"model_path","Model File"},{"config_path","Config File (optional for ONNX)"},{"classes_path","Classes File"},
        {"colors_path","Colors File (optional)"},{"voc_path","Vocabulary File"},{"mean","Mean"},{"std_dev","Std Dev"},
        {"scale","Scale"},{"model_res","Model Size"},{"conf_thresh","Confidence Threshold"},{"_nms_thresh","NMS Threshold"},
        {"preproc","Preprocess Resize"},{"swap_rb","RGB"},{"crop","Crop"},{"softmax","Softmax"},
        {"_draw","Draw Detections / Class"},{"draw_joints","Draw Joints"},{"force_single","Force Single Detection Mode"},
        {"dataset_mode","Dataset Mode"},{"_processing_mode","Processing Mode"},{"bin_thresh","Binary Threshold"},
        {"poly_thresh","Polygon Threshold"},{"unclip_ratio","Unclip Ratio"},{"max_candidates","Max Candidates"},
        {"txt_position","Text Position"},{"txt_scale","Text Scale"},{"txt_thickness","Text Thickness"},
        {"txt_color","Text Color"},{"bbox_color","BBox Color"},{"bbox_thickness","BBox Thickness"}
    };
    auto* backend = new QComboBox(panel); backend->setObjectName("backend"); form->addRow("Backend", backend);
    auto* target = new QComboBox(panel); target->setObjectName("target"); form->addRow("Target", target);
    QObject::connect(backend, &QComboBox::currentIndexChanged, panel, [apply, backend](int i) { if (i>=0) apply({{"backend",backend->itemData(i).toInt()},{"target",0}}); });
    QObject::connect(target, &QComboBox::currentIndexChanged, panel, [apply, target](int i) { if (i>=0) apply({{"target",target->itemData(i).toInt()}}); });
    std::vector<QWidget*> drawRows;
    for (const auto& [key,value] : initial.items()) {
        if (key == "backend" || key == "target") continue;
        const QString title = labels.count(key) ? labels.at(key) : QString::fromStdString(key);
        QWidget* row = nullptr;
        if (value.is_string()) {
            row = new QWidget(panel); auto* line = new QLineEdit(row); line->setObjectName(QString::fromStdString(key));
            auto* browse = new QPushButton("Browse…", row); browse->setObjectName(QString::fromStdString(key+"Browse"));
            auto* h = new QHBoxLayout(row); h->setContentsMargins(0,0,0,0); h->addWidget(line); h->addWidget(browse);
            QObject::connect(line, &QLineEdit::editingFinished, panel, [apply,key,line] { apply({{key,line->text().toStdString()}}); });
            QObject::connect(browse, &QPushButton::clicked, panel, [apply,key,line,panel] {
                const auto path = QFileDialog::getOpenFileName(panel, "Select DNN File", line->text());
                if (!path.isEmpty()) { line->setText(path); apply({{key,path.toStdString()}}); }
            });
            refreshers->emplace_back([line,key](const Json& s) { if (!line->hasFocus()) line->setText(QString::fromStdString(s.at(key).get<std::string>())); });
        } else if (value.is_boolean()) {
            auto* box = new QCheckBox(panel); row = box; box->setObjectName(QString::fromStdString(key));
            QObject::connect(box, &QCheckBox::toggled, panel, [apply,key](bool checked) { apply({{key,checked}}); });
            refreshers->emplace_back([box,key](const Json& s) { const QSignalBlocker b(box); box->setChecked(s.at(key).get<bool>()); });
        } else if (key == "dataset_mode" || key == "_processing_mode") {
            auto* combo = new QComboBox(panel); row = combo; combo->setObjectName(QString::fromStdString(key));
            if (key == "dataset_mode") combo->addItems({"COCO","MPII","Hand"}); else combo->addItem("Style Transfer");
            QObject::connect(combo,&QComboBox::currentIndexChanged,panel,[apply,key](int index){if(index>=0)apply({{key,index}});});
            refreshers->emplace_back([combo,key](const Json& s){const QSignalBlocker b(combo);combo->setCurrentIndex(s.at(key).get<int>());});
        } else if (key == "txt_color" || key == "bbox_color") {
            auto* color = new QPushButton(panel); row = color; color->setObjectName(QString::fromStdString(key));
            QObject::connect(color, &QPushButton::clicked, panel, [weak,apply,key,panel] {
                if (auto current=weak.lock()) {
                    const auto c=dynamic_cast<Interface*>(current.get())->GetControlState().at(key);
                    const auto chosen=QColorDialog::getColor(QColor::fromRgbF(std::clamp(c.at("R").get<double>(),0.,1.),std::clamp(c.at("G").get<double>(),0.,1.),std::clamp(c.at("B").get<double>(),0.,1.)),panel);
                    if(chosen.isValid()) apply({{key,{{"R",chosen.redF()},{"G",chosen.greenF()},{"B",chosen.blueF()}}}});
                }
            });
            refreshers->emplace_back([color,key](const Json& s){ const auto c=s.at(key); const auto rgb=QColor::fromRgbF(std::clamp(c.at("R").get<double>(),0.,1.),std::clamp(c.at("G").get<double>(),0.,1.),std::clamp(c.at("B").get<double>(),0.,1.)); color->setText(rgb.name());color->setStyleSheet(QString("background: %1; color: %2;").arg(rgb.name(),rgb.lightness()>127?"black":"white")); });
        } else {
            row = new QWidget(panel); auto* h = new QHBoxLayout(row); h->setContentsMargins(0,0,0,0);
            std::vector<std::string> components;
            if(value.is_array()) for(size_t i=0;i<value.size();++i) components.push_back(std::to_string(i));
            else if(value.is_object()) for(const auto& [part,v]:value.items()) components.push_back(part);
            else components.emplace_back();
            for(const auto& part:components) {
                const auto pointer=Json::json_pointer("/"+key+(part.empty()?"":"/"+part));
                const auto currentValue=initial.at(pointer);
                if(!part.empty())h->addWidget(new QLabel(QString::fromStdString(part),row));
                const auto change=[weak,apply,key,part](const Json& v){
                    if(part.empty()){apply({{key,v}});return;}
                    if(auto current=weak.lock()) {auto compound=dynamic_cast<Interface*>(current.get())->GetControlState().at(key);if(compound.is_array())compound.at(std::stoul(part))=v;else compound[part]=v;apply({{key,compound}});}
                };
                const auto objectName=QString::fromStdString(key+(part.empty()?"":"."+part));
                if(currentValue.is_number_integer()) {
                    int lower=std::numeric_limits<int>::min(), upper=std::numeric_limits<int>::max();
                    if(key=="model_res") {lower=224;upper=1000;}
                    else if(key=="max_candidates") {lower=1;upper=500;}
                    else if(key=="bbox_thickness"||(key=="txt_thickness"&&node->GetComponentName()!="Classification")) {lower=1;upper=10;}
                    auto* spin=new DnnStepSpinBox<QSpinBox,int>(row,lower,upper); spin->setObjectName(objectName);spin->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max()); h->addWidget(spin);
                    QObject::connect(spin,&QSpinBox::valueChanged,panel,[change](int v){change(v);});
                    refreshers->emplace_back([spin,pointer](const Json& s){const QSignalBlocker b(spin);if(!spin->hasFocus())spin->setValue(s.at(pointer).get<int>());});
                }else{
                    double lower=-1e12,upper=1e12,step=.01;
                    if(key=="mean"||key=="std_dev") {lower=0;upper=500;step=.1;}
                    else if(key=="scale") {lower=0;upper=10;step=.0001;}
                    else if(key=="conf_thresh"||key=="_nms_thresh"||key=="bin_thresh"||key=="poly_thresh") {lower=0;upper=1;}
                    else if(key=="unclip_ratio") {lower=0;upper=5;}
                    else if(key=="txt_scale") {
                        if(node->GetComponentName()=="Classification") step=.1;
                        else {lower=0;upper=100;}
                    }
                    auto* spin=new DnnStepSpinBox<QDoubleSpinBox,double>(row,lower,upper);spin->setObjectName(objectName);spin->setDecimals(8);spin->setRange(-1e12,1e12);spin->setSingleStep(step);h->addWidget(spin);
                    QObject::connect(spin,&QDoubleSpinBox::valueChanged,panel,[change](double v){change(v);});
                    refreshers->emplace_back([spin,pointer](const Json& s){const QSignalBlocker b(spin);if(!spin->hasFocus())spin->setValue(s.at(pointer).get<double>());});
                }
            }
        }
        form->addRow(title,row);
        if(key.rfind("txt_",0)==0||key.rfind("bbox_",0)==0)drawRows.push_back(row);
    }
    auto* forceSingle = panel->findChild<QCheckBox*>("force_single");
    auto* crop = initial.contains("_nms_thresh") ? panel->findChild<QCheckBox*>("crop") : nullptr;
    const auto refresh=[weak,panel,timer,form,drawRows,refreshers,status,initialize,backend,target,forceSingle,crop] {
        const auto current=weak.lock();if(!current){timer->stop();panel->setEnabled(false);return;}
        auto* control=dynamic_cast<Interface*>(current.get());const auto state=control->GetControlState();
        for(const auto& update:*refreshers)update(state);
        for(auto* row:drawRows)form->setRowVisible(row,state.value("_draw",false));
        const auto fill=[](QComboBox* combo,const auto& entries,int selected){
            const QSignalBlocker b(combo);bool changed=combo->count()!=static_cast<int>(entries.size());
            if(!changed)for(int i=0;i<combo->count();++i)if(combo->itemData(i).toInt()!=entries[i].second){changed=true;break;}
            if(changed){combo->clear();for(const auto& e:entries)combo->addItem(QString::fromStdString(e.first),e.second);}
            combo->setCurrentIndex(combo->findData(selected));
        };
        fill(backend,control->GetBackendOptions(),state.at("backend").get<int>());fill(target,control->GetTargetOptions(),state.at("target").get<int>());
        const auto health=control->GetNetworkStatus();
        if(forceSingle)form->setRowVisible(forceSingle,health.value("has_paf_output",false));
        if(crop)form->setRowVisible(crop,state.value("preproc",false));
        const bool initialized=health.at("initialized").get<bool>();
        initialize->setText(initialized?"Reinitialize Network":"Initialize Network");
        bool ready = !state.at("model_path").get<std::string>().empty();
        if (state.contains("classes_path") && !state.contains("_nms_thresh"))
            ready = ready && !state.at("classes_path").get<std::string>().empty();
        if (state.contains("voc_path")) ready = ready && !state.at("voc_path").get<std::string>().empty();
        initialize->setEnabled(ready);
        const auto error=health.at("error").get<std::string>();
        status->setText(!error.empty()?QString::fromStdString(error):health.at("needs_reinit").get<bool>()?"Network needs to be reinitialized":initialized?"Network initialized":"Network needs to be initialized");
    };
    QObject::connect(initialize,&QPushButton::clicked,panel,[weak,refresh]{if(auto current=weak.lock())dynamic_cast<Interface*>(current.get())->InitializeNetwork();refresh();});
    QObject::connect(clear,&QPushButton::clicked,panel,[apply,initial,refresh]{Json patch=Json::object();for(const auto& [key,v]:initial.items())if(v.is_string())patch[key]="";apply(patch);refresh();});
    QObject::connect(timer,&QTimer::timeout,panel,refresh);timer->start(150);refresh();return panel;
}
