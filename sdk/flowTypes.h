#pragma once

#include <dspatch/ComponentTypes.h>

#include <map>
#include <string>

namespace FlowCV 
{
    // 区分节点界面的用途
    enum GuiInterfaceType
    {
        GuiInterfaceType_Controls,      // 参数控制界面
        GuiInterfaceType_Main,          // 主显示界面
        GuiInterfaceType_Other          // 其他界面
    };

    struct NodeDescription
    {
        int input_count{};              // 输入端口数量
        int output_count{};             // 输出端口数量
        std::string name{};             // 节点名称
        DSPatch::Category category{};   // 节点分类：例如source、filter、Merge
        std::string author{};           // 作者
        std::string version{};          // 节点版本
    };
}	// namespace FlowCV

namespace DSPatch
{
    const std::map<DSPatch::Category, const char*>& getCategories();
}