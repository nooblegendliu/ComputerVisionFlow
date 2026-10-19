#include "DepthViewerPanel.h"
#include <DepthViewer3d/depth_viewer_3d.hpp>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QMatrix4x4>
#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>

namespace {
using Node=DSPatch::DSPatchables::DepthViewer3D;
using Json=nlohmann::json;
constexpr double Pi=3.14159265358979323846;
QVector3D Vector(const Json& j){return {j.at("x").get<float>(),j.at("y").get<float>(),j.at("z").get<float>()};}
Json Vector(const QVector3D& v){return {{"x",v.x()},{"y",v.y()},{"z",v.z()}};}
QColor Color(const Json& j){return QColor::fromRgbF(std::clamp(j.at("R").get<float>(),0.f,1.f),std::clamp(j.at("G").get<float>(),0.f,1.f),std::clamp(j.at("B").get<float>(),0.f,1.f));}
void Orbit(Json& s){constexpr float pi=3.14159265358979323846f;const float lat=s["view_lat"],lon=s["view_lon"],zoom=s["zoom"],rad=std::cos((lat-.5f)*pi);
    s["view_pos"]=Vector(QVector3D(std::cos(lon*2*pi)*rad*zoom,std::sin((lat-.5f)*pi)*zoom,std::sin(lon*2*pi)*rad*zoom));}
void Fly(Json& s){constexpr float radians=3.14159265358979323846f/180.f;const float yaw=s["yaw"].get<float>()*radians,pitch=s["pitch"].get<float>()*radians;
    const QVector3D fwd=QVector3D(std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch)).normalized();
    const auto right=QVector3D::crossProduct(fwd,{0,-1,0}).normalized();s["cam_fwd"]=Vector(fwd);s["cam_up"]=Vector(QVector3D::crossProduct(fwd,right).normalized());}
void ChangeMode(Json& s,bool orbit){
    if(orbit==s["orbit_mode"].get<bool>())return;
    const auto position=Vector(s["view_pos"]);
    if(orbit){
        const auto pos=(position/s["zoom"].get<float>()).normalized();
        const float lat=std::asin(std::clamp(pos.y(),-1.f,1.f))/float(Pi)+.5f,rad=std::cos((lat-.5f)*float(Pi));
        float lon=std::acos(std::clamp(rad!=0?pos.x()/rad:0.f,-1.f,1.f))/float(Pi)/2.f;
        if(position.z()<0)lon=-lon;
        s["view_center"]=Vector(QVector3D());s["view_lat"]=lat;s["view_lon"]=lon;Orbit(s);
    }else{
        const auto flat=QVector3D(position.x(),0,position.z()).normalized();
        float yaw=std::acos(std::clamp(QVector3D::dotProduct(flat,{0,0,-1}),-1.f,1.f))*float(180/Pi)+(position.x()<0?-90.f:90.f);
        if(position.x()<0)yaw=-yaw;
        s["yaw"]=yaw;s["pitch"]=std::clamp(std::acos(std::clamp(position.normalized().y(),-1.f,1.f))*float(180/Pi)-90.f,-89.99f,89.99f);Fly(s);
    }
    s["orbit_mode"]=orbit;
}
class Cloud final:public QOpenGLWidget,protected QOpenGLFunctions_3_3_Core {
    std::weak_ptr<Node> weak_;std::unique_ptr<QOpenGLShaderProgram> program_;GLuint vao_=0,vbo_=0,gridVao_=0,gridVbo_=0;
    uint64_t revision_=std::numeric_limits<uint64_t>::max();GLsizei count_=0,gridCount_=0;QPointF previous_;QString error_;bool functionsReady_=false;QMetaObject::Connection contextConnection_;
public:
    explicit Cloud(std::shared_ptr<Node> n,QWidget* parent):QOpenGLWidget(parent),weak_(n){
        setObjectName("depthCloud");setMinimumSize(320,200);setFocusPolicy(Qt::StrongFocus);
        QSurfaceFormat format;format.setVersion(3,3);format.setProfile(QSurfaceFormat::CoreProfile);format.setDepthBufferSize(24);format.setSamples(8);setFormat(format);
        auto* timer=new QTimer(this);QObject::connect(timer,&QTimer::timeout,this,[this,timer]{if(weak_.expired()){timer->stop();setEnabled(false);}else update();});timer->start(33);
        setToolTip("Right drag: orbit/look; middle drag or Ctrl+right drag: pan; wheel: zoom/move; WASD/QE: fly; H: home");
    }
    ~Cloud() override {cleanup();}
    void cleanup(){QObject::disconnect(contextConnection_);if(!context()||!functionsReady_)return;makeCurrent();if(vbo_)glDeleteBuffers(1,&vbo_);if(vao_)glDeleteVertexArrays(1,&vao_);if(gridVbo_)glDeleteBuffers(1,&gridVbo_);if(gridVao_)glDeleteVertexArrays(1,&gridVao_);vbo_=vao_=gridVbo_=gridVao_=0;program_.reset();doneCurrent();functionsReady_=false;revision_=std::numeric_limits<uint64_t>::max();}
    void setup(GLuint& vao,GLuint& vbo){glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Node::Vertex),nullptr);
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Node::Vertex),reinterpret_cast<void*>(3*sizeof(float)));
        glEnableVertexAttribArray(2);glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Node::Vertex),reinterpret_cast<void*>(7*sizeof(float)));glBindVertexArray(0);}
    void initializeGL() override {
        if(!initializeOpenGLFunctions()){error_="OpenGL 3.3 is unavailable";return;}
        functionsReady_=true;contextConnection_=QObject::connect(context(),&QOpenGLContext::aboutToBeDestroyed,this,[this]{cleanup();},Qt::DirectConnection);
        program_=std::make_unique<QOpenGLShaderProgram>();
        const char* vertex=R"(#version 330 core
layout(location=0) in vec3 aPos;layout(location=1) in vec4 aColor;layout(location=2) in vec3 aNormal;
uniform mat4 view;uniform mat4 proj;uniform float size;uniform float pointSize;
out vec4 color;out vec3 Normal;out vec3 viewDir;
void main(){gl_Position=proj*view*vec4(size*aPos,1);gl_PointSize=pointSize;color=aColor;Normal=aNormal;viewDir=(vec4(0,0,-1,1)*view).xyz;})";
        const char* fragment=R"(#version 330 core
