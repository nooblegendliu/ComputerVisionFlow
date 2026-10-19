#include "HistogramPanel.h"
#include <HistogramViewer/histogram_viewer.hpp>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>

namespace {
using Node = DSPatch::DSPatchables::HistogramViewer;
class Plot final : public QWidget {
public:
    Node::Snapshot data;
    bool channels[3]{true,true,true};
    QRectF range{0,0,255,1024};
    QPointF previous;
    explicit Plot(QWidget* parent) : QWidget(parent) { setMinimumSize(240,160); setObjectName("histogramPlot"); }
    QRectF area() const { return QRectF(52,15,std::max(1,width()-68),std::max(1,height()-52)); }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.fillRect(rect(), palette().base());
        const auto a=area(); p.setPen(palette().text().color()); p.drawRect(a);
        p.drawText(QRectF(0,height()-25,width(),20),Qt::AlignCenter,"Range");
        p.drawText(2,13,"Value");
        for(int i=0;i<=4;++i) {
            const double t=i/4.; p.drawText(QRectF(a.left()-18+a.width()*t,a.bottom()+3,40,18),Qt::AlignCenter,QString::number(range.left()+range.width()*t,'g',4));
            p.drawText(QRectF(0,a.bottom()-a.height()*t-9,48,18),Qt::AlignRight,QString::number(range.top()+range.height()*t,'g',4));
        }
        if(!data.has_input) { p.drawText(a,Qt::AlignCenter,data.error?"Unsupported histogram input":"No histogram"); return; }
        p.setClipRect(a);
        const auto map=[&](double x,double y){return QPointF(a.left()+(x-range.left())*a.width()/range.width(),a.bottom()-(y-range.top())*a.height()/range.height());};
        const std::vector<float>* values[]{&data.red,&data.green,&data.blue};
        const QColor colors[]{Qt::red,Qt::green,data.is_color?QColor(Qt::blue):palette().text().color()};
        for(int c=0;c<3;++c) {
            if(!channels[c] || values[c]->empty()) continue;
            QPainterPath line; line.moveTo(map(data.x[0],(*values[c])[0]));
            for(size_t i=1;i<data.x.size();++i) line.lineTo(map(data.x[i],(*values[c])[i]));
            auto fill=line; fill.lineTo(map(data.x.back(),0)); fill.lineTo(map(data.x.front(),0)); fill.closeSubpath();
            auto shade=colors[c]; shade.setAlphaF(data.is_color?.5:.75); p.fillPath(fill,shade); p.setPen(colors[c]); p.drawPath(line);
        }
    }
    void mousePressEvent(QMouseEvent* e) override { previous=e->position(); }
    void mouseMoveEvent(QMouseEvent* e) override {
        if(e->buttons()&Qt::LeftButton) { const auto d=e->position()-previous; range.translate(-d.x()*range.width()/area().width(),d.y()*range.height()/area().height()); update(); }
        previous=e->position();
    }
    void wheelEvent(QWheelEvent* e) override {
        const double factor=e->angleDelta().y()>0?.8:1.25; const auto a=area();
        const double x=range.left()+(e->position().x()-a.left())/a.width()*range.width();
        const double y=range.top()+(a.bottom()-e->position().y())/a.height()*range.height();
        const double w=std::clamp(range.width()*factor,1e-6,1e12),h=std::clamp(range.height()*factor,1e-6,1e12);
        range=QRectF(x-(x-range.left())*w/range.width(),y-(y-range.top())*h/range.height(),w,h); update();
    }
    void mouseDoubleClickEvent(QMouseEvent*) override {
        double maximum=0;for(const auto* values:{&data.red,&data.green,&data.blue})for(float value:*values)maximum=std::max(maximum,double(value));
        range={0,0,255,std::max(1.,maximum*1.1)}; update();
    }
};
}
QWidget* CreateHistogramPanel(std::shared_ptr<Node> node,QWidget* parent)
{
    if(!node) return nullptr;
    auto* panel=new QWidget(parent); panel->setObjectName("histogramPanel");
    auto* layout=new QVBoxLayout(panel); auto* legend=new QHBoxLayout; layout->addLayout(legend);
    auto* plot=new Plot(panel); layout->addWidget(plot);
    for(int i=0;i<3;++i) { auto* box=new QCheckBox(i==0?"Red":i==1?"Green":"Blue / Gray",panel);box->setChecked(true);legend->addWidget(box);
        QObject::connect(box,&QCheckBox::toggled,plot,[plot,i](bool show){plot->channels[i]=show;plot->update();}); }
    auto* reset=new QPushButton("Reset axes",panel);legend->addWidget(reset);
    QObject::connect(reset,&QPushButton::clicked,plot,[plot]{plot->range={0,0,255,1024};plot->update();});
    auto* timer=new QTimer(panel);const std::weak_ptr<Node> weak=node;
    QObject::connect(timer,&QTimer::timeout,panel,[weak,plot,timer,panel]{
        if(auto n=weak.lock()){plot->data=n->GetSnapshot();plot->update();}else{timer->stop();panel->setEnabled(false);}
    }); plot->data=node->GetSnapshot(); timer->start(33);return panel;
}
