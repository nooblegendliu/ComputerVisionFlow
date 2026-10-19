// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin UdpSend
//

#include "UDP_Send.hpp"

using namespace DSPatch;
using namespace DSPatchables;


namespace DSPatch::DSPatchables::internal
{
class UdpSend
{
};
}  // namespace DSPatch::DSPatchables::internal

UdpSend::UdpSend() : FlowCV::Node(ProcessOrder::OutOfOrder), p(new internal::UdpSend())
{
    // Name and Category
    SetComponentName_("UDP_Send");
    SetComponentCategory_(Category::Category_Output);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    ui_->node = this;
    ui_->status = [this] { return Status(); };
    ui_->updateControls = [this](const nlohmann::json& patch) { ApplyState_(patch, false); };
    ui_->multicast = [this] { std::lock_guard lock(mutex_); return is_multicast_; };

    // 1 inputs
    SetInputCount_(5, {"bool", "int", "float", "str", "json"},
        {IoType::Io_Type_Bool, IoType::Io_Type_Int, IoType::Io_Type_Float, IoType::Io_Type_String, IoType::Io_Type_JSON});

    // 0 outputs
    SetOutputCount_(0);

    port_ = 0;
    data_pack_mode_ = 3;
    is_multicast_ = false;
    is_valid_ip = false;
    send_as_binary_ = true;
    transmit_rate_ = 0;
    eol_seq_index_ = 0;
    current_time_ = std::chrono::steady_clock::now();
    last_time_ = current_time_;

    SetEnabled(true);
}


void UdpSend::SetEOLSeq_()
{
    memset(eol_seq_, '\0', 3);

    switch (eol_seq_index_) {
        case 1:
            eol_seq_[0] = '\r';
            break;
        case 2:
            eol_seq_[0] = '\n';
            break;
        case 3:
            eol_seq_[0] = '\r';
            eol_seq_[1] = '\n';
            break;
        case 4:
            eol_seq_[0] = ' ';
            break;
        case 5:
            eol_seq_[0] = '\t';
            break;
        case 6:
            eol_seq_[0] = ',';
            break;
        default:
            eol_seq_[0] = '\0';
            break;
    }
}

template<typename T> std::vector<uint8_t> UdpSend::GenerateOutBuffer_(T data)
{
    std::vector<uint8_t> outBuffer;
    if (send_as_binary_) {
        auto *d = reinterpret_cast<uint8_t *>(&data);
        for (int i = 0; i < sizeof(T); i++) {
            outBuffer.emplace_back(d[i]);
        }
    }
    else {
        std::string buf;
        if (typeid(T).name() == typeid(char).name())
            buf = data;
        else
            buf = std::to_string(data);
        for (char &i : buf) {
            outBuffer.emplace_back(i);
        }
    }
    for (int i = 0; i < 2; i++) {
        if (eol_seq_[i] != '\0')
            outBuffer.emplace_back(eol_seq_[i]);
    }
    return outBuffer;
}

void UdpSend::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    std::lock_guard lock(mutex_);
    try {

    // Input Handler
    auto in1 = inputs.GetValue<bool>(0);
    auto in2 = inputs.GetValue<int>(1);
    auto in3 = inputs.GetValue<float>(2);
    auto in4 = inputs.GetValue<std::string>(3);
    auto in5 = inputs.GetValue<nlohmann::json>(4);

    if (IsEnabled()) {
        if (socket_ != nullptr) {
            if (socket_->is_open()) {
                current_time_ = std::chrono::steady_clock::now();
                auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(current_time_ - last_time_).count();
                bool readyToSend = false;
                if (transmit_rate_ > 0 && transmit_rate_ < 9) {
                    if (delta >= (long long)rate_val_) {
                        readyToSend = true;
                    }
                }
                else
                    readyToSend = true;

                if (in1) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<bool>(*in1);
                        socket_->send_to(asio::buffer(buf.data(), buf.size()), *remote_endpoint_);
                    }
                }
                else if (in2) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<int>(*in2);
                        socket_->send_to(asio::buffer(buf.data(), buf.size()), *remote_endpoint_);
                    }
                }
                else if (in3) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<float>(*in3);
                        socket_->send_to(asio::buffer(buf.data(), buf.size()), *remote_endpoint_);
                    }
                }
                else if (in4) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<char>(in4->empty() ? '\0' : in4->front());
                        socket_->send_to(asio::buffer(buf.data(), buf.size()), *remote_endpoint_);
                    }
                }
                else if (in5) {
                    if (!in5->empty()) {
                        nlohmann::json json_in_ = *in5;
                        if (readyToSend) {
                            switch (data_pack_mode_) {
                                case 1:  // BSON
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_bson(json_in_);
                                    socket_->send_to(asio::buffer(msg.data(), msg.size()), *remote_endpoint_);
                                    break;
                                }
                                case 2:  // CBOR
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_cbor(json_in_);
                                    socket_->send_to(asio::buffer(msg.data(), msg.size()), *remote_endpoint_);
                                    break;
                                }
                                case 3:  // MessagePack
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(json_in_);
                                    socket_->send_to(asio::buffer(msg.data(), msg.size()), *remote_endpoint_);
                                    break;
                                }
                                case 4:  // UBJSON
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_ubjson(json_in_);
                                    socket_->send_to(asio::buffer(msg.data(), msg.size()), *remote_endpoint_);
                                    break;
                                }
                                default:  // None (0)
                                    std::string jsonStr = json_in_.dump();
                                    for (int i = 0; i < 2; i++) {
                                        if (eol_seq_[i] != '\0')
                                            jsonStr += eol_seq_[i];
                                    }
                                    socket_->send_to(asio::buffer(jsonStr), *remote_endpoint_, 0);
                            }
                        }
                    }
                }
                if (readyToSend) {
                    float curRate = 1.0f / ((float)delta * 0.001f);
                    // std::cout << curRate << std::endl;
                    last_time_ = current_time_;
                }
            }
        }
    }

    } catch (const std::exception& e) { error_ = e.what(); }
}