in vec4 color;in vec3 Normal;in vec3 viewDir;uniform bool unshaded;out vec4 FragColor;
void main(){if(color.a<=0.0)discard;float diffuse=max(dot(normalize(Normal),normalize(viewDir)),0);FragColor=color*(unshaded?1.0:(diffuse+0.10));})";
        if(!program_->addShaderFromSourceCode(QOpenGLShader::Vertex,vertex)||!program_->addShaderFromSourceCode(QOpenGLShader::Fragment,fragment)||!program_->link()){error_=program_->log();program_.reset();return;}
        setup(vao_,vbo_);setup(gridVao_,gridVbo_);std::vector<Node::Vertex> grid;
        for(int i=-5;i<=5;++i){const float c=i==0?.55f:.25f;for(auto v:{QVector3D(float(i),0,-5),QVector3D(float(i),0,5),QVector3D(-5,0,float(i)),QVector3D(5,0,float(i))})grid.push_back({v.x(),v.y(),v.z(),c,c,c,1,0,1,0});}
        glBindBuffer(GL_ARRAY_BUFFER,gridVbo_);glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(grid.size()*sizeof(Node::Vertex)),grid.data(),GL_STATIC_DRAW);gridCount_=GLsizei(grid.size());
        glEnable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glEnable(GL_PROGRAM_POINT_SIZE);
    }
    void paintGL() override {
        if(!functionsReady_){QPainter p(this);p.setPen(Qt::red);p.drawText(rect(),Qt::AlignCenter,error_);return;}
        const auto n=weak_.lock();if(!n)return;const auto s=n->GetSettings();const auto bg=Color(s.at("bg_color"));glClearColor(bg.redF(),bg.greenF(),bg.blueF(),1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        const auto cloud=n->GetSnapshot();
        if(program_){
            if(s.at("msaa").get<bool>())glEnable(GL_MULTISAMPLE);else glDisable(GL_MULTISAMPLE);
            if(revision_!=cloud.revision){revision_=cloud.revision;count_=GLsizei(std::min<size_t>(cloud.points.size(),std::numeric_limits<GLsizei>::max()));glBindBuffer(GL_ARRAY_BUFFER,vbo_);glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(count_)*sizeof(Node::Vertex),cloud.points.data(),GL_DYNAMIC_DRAW);}
            QMatrix4x4 view,proj;const auto pos=Vector(s["view_pos"]);view.lookAt(pos,s["orbit_mode"].get<bool>()?Vector(s["view_center"]):pos+Vector(s["cam_fwd"]),s["orbit_mode"].get<bool>()?QVector3D(0,1,0):Vector(s["cam_up"]));proj.perspective(s["fov"].get<float>(),float(width())/std::max(1,height()),.1f,5000.f);
            program_->bind();program_->setUniformValue("view",view);program_->setUniformValue("proj",proj);program_->setUniformValue("pointSize",s["point_size"].get<float>());
            if(s["show_grid"].get<bool>()){program_->setUniformValue("unshaded",true);program_->setUniformValue("size",s["grid_size"].get<float>());glBindVertexArray(gridVao_);glDrawArrays(GL_LINES,0,gridCount_);}
            program_->setUniformValue("size",1.f);program_->setUniformValue("unshaded",!s["diff_shading"].get<bool>());glBindVertexArray(vao_);glDrawArrays(GL_POINTS,0,count_);glBindVertexArray(0);program_->release();
        }
        const auto error=error_.isEmpty()?QString::fromStdString(cloud.error):error_;if(!error.isEmpty()){QPainter p(this);p.setPen(Qt::red);p.drawText(rect().adjusted(8,8,-8,-8),Qt::AlignTop|Qt::TextWordWrap,error);}
    }
    void mousePressEvent(QMouseEvent* e) override{previous_=e->position();setFocus();}
    void mouseMoveEvent(QMouseEvent* e) override{
        auto n=weak_.lock();if(!n)return;auto s=n->GetSettings();const auto d=e->position()-previous_;previous_=e->position();const bool orbit=s["orbit_mode"];
        if(e->buttons()&Qt::RightButton){if(orbit){s["view_lat"]=std::clamp(s["view_lat"].get<float>()+float(d.y())*.002f,.02f,.98f);s["view_lon"]=s["view_lon"].get<float>()+float(d.x())*.001f;Orbit(s);}
            else if(!(e->modifiers()&Qt::ControlModifier)){s["pitch"]=std::clamp(s["pitch"].get<float>()-float(d.y())*.1f,-89.99f,89.99f);s["yaw"]=s["yaw"].get<float>()+float(d.x())*.1f;Fly(s);}}
        if((e->buttons()&Qt::MiddleButton)||(!orbit&&(e->buttons()&Qt::RightButton)&&(e->modifiers()&Qt::ControlModifier))){if(orbit)s["view_center"]["y"]=s["view_center"]["y"].get<float>()+float(d.y())*.01f;else{const auto up=Vector(s["cam_up"]),fwd=Vector(s["cam_fwd"]);const float speed=s["fly_speed"];s["view_pos"]=Vector(Vector(s["view_pos"])+up*float(d.y())*.1f*speed-QVector3D::crossProduct(fwd,up).normalized()*float(d.x())*.1f*speed);}}
        n->UpdateSettings(s);update();
    }
    void wheelEvent(QWheelEvent* e) override{if(auto n=weak_.lock()){auto s=n->GetSettings();const float sign=e->angleDelta().y()>0?1.f:-1.f;if(s["orbit_mode"].get<bool>()){s["zoom"]=std::max(.25f,s["zoom"].get<float>()-sign*.25f);Orbit(s);}else s["view_pos"]=Vector(Vector(s["view_pos"])+Vector(s["cam_fwd"])*s["fly_speed"].get<float>()*sign);n->UpdateSettings(s);update();}}
    void keyPressEvent(QKeyEvent* e) override{if(auto n=weak_.lock()){auto s=n->GetSettings();if(e->key()==Qt::Key_H){auto defaults=Node::DefaultCamera();for(const auto* key:{"view_pos","view_center","cam_fwd","cam_up","yaw","pitch","view_lat","view_lon","zoom"})s[key]=defaults[key];}
        else if(!s["orbit_mode"].get<bool>()){auto pos=Vector(s["view_pos"]);const auto fwd=Vector(s["cam_fwd"]),right=QVector3D::crossProduct(fwd,Vector(s["cam_up"])).normalized();const float speed=s["fly_speed"];switch(e->key()){case Qt::Key_W:pos+=fwd*speed;break;case Qt::Key_S:pos-=fwd*speed;break;case Qt::Key_A:pos-=right*speed;break;case Qt::Key_D:pos+=right*speed;break;case Qt::Key_Q:pos.setY(pos.y()+speed);break;case Qt::Key_E:pos.setY(pos.y()-speed);break;default:QOpenGLWidget::keyPressEvent(e);return;}s["view_pos"]=Vector(pos);}n->UpdateSettings(s);update();}}
};
QWidget* Form(std::shared_ptr<Node> node,QWidget* parent,bool camera){
    auto* panel=new QWidget(parent);panel->setObjectName(camera?"depthCameraControls":"depthViewerControls");auto* form=new QFormLayout(panel);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    const std::weak_ptr<Node> weak=node;std::vector<std::function<void(const Json&)>> refresh;
    auto number=[&](const char* key,const char* label,double low,double high,double step){auto* spin=new QDoubleSpinBox(panel);spin->setObjectName(key);spin->setDecimals(4);spin->setRange(low,high);spin->setSingleStep(step);form->addRow(label,spin);
        refresh.push_back([spin,key,low,high](const Json& s){const QSignalBlocker b(spin);const double v=s.at(key);spin->setRange(std::min(low,v),std::max(high,v));if(!spin->hasFocus())spin->setValue(v);});
        QObject::connect(spin,&QDoubleSpinBox::valueChanged,panel,[weak,key](double v){if(auto n=weak.lock())n->UpdateSettings({{key,float(v)}});});};
    auto boolean=[&](const char* key,const char* label){auto* box=new QCheckBox(label,panel);box->setObjectName(key);form->addRow(box);refresh.push_back([box,key](const Json& s){const QSignalBlocker b(box);box->setChecked(s.at(key));});QObject::connect(box,&QCheckBox::toggled,panel,[weak,key](bool v){if(auto n=weak.lock()){if(std::string(key)=="orbit_mode"){auto s=n->GetSettings();ChangeMode(s,v);n->UpdateSettings(s);}else n->UpdateSettings({{key,v}});}});};
    auto color=[&](const char* key,const char* label){auto* button=new QPushButton(panel);button->setObjectName(key);form->addRow(label,button);refresh.push_back([button,key](const Json& s){const auto c=Color(s.at(key));button->setText(c.name());button->setStyleSheet(QString("background-color:%1;color:%2").arg(c.name(),c.lightness()>127?"black":"white"));});QObject::connect(button,&QPushButton::clicked,panel,[weak,panel,key]{if(auto n=weak.lock()){auto* dialog=new QColorDialog(Color(n->GetSettings().at(key)),panel);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setOption(QColorDialog::DontUseNativeDialog);QObject::connect(dialog,&QColorDialog::colorSelected,panel,[weak,key](const QColor& c){if(auto current=weak.lock())current->UpdateSettings({{key,{{"R",c.redF()},{"G",c.greenF()},{"B",c.blueF()}}}});});dialog->open();}});};
    if(camera){number("fov","FOV",1,100,.5);boolean("orbit_mode","Orbit");number("fly_speed","Fly Speed",.01,10,.01);boolean("show_grid","Show Grid");number("grid_size","Grid Size",.1,100,.05);boolean("msaa","MSAA");color("bg_color","BG Color");}
    else{number("no_int_hfov","H-FOV",1,200,1);number("point_size","Point Size",.1,100,.1);boolean("diff_shading","Diffuse Shading");color("diff_color","Diffuse Color");
        for(const auto* axis:{"x","y","z"}){auto* spin=new QDoubleSpinBox(panel);spin->setObjectName(QString("pos_offset.%1").arg(axis));spin->setRange(-500,500);spin->setSingleStep(.1);spin->setDecimals(4);form->addRow(QString("Offset %1").arg(axis),spin);refresh.push_back([spin,axis](const Json& s){const QSignalBlocker b(spin);const double v=s.at("pos_offset").at(axis);spin->setRange(std::min(-500.,v),std::max(500.,v));if(!spin->hasFocus())spin->setValue(v);});QObject::connect(spin,&QDoubleSpinBox::valueChanged,panel,[weak,axis](double v){if(auto n=weak.lock()){auto pos=n->GetSettings().at("pos_offset");pos[axis]=float(v);n->UpdateSettings({{"pos_offset",pos}});}});}}
    auto* timer=new QTimer(panel);auto update=[weak,panel,timer,form,refresh,camera]{if(auto n=weak.lock()){const auto s=n->GetSettings();for(const auto& f:refresh)f(s);if(camera){form->setRowVisible(panel->findChild<QDoubleSpinBox*>("fly_speed"),!s["orbit_mode"].get<bool>());form->setRowVisible(panel->findChild<QDoubleSpinBox*>("grid_size"),s["show_grid"].get<bool>());}else form->setRowVisible(panel->findChild<QDoubleSpinBox*>("no_int_hfov"),!n->GetSnapshot().has_intrinsic);}else{timer->stop();panel->setEnabled(false);}};
    QObject::connect(timer,&QTimer::timeout,panel,update);update();timer->start(100);return panel;
}
}
QWidget* CreateDepthViewerControls(std::shared_ptr<Node> node,QWidget* parent){return node?Form(std::move(node),parent,false):nullptr;}
QWidget* CreateDepthViewerPanel(std::shared_ptr<Node> node,QWidget* parent){if(!node)return nullptr;node->InitializeView();auto* panel=new QWidget(parent);panel->setObjectName("depthViewerPanel");auto* layout=new QHBoxLayout(panel);layout->addWidget(new Cloud(node,panel),1);layout->addWidget(Form(node,panel,true));return panel;}
