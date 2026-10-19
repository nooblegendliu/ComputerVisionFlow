// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin SerialSend
//

#include "Serial_Send.hpp"

using namespace DSPatch;
using namespace DSPatchables;


namespace DSPatch::DSPatchables::internal
{
class SerialSend
{
};
}  // namespace DSPatch::DSPatchables::internal

SerialSend::SerialSend() : FlowCV::Node(ProcessOrder::OutOfOrder), p(new internal::SerialSend())
{
    // Name and Category
    SetComponentName_("Serial_Send");
    SetComponentCategory_(Category::Category_Output);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    ui_->node = this;
    ui_->status = [this] { return Status(); };
    ui_->updateControls = [this](const nlohmann::json& patch) { ApplyState_(patch, false); };

    // 1 inputs
    SetInputCount_(5, {"bool", "int", "float", "str", "json"},
        {IoType::Io_Type_Bool, IoType::Io_Type_Int, IoType::Io_Type_Float, IoType::Io_Type_String, IoType::Io_Type_JSON});

    // 0 outputs
    SetOutputCount_(0);

    sp_ = std::make_unique<asio2::serial_port>();
    se_.RefreshSerialList();
    ui_->ports = [this] { return SerialPorts(); };
    ui_->refreshPorts = [this] { RefreshPorts(); };
    // Supported Baud Rates
    baud_rate_.emplace_back("300");
    baud_rate_.emplace_back("600");
    baud_rate_.emplace_back("1200");
    baud_rate_.emplace_back("1800");
    baud_rate_.emplace_back("2400");
    baud_rate_.emplace_back("4800");
    baud_rate_.emplace_back("7200");
    baud_rate_.emplace_back("9600");
    baud_rate_.emplace_back("14400");
    baud_rate_.emplace_back("19200");
    baud_rate_.emplace_back("28800");
    baud_rate_.emplace_back("33600");
    baud_rate_.emplace_back("38400");
    baud_rate_.emplace_back("57600");
    baud_rate_.emplace_back("115200");
    baud_rate_.emplace_back("128000");
    baud_rate_.emplace_back("230400");
    baud_rate_.emplace_back("256000");
    baud_rate_.emplace_back("460800");
    baud_rate_.emplace_back("921600");
    baud_rate_index_ = 14;
    flow_control_index_ = 0;
    parity_index_ = 0;
    stop_bits_index_ = 0;
    char_size_index_ = 3;
    data_pack_mode_ = 3;
    send_as_binary_ = true;
    transmit_rate_ = 0;
    eol_seq_index_ = 0;
    serial_list_index_ = 0;
    current_time_ = std::chrono::steady_clock::now();
    last_time_ = current_time_;

    sp_->bind_init([&]() {
        sp_->socket().set_option(asio::serial_port::flow_control((asio::serial_port::flow_control::type)flow_control_index_));
        sp_->socket().set_option(asio::serial_port::parity((asio::serial_port::parity::type)parity_index_));
        sp_->socket().set_option(asio::serial_port::stop_bits((asio::serial_port::stop_bits::type)stop_bits_index_));
        sp_->socket().set_option(asio::serial_port::character_size(char_size_index_ + 5));
    });

    SetEnabled(true);
}

void SerialSend::SetEOLSeq_()
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

template<typename T> std::vector<uint8_t> SerialSend::GenerateOutBuffer_(T data)
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

void SerialSend::Process_(SignalBus const &inputs, SignalBus &outputs)
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
        if (sp_ != nullptr) {
            if (sp_->is_started()) {
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
                        sp_->async_send(buf);
                    }
                }
                else if (in2) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<int>(*in2);
                        sp_->async_send(buf);
                    }
                }
                else if (in3) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<float>(*in3);
                        sp_->async_send(buf);
                    }
                }
                else if (in4) {
                    if (readyToSend) {
                        std::vector<uint8_t> buf = GenerateOutBuffer_<char>(in4->empty() ? '\0' : in4->front());
                        sp_->async_send(buf);
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
                                    sp_->async_send(msg);
                                    break;
                                }
                                case 2:  // CBOR
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_cbor(json_in_);
                                    sp_->async_send(msg);
                                    break;
                                }
                                case 3:  // MessagePack
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_msgpack(json_in_);
                                    sp_->async_send(msg);
                                    break;
                                }
                                case 4:  // UBJSON
                                {
                                    std::vector<uint8_t> msg = nlohmann::json::to_ubjson(json_in_);
                                    sp_->async_send(msg);
                                    break;
                                }
                                default:  // None (0)
                                    std::string jsonStr = json_in_.dump();
                                    for (int i = 0; i < 2; i++) {
                                        if (eol_seq_[i] != '\0')
                                            jsonStr += eol_seq_[i];
                                    }
                                    sp_->async_send(jsonStr);
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

