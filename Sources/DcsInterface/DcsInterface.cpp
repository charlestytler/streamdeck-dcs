// Copyright 2020 Charles Tytler

#include "pch.h"

#include "DcsInterface.h"
#include "NumericStringUtilities.h"

DcsInterface::DcsInterface(const std::string &rx_port, const std::string &tx_port, const std::string &ip_address)
    : dcs_socket_(rx_port, tx_port, ip_address) {}

void DcsInterface::update_dcs_state() {
    // Receive next UDP message from DCS and strip header.
    const char header_delimiter = '*'; // Header content ends in an '*'.
    std::stringstream recv_msg = dcs_socket_.DcsReceive();

    std::string token;
    if (std::getline(recv_msg, token, header_delimiter)) {
        // Iterate through tokens received from single message.
        const char token_delimiter = ':';
        while (std::getline(recv_msg, token, token_delimiter)) {
            // Parse token string of the form:
            //   "<dcs_id>=<reported_value>"
            const std::string delim = "=";
            const auto delim_loc = token.find(delim);
            const std::string key = token.substr(0, delim_loc);

            // Reported value is found after delimiter until a newline character or end of string.
            const auto token_start = delim_loc + delim.size();
            const auto token_end = token.find('\n');
            const std::string reported_value = token.substr(token_start, token_end - token_start);
            handle_received_token(key, reported_value);
        }
    }
}

std::string DcsInterface::get_current_dcs_module() { return current_game_module_; }

std::string DcsInterface::get_value_of_dcs_id(const int dcs_id) {
    if (current_game_state_.count(dcs_id) > 0) {
        return current_game_state_[dcs_id];
    } else {
        return "";
    }
}

void DcsInterface::send_dcs_command(const int button_id, const std::string &device_id, const std::string &value) {
    const std::string message_assembly = "C" + device_id + "," + std::to_string(3000 + button_id) + "," + value;
    dcs_socket_.DcsSend(message_assembly);
}

void DcsInterface::send_dcs_reset_command() { dcs_socket_.DcsSend("R"); }

void DcsInterface::clear_game_state() { current_game_state_.clear(); }

std::map<int, std::string> DcsInterface::debug_get_current_game_state() { return current_game_state_; }

void DcsInterface::handle_received_token(const std::string &key, const std::string &value) {
    if (is_integer(key)) {
        current_game_state_[std::stoi(key)] = value;
    } else if (key == "File") {
        current_game_module_ = value;
    } else if (key == "Ikarus" || key == "DAC") {
        // Stop is received when user has quit mission -- game state should be cleared.
        if (value == "stop") {
            clear_game_state();
            current_game_module_ = "";
        }
    }
}