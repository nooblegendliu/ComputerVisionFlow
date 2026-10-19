// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin OscSend
//

#ifndef FLOWCV_PLUGIN_OSC_SEND_HPP_
#define FLOWCV_PLUGIN_OSC_SEND_HPP_
#include "output_support.hpp"
#include <asio2/external/asio.hpp>
#include "OscOutboundPacketStream.h"

namespace DSPatch::DSPatchables
{
namespace internal
{
class OscSend;
}

class OscSend : public FlowCV::Node
{
  public:
    OscSend();
    ~OscSend() override;
    std::shared_ptr<OutputUiBridge> UiHandle() const { return ui_; }
    std::string Status();

    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void ApplyState_(const nlohmann::json& state, bool restoring);
    void Process_(SignalBus const &inputs, SignalBus &outputs) override;
    bool IsValidIP_();
    void OpenSocket_();
    void CloseSocket_();

  protected:
    mutable std::recursive_mutex mutex_;
    std::shared_ptr<OutputUiBridge> ui_ = std::make_shared<OutputUiBridge>();
    std::string error_;
    std::unique_ptr<internal::OscSend> p;
    std::string ip_addr_;
    std::string osc_addr_prefix_;
    char tmp_ip_buf[64] = {'\0'};
    int port_;
    int transmit_rate_;
    bool is_multicast_;
    bool is_valid_ip;
    float rate_val_{};
    int rate_selection_[9] = {0, 60, 30, 20, 15, 10, 5, 2, 1};
    std::chrono::steady_clock::time_point current_time_;
    std::chrono::steady_clock::time_point last_time_;
    asio::io_service io_service_;
    std::unique_ptr<asio::ip::udp::socket> socket_;
    std::unique_ptr<asio::ip::udp::endpoint> remote_endpoint_;
};



}  // namespace DSPatch::DSPatchables
#endif  // FLOWCV_PLUGIN_OSC_SEND_HPP_
