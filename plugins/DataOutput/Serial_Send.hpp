// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin SerialSend
//

#ifndef FLOWCV_PLUGIN_SERIAL_SEND_HPP_
#define FLOWCV_PLUGIN_SERIAL_SEND_HPP_
#include "output_support.hpp"
#include <asio2/serial_port/serial_port.hpp>
#include "serial_enumerator.hpp"

namespace DSPatch::DSPatchables
{
namespace internal
{
class SerialSend;
}

class SerialSend : public FlowCV::Node
{
  public:
    SerialSend();
    ~SerialSend() override;
    std::shared_ptr<OutputUiBridge> UiHandle() const { return ui_; }
    std::string Status();
    std::vector<std::string> SerialPorts();
    void RefreshPorts();

    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void ApplyState_(const nlohmann::json& state, bool restoring);
    void Process_(SignalBus const &inputs, SignalBus &outputs) override;
    void OpenSerialConn_();
    void CloseSerialConn_();
    void SetEOLSeq_();
    template<typename T> std::vector<uint8_t> GenerateOutBuffer_(T data);

  protected:
    mutable std::recursive_mutex mutex_;
    std::shared_ptr<OutputUiBridge> ui_ = std::make_shared<OutputUiBridge>();
    std::string error_;
    std::unique_ptr<internal::SerialSend> p;
    Serial_Enumerator se_;
    char eol_seq_[3] = {'\0'};
    int eol_seq_index_;
    int data_pack_mode_;
    int transmit_rate_;
    bool send_as_binary_;
    float rate_val_{};
    int serial_list_index_;
    std::vector<std::string> baud_rate_;
    int baud_rate_index_;
    int flow_control_index_;
    int parity_index_;
    int stop_bits_index_;
    int char_size_index_;
    int rate_selection_[9] = {0, 60, 30, 20, 15, 10, 5, 2, 1};
    std::chrono::steady_clock::time_point current_time_;
    std::chrono::steady_clock::time_point last_time_;
    std::unique_ptr<asio2::serial_port> sp_;
};



}  // namespace DSPatch::DSPatchables
#endif  // FLOWCV_PLUGIN_SERIAL_SEND_HPP_