std::string UdpSend::GetState()
{
    std::lock_guard lock(mutex_);
    using namespace nlohmann;

    json state;

    state["ip_addr"] = ip_addr_;
    state["port"] = port_;
    state["data_mode"] = data_pack_mode_;
    state["data_rate"] = transmit_rate_;
    state["eol_seq"] = eol_seq_index_;
    state["send_binary"] = send_as_binary_;

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void UdpSend::SetState(std::string &&json_serialized)
{
    ApplyState_(nlohmann::json::parse(json_serialized), true);
}

void UdpSend::ApplyState_(const nlohmann::json& state, bool restoring)
{
    std::lock_guard lock(mutex_);
    const auto current = nlohmann::json::parse(GetState());
    ValidateOutputState(current, state);
    // Loading a project restores resources; live edits follow the original Controls triggers.
    const bool reopen = restoring || OutputSettingsChanged(current, state, {"ip_addr", "port"});
    if (reopen) error_.clear();

    if (state.contains("ip_addr")) {
        ip_addr_ = state["ip_addr"].get<std::string>();
        
    }
    if (state.contains("port"))
        port_ = state["port"].get<int>();
    if (state.contains("data_mode"))
        data_pack_mode_ = state["data_mode"].get<int>();
    if (state.contains("data_rate")) {
        transmit_rate_ = state["data_rate"].get<int>();
        if (transmit_rate_ > 0 && transmit_rate_ < 9)
            rate_val_ = (1.0f / (float)rate_selection_[transmit_rate_]) * 1000.0f;
        else
            rate_val_ = 0;
    }
    if (state.contains("send_binary"))
        send_as_binary_ = state["send_binary"].get<bool>();
    if (state.contains("eol_seq")) {
        eol_seq_index_ = state["eol_seq"].get<int>();
        SetEOLSeq_();
    }

    if (reopen) {
        CloseSocket_();
        if (IsValidIP_() && port_ > 0 && port_ <= 65535)
            try { OpenSocket_(); } catch (const std::exception& e) { error_=e.what(); }
    }
}

void UdpSend::OpenSocket_()
{
    if (IsValidIP_()) {
        auto address = asio::ip::address::from_string(ip_addr_);
        if (socket_ == nullptr)
            socket_ = std::make_unique<asio::ip::udp::socket>(io_service_);
        else {
            if (socket_->is_open())
                socket_->close();
        }
        remote_endpoint_ = std::make_unique<asio::ip::udp::endpoint>(address, port_);
        is_multicast_ = false;
        if (address.is_v4()) {
            socket_->open(asio::ip::udp::v4());
            if (address.is_multicast())
                is_multicast_ = true;
        }
        else if (address.is_v6()) {
            socket_->open(asio::ip::udp::v6());
            if (address.is_multicast())
                is_multicast_ = true;
        }
    }
}

void UdpSend::CloseSocket_()
{
    if (socket_ && socket_->is_open()) { std::error_code ec; socket_->close(ec); }
}

bool UdpSend::IsValidIP_()
{
    std::error_code ec;
    const auto address = asio::ip::address::from_string(ip_addr_, ec);
    is_valid_ip = !ec;
    if (ec) error_ = ec.message();
    return is_valid_ip;
}

UdpSend::~UdpSend() { InvalidateUi(ui_); std::lock_guard lock(mutex_); CloseSocket_(); }

std::string UdpSend::Status() { std::lock_guard lock(mutex_); return !error_.empty() ? error_ : (socket_ && socket_->is_open()) ? "Connected" : "Disconnected"; }

#ifndef DATA_OUTPUT_WITH_QT
EXPORT_PLUGIN(UdpSend)
#endif
