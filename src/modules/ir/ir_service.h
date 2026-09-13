#pragma once
#include <cstdint>
#include <string>

void ir_init();
void ir_send_protocol(const std::string& protocol, uint32_t address, uint32_t command);
void ir_send_raw(uint16_t* data, size_t len, uint16_t frequency_khz);

bool ir_receive_available();
std::string ir_receive_get_result(); // returns a description string once a signal is captured
void ir_receive_resume();             // call after reading a result, to listen for the next one
uint16_t ir_receive_get_address();
uint8_t ir_receive_get_command();
std::string ir_receive_get_protocol();