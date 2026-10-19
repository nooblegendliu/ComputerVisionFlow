#pragma once

#include <flowcv_manager.h>

// Nested editor operations preserve the caller's running/stopped state.
class AutoTickPause final
{
public:
    explicit AutoTickPause(FlowCV::FlowCV_Manager& manager)
        : manager_(manager), resume_(manager.IsAutoTickRunning())
    {
        if (resume_) manager_.StopAutoTick();
    }
    ~AutoTickPause() { if (resume_) manager_.StartAutoTick(); }
    AutoTickPause(const AutoTickPause&) = delete;
    AutoTickPause& operator=(const AutoTickPause&) = delete;
private:
    FlowCV::FlowCV_Manager& manager_;
    bool resume_;
};
