// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin OscSend
//

#include "OSC_Send.hpp"

using namespace DSPatch;
using namespace DSPatchables;


namespace DSPatch::DSPatchables::internal
{
class OscSend
{
};
}  // namespace DSPatch::DSPatchables::internal

OscSend::OscSend() : FlowCV::Node(ProcessOrder::OutOfOrder), p(new internal::OscSend())
{
    // Name and Category
    SetComponentName_("OSC_Send");
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
    is_multicast_ = false;
    is_valid_ip = false;
    transmit_rate_ = 0;
    current_time_ = std::chrono::steady_clock::now();
    last_time_ = current_time_;

    SetEnabled(true);
}


void OscSend::Process_(SignalBus const &inputs, SignalBus &outputs)
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
                        char buf[1024] = {'\0'};
                        osc::OutboundPacketStream oscStream(buf, 1024);
                        oscStream << osc::BeginMessage(osc_addr_prefix_.c_str());
                        oscStream << *in1;
                        oscStream << osc::EndMessage;
                        socket_->send_to(asio::buffer(oscStream.Data(), oscStream.Size()), *remote_endpoint_);
                    }
                }
                else if (in2) {
                    if (readyToSend) {
                        char buf[1024] = {'\0'};
                        osc::OutboundPacketStream oscStream(buf, 1024);
                        oscStream << osc::BeginMessage(osc_addr_prefix_.c_str());
                        oscStream << *in2;
                        oscStream << osc::EndMessage;
                        socket_->send_to(asio::buffer(oscStream.Data(), oscStream.Size()), *remote_endpoint_);
                    }
                }
                else if (in3) {
                    if (readyToSend) {
                        char buf[1024] = {'\0'};
                        osc::OutboundPacketStream oscStream(buf, 1024);
                        oscStream << osc::BeginMessage(osc_addr_prefix_.c_str());
                        oscStream << *in3;
                        oscStream << osc::EndMessage;
                        socket_->send_to(asio::buffer(oscStream.Data(), oscStream.Size()), *remote_endpoint_);
                    }
                }
                else if (in4) {
                    if (readyToSend) {
                        char buf[1024] = {'\0'};
                        osc::OutboundPacketStream oscStream(buf, 1024);
                        std::string oscAddr = osc_addr_prefix_;
                        if (oscAddr.empty() || oscAddr.back() != '/')
                            oscAddr += '/';
                        oscAddr += *in4;
                        oscStream << osc::BeginMessage(oscAddr.c_str());
                        // oscStream << 0;
                        oscStream << osc::EndMessage;
                        socket_->send_to(asio::buffer(oscStream.Data(), oscStream.Size()), *remote_endpoint_);
                    }
                }
                else if (in5) {
                    if (!in5->empty()) {
                        nlohmann::json json_in_ = *in5;
                        if (readyToSend) {
                            std::string oscBaseAddr;
                            if (!osc_addr_prefix_.empty()) {
                                if (osc_addr_prefix_[0] == '/')
                                    oscBaseAddr += osc_addr_prefix_;
                                else {
                                    oscBaseAddr += '/';
                                    oscBaseAddr += osc_addr_prefix_;
                                }
                            }
                            if (!oscBaseAddr.empty()) {
                                if (oscBaseAddr.at(oscBaseAddr.size() - 1) != '/')
                                    oscBaseAddr += '/';
                            }
                            else
                                oscBaseAddr += '/';
                            char buf[8192] = {'\0'};
                            osc::OutboundPacketStream oscStream(buf, 8192);
                            oscStream << osc::BeginBundleImmediate;
                            if (json_in_.contains("ref_frame")) {
                                std::string oscAddr = oscBaseAddr;
                                oscAddr += "ref_frame";
                                oscStream << osc::BeginMessage(oscAddr.c_str());
                                oscStream << json_in_["ref_frame"]["w"].get<int>();
                                oscStream << json_in_["ref_frame"]["h"].get<int>();
                                oscStream << osc::EndMessage;
                            }
                            std::string dataType = "data";
                            if (json_in_.contains("data_type")) {
                                dataType += '/';
                                dataType += json_in_["data_type"].get<std::string>();
                            }
                            if (json_in_.contains("data")) {
                                if (!json_in_["data"].empty()) {
                                    int idx = 0;
                                    for (auto &d : json_in_["data"]) {
                                        for (nlohmann::json::iterator it = d.begin(); it != d.end(); ++it) {
                                            std::string oscAddr = oscBaseAddr;
                                            oscAddr += dataType;
                                            oscAddr += '/';
                                            oscAddr += it.key();
                                            oscAddr += '/';
                                            oscAddr += std::to_string(idx);
                                            if (it->is_string()) {
                                                oscAddr += '/';
                                                oscAddr += it->get<std::string>();
                                            }
                                            oscStream << osc::BeginMessage(oscAddr.c_str());
                                            if (it->is_number_float())
                                                oscStream << it->get<float>();
                                            else if (it->is_number_integer())
                                                oscStream << it->get<int>();
                                            else if (it->is_boolean()) {
                                                oscStream << it->get<bool>();
                                            }
                                            else if (it->is_array()) {
                                                for (int i = 0; i < it->size(); i++) {
                                                    if (it->at(i).is_number_float())
                                                        oscStream << it->at(i).get<float>();
                                                    else if (it->at(i).is_number_integer())
                                                        oscStream << it->at(i).get<int>();
                                                }
                                            }
                                            else if (it->is_object()) {  // TODO: Handle Deeper Level JSON Objects
                                            }
                                            oscStream << osc::EndMessage;
                                        }
                                        idx++;
                                    }
                                }
                            }
                            oscStream << osc::EndBundle;
                            socket_->send_to(asio::buffer(oscStream.Data(), oscStream.Size()), *remote_endpoint_);
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

std::string OscSend::GetState()
{
    std::lock_guard lock(mutex_);
    using namespace nlohmann;

    json state;

    state["ip_addr"] = ip_addr_;
    state["port"] = port_;
    state["data_rate"] = transmit_rate_;
    state["osc_addr"] = osc_addr_prefix_;
    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void OscSend::SetState(std::string &&json_serialized)
{
    ApplyState_(nlohmann::json::parse(json_serialized), true);
}

void OscSend::ApplyState_(const nlohmann::json& state, bool restoring)
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
    if (state.contains("osc_addr")) {
        osc_addr_prefix_ = state["osc_addr"].get<std::string>();
    }
    if (state.contains("data_rate")) {
        transmit_rate_ = state["data_rate"].get<int>();
        if (transmit_rate_ > 0 && transmit_rate_ < 9)
            rate_val_ = (1.0f / (float)rate_selection_[transmit_rate_]) * 1000.0f;
        else
            rate_val_ = 0;
    }

    if (reopen) {
        CloseSocket_();
        if (IsValidIP_() && port_ > 0 && port_ <= 65535)
            try { OpenSocket_(); } catch (const std::exception& e) { error_=e.what(); }
    }
}

void OscSend::OpenSocket_()
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

void OscSend::CloseSocket_()
{
    if (socket_ && socket_->is_open()) { std::error_code ec; socket_->close(ec); }
}

bool OscSend::IsValidIP_()
{
    std::error_code ec;
    const auto address = asio::ip::address::from_string(ip_addr_, ec);
    is_valid_ip = !ec;
    if (ec) error_ = ec.message();
    return is_valid_ip;
}

OscSend::~OscSend() { InvalidateUi(ui_); std::lock_guard lock(mutex_); CloseSocket_(); }

std::string OscSend::Status() { std::lock_guard lock(mutex_); return !error_.empty() ? error_ : (socket_ && socket_->is_open()) ? "Connected" : "Disconnected"; }

#ifndef DATA_OUTPUT_WITH_QT
EXPORT_PLUGIN(OscSend)
#endif