std::string SerialSend::GetState()
{
    std::lock_guard lock(mutex_);
    using namespace nlohmann;

    json state;

    state["port_str"] = se_.GetSerialName(serial_list_index_);
    state["port_idx"] = serial_list_index_;
    state["baud_rate_idx"] = baud_rate_index_;
    state["data_bits_idx"] = char_size_index_;
    state["parity_idx"] = parity_index_;
    state["stop_bits_idx"] = stop_bits_index_;
    state["flow_ctrl_idx"] = flow_control_index_;
    state["data_mode"] = data_pack_mode_;
    state["data_rate"] = transmit_rate_;
    state["eol_seq"] = eol_seq_index_;
    state["send_binary"] = send_as_binary_;

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void SerialSend::SetState(std::string &&json_serialized)
{
    ApplyState_(nlohmann::json::parse(json_serialized), true);
}

void SerialSend::ApplyState_(const nlohmann::json& state, bool restoring)
{
    std::lock_guard lock(mutex_);
    const auto current = nlohmann::json::parse(GetState());
    ValidateOutputState(current, state);
    // Loading a project restores resources; live edits follow the original Controls triggers.
    const bool reopen = restoring || OutputSettingsChanged(current, state, {"port_str", "port_idx", "baud_rate_idx", "data_bits_idx", "parity_idx", "stop_bits_idx", "flow_ctrl_idx"});
    if (reopen) error_.clear();

    std::string portStr;
    if (state.contains("port_str"))
        portStr = state["port_str"].get<std::string>();
    if (state.contains("port_idx")) {
        serial_list_index_ = state["port_idx"].get<int>();
        se_.RefreshSerialList();
        if (portStr != se_.GetSerialName(serial_list_index_)) {
            bool foundMatch = false;
            for (int i = 0; i < se_.GetSerialCount(); i++) {
                if (portStr == se_.GetSerialName(i)) {
                    serial_list_index_ = i;
                    foundMatch = true;
                    break;
                }
            }
            if (!foundMatch) {
                serial_list_index_ = 0;
            }
        }
    }
    if (state.contains("baud_rate_idx"))
        baud_rate_index_ = state["baud_rate_idx"].get<int>();
    if (state.contains("data_bits_idx"))
        char_size_index_ = state["data_bits_idx"].get<int>();
    if (state.contains("parity_idx"))
        parity_index_ = state["parity_idx"].get<int>();
    if (state.contains("stop_bits_idx"))
        stop_bits_index_ = state["stop_bits_idx"].get<int>();
    if (state.contains("flow_ctrl_idx"))
        flow_control_index_ = state["flow_ctrl_idx"].get<int>();

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

    if (reopen && (!restoring || se_.GetSerialCount() > 1)) {
        // Live selection of None also releases a former port after a list refresh.
        OpenSerialConn_();
    }
}

void SerialSend::OpenSerialConn_()
{
    if (sp_ != nullptr) {
        if (sp_->is_started())
            sp_->stop();

        if (serial_list_index_ < se_.GetSerialCount() && serial_list_index_ > 0) {
            if (!sp_->start(se_.GetSerialName(serial_list_index_), baud_rate_.at(baud_rate_index_))) error_ = asio2::last_error_msg();
        }
    }
}

void SerialSend::CloseSerialConn_()
{
    if (sp_) sp_->stop();
}

SerialSend::~SerialSend() { InvalidateUi(ui_); std::lock_guard lock(mutex_); CloseSerialConn_(); }

std::string SerialSend::Status() { std::lock_guard lock(mutex_); return !error_.empty() ? error_ : (sp_ && sp_->is_started()) ? "Connected" : "Disconnected"; }

std::vector<std::string> SerialSend::SerialPorts() { std::lock_guard lock(mutex_); std::vector<std::string> result; for (int i=0;i<se_.GetSerialCount();++i) result.push_back(se_.GetSerialName(i)); return result; }
void SerialSend::RefreshPorts() { std::lock_guard lock(mutex_); const auto current=se_.GetSerialName(serial_list_index_); se_.RefreshSerialList(); serial_list_index_=0; for(int i=0;i<se_.GetSerialCount();++i) if(se_.GetSerialName(i)==current) { serial_list_index_=i; break; } if(serial_list_index_==0) CloseSerialConn_(); }

#ifndef DATA_OUTPUT_WITH_QT
EXPORT_PLUGIN(SerialSend)
#endif
