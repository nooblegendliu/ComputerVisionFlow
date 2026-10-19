#include "depth_viewer_3d.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace DSPatch::DSPatchables {
using Json=nlohmann::json;
Json DepthViewer3D::DefaultCamera()
{
    constexpr float pi=3.14159265358979323846f;
    const float lat=.5437f,lon=-.2505f,zoom=13.5f,rad=std::cos((lat-.5f)*pi);
    return {{"view_lat",lat},{"view_lon",lon},{"yaw",88.5f},{"pitch",8.88f},{"zoom",zoom},
        {"fov",45.f},{"orbit_mode",true},{"fly_speed",.1f},{"show_grid",true},{"grid_size",1.f},{"msaa",false},
        {"view_pos",{{"x",std::cos(lon*2*pi)*rad*zoom},{"y",std::sin((lat-.5f)*pi)*zoom},{"z",std::sin(lon*2*pi)*rad*zoom}}},
        {"view_center",{{"x",0.f},{"y",3.7f},{"z",0.f}}},
        {"cam_fwd",{{"x",.00483705f},{"y",.118993f},{"z",.992883f}}},
        {"cam_up",{{"x",0.f},{"y",1.f},{"z",0.f}}},
        {"bg_color",{{"R",22/255.f},{"G",26/255.f},{"B",33/255.f}}}};
}
DepthViewer3D::DepthViewer3D():FlowCV::Node(ProcessOrder::OutOfOrder)
{
    SetComponentName_("Depth_Viewer_3D");SetComponentCategory_(Category::Category_Views);
    SetComponentAuthor_("Richard");SetComponentVersion_("0.1.0");
    SetInputCount_(3,{"color","depth","intrin"},{IoType::Io_Type_CvMat,IoType::Io_Type_CvMat,IoType::Io_Type_JSON});
    SetOutputCount_(0);SetEnabled(true);
    settings_=DefaultCamera();
    settings_.update(Json{{"diff_shading",true},{"diff_color",{{"R",.75f},{"G",.75f},{"B",.75f}}},
        {"point_size",1.5f},{"no_int_hfov",60.f},{"pos_offset",{{"x",0.f},{"y",-2.5f},{"z",0.f}}}});
}
Json DepthViewer3D::GetSettings() const {const std::lock_guard<std::mutex> lock(mutex_);return settings_;}
DepthViewer3D::Snapshot DepthViewer3D::GetSnapshot() const {const std::lock_guard<std::mutex> lock(mutex_);return snapshot_;}
void DepthViewer3D::InitializeView(){const std::lock_guard<std::mutex> lock(mutex_);view_initialized_=true;}
std::string DepthViewer3D::GetState()
{
    const std::lock_guard<std::mutex> lock(mutex_);auto state=settings_;
    if(snapshot_.has_intrinsic)state.erase("no_int_hfov");
    // Camera keys are conditional in the original state.
    if(!view_initialized_){const auto defaults=DefaultCamera();for(auto it=defaults.begin();it!=defaults.end();++it)state.erase(it.key());}
    return state.dump();
}
void DepthViewer3D::UpdateSettings(const Json& state)
{
    if(!state.is_object())throw std::invalid_argument("Depth_Viewer_3D state must be an object");
    const std::lock_guard<std::mutex> lock(mutex_);auto next=settings_;
    const auto number=[](const Json& v){if(!v.is_number()||!std::isfinite(v.get<double>())||std::abs(v.get<double>())>std::numeric_limits<float>::max())throw std::invalid_argument("Non-finite depth setting");return v.get<float>();};
    for(auto it=next.begin();it!=next.end();++it){
        if(!state.contains(it.key()))continue;
        const auto& value=state.at(it.key());
        if(it->is_boolean())*it=value.get<bool>();
        else if(it->is_object()){auto object=*it;for(auto f=object.begin();f!=object.end();++f)*f=number(value.at(f.key()));*it=object;}
        else *it=number(value);
    }
    if(next["point_size"].get<float>()<=0||next["zoom"].get<float>()<=0||next["fov"].get<float>()<=0||next["fov"].get<float>()>=180)
        throw std::invalid_argument("Invalid point size or camera projection");
    settings_=std::move(next);
}
void DepthViewer3D::SetState(std::string&& state){UpdateSettings(Json::parse(state));InitializeView();}
void DepthViewer3D::Process_(SignalBus const& inputs,SignalBus&)
{
    if(!IsEnabled())SetEnabled(true);
    const auto* input=inputs.GetValue<cv::Mat>(1);
    if(!input||input->empty())return;
    const auto s=GetSettings();Snapshot next;
    try {
        if(input->type()!=CV_16UC1||input->dims!=2)CV_Error(cv::Error::StsUnsupportedFormat,"Depth must be CV_16UC1");
        cv::Mat depth,color;cv::flip(*input,depth,1);
        if(const auto* image=inputs.GetValue<cv::Mat>(0);image&&!image->empty()){
            if(image->type()!=CV_8UC3||image->dims!=2)CV_Error(cv::Error::StsUnsupportedFormat,"Color must be CV_8UC3");
            cv::flip(*image,color,1);if(color.size()!=depth.size())cv::resize(color,color,depth.size(),0,0,cv::INTER_NEAREST);
        }
        const int w=depth.cols,h=depth.rows;float fx=0,fy=0,ppx=0,ppy=0;
        if(const auto* intr=inputs.GetValue<Json>(2);intr&&intr->contains("data")&&intr->at("data").is_array()&&!intr->at("data").empty()){
            const auto& first=intr->at("data").at(0);
            if(first.contains("intrinsics")&&first.at("intrinsics").contains("depth")){
                const auto& i=first.at("intrinsics").at("depth");fx=i.at("fx").get<float>();fy=i.at("fy").get<float>();ppx=i.at("ppx").get<float>();ppy=i.at("ppy").get<float>();next.has_intrinsic=true;
            }
        }
        if(!next.has_intrinsic){constexpr float pi=3.14159265358979323846f;const float fov=s.at("no_int_hfov");
            const float aspect=float(h)/w,vfov=2.f*std::atan(std::tan((fov*pi/180.f)/2.f)*aspect);
            ppx=float(w)/2.f;ppy=float(h)/2.f;fx=ppx/std::tan(fov*.5f*pi/180.f);fy=ppy/std::tan(vfov*.5f);
        }
        if(!std::isfinite(fx)||!std::isfinite(fy)||!std::isfinite(ppx)||!std::isfinite(ppy)||fx==0||fy==0)throw std::invalid_argument("Invalid depth intrinsics");
        const auto& off=s.at("pos_offset");const cv::Vec3f offset(off.at("x").get<float>(),off.at("y").get<float>(),off.at("z").get<float>());
        const auto& diffuse=s.at("diff_color");const cv::Vec3f rgb(diffuse.at("R").get<float>(),diffuse.at("G").get<float>(),diffuse.at("B").get<float>());
        next.points.resize(size_t(w)*h);
        // Fill positions before normals: upstream read not-yet-updated right neighbours.
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto& p=next.points[size_t(y)*w+x];const float z=float(depth.at<uint16_t>(y,x))*.01f;
            if(z<=0)continue;p.x=((float(x)-ppx)/fx)*z-offset[0];p.y=-((float(y)-ppy)/fy)*z-offset[1];p.z=z-offset[2];
            if(!color.empty()){const auto c=color.at<cv::Vec3b>(y,x);p.r=c[2]/255.f;p.g=c[1]/255.f;p.b=c[0]/255.f;}else{p.r=rgb[0];p.g=rgb[1];p.b=rgb[2];}p.a=1;
        }
        const auto position=[](const Vertex& p){return cv::Vec3f(p.x,p.y,p.z);};
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto& p=next.points[size_t(y)*w+x];if(p.a==0)continue;
            cv::Vec3f n(0,0,1);
            if(s.at("diff_shading").get<bool>()&&w>1&&h>1){
                const auto v1=position(p),v2=position(next.points[size_t(y)*w+(x<w-1?x+1:x-1)]),v3=position(next.points[size_t(y<h-1?y+1:y-1)*w+x]);
                if(y==h-1&&x==w-1)n=(v2-v1).cross(v1-v3);
                else if(y==h-1)n=(v1-v2).cross(v1-v3);
                else if(x==w-1)n=(v2-v1).cross(v3-v1);
                else n=(v1-v2).cross(v3-v1);
            }p.nx=n[0];p.ny=n[1];p.nz=n[2];
        }
    }catch(const std::exception& e){next.points.clear();next.error=e.what();}
    const std::lock_guard<std::mutex> lock(mutex_);next.revision=snapshot_.revision+1;snapshot_=std::move(next);
}
}
