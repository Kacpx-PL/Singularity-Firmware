#include "ir_service.h"
#define IR_TX_PIN 44
#define IR_RX_PIN 1
#include <IRremote.hpp>

void ir_init() {
    IrSender.begin(DISABLE_LED_FEEDBACK);
    IrSender.setSendPin(IR_TX_PIN);

    IrReceiver.begin(IR_RX_PIN, DISABLE_LED_FEEDBACK);
}

void ir_send_protocol(const std::string& protocol, uint32_t address, uint32_t command) {
    if (protocol == "NEC" || protocol == "NECext" || protocol == "ONKYO" || protocol == "APPLE") {
        IrSender.sendNEC((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "DENON" || protocol == "SHARP") {
        IrSender.sendDenon((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "PANASONIC" || protocol == "KASEIKYO") {
        IrSender.sendPanasonic((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "JVC") {
        IrSender.sendJVC((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "LG") {
        IrSender.sendLG((uint16_t)address, (uint32_t)command, 0);
    } else if (protocol == "RC5") {
        IrSender.sendRC5((uint8_t)address, (uint8_t)command, 0);
    } else if (protocol == "RC6") {
        IrSender.sendRC6((uint8_t)address, (uint8_t)command, 0);
    } else if (protocol == "SAMSUNG") {
        IrSender.sendSamsung((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "SONY") {
        IrSender.sendSony((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "MARANTZ") {
        IrSender.sendDenon((uint16_t)address, (uint8_t)command, 0);
    } else if (protocol == "BOSEWAVE") {
        IrSender.sendBoseWave((uint8_t)command, 0);
    } else if (protocol == "LEGO_PF" || protocol == "LEGO") {
        IrSender.sendLegoPowerFunctions((uint16_t)address, (uint8_t)command, 0, true);
    } else if (protocol == "FAST") {
        IrSender.sendFAST((uint8_t)command, 0);
    } else if (protocol == "WHYNTER") {
        IrSender.sendWhynter((uint16_t)address, (uint8_t)command);
    } else if (protocol == "MAGIQUEST") {
        IrSender.sendMagiQuest((uint32_t)address, (uint16_t)command);
    }
}

void ir_send_raw(uint16_t* data, size_t len, uint16_t frequency_khz) {
    IrSender.sendRaw(data, len, frequency_khz);
}

bool ir_receive_available() {
    return IrReceiver.decode();
}

std::string ir_receive_get_result() {
    auto& r = IrReceiver.decodedIRData;

    std::string protocol = getProtocolString(r.protocol);
    char buf[128];
    snprintf(buf, sizeof(buf), "protocol=%s address=0x%04X command=0x%02X",
             protocol.c_str(), r.address, r.command);

    return std::string(buf);
}

uint16_t ir_receive_get_address() {
    return IrReceiver.decodedIRData.address;
}

uint8_t ir_receive_get_command() {
    return IrReceiver.decodedIRData.command;
}

std::string ir_receive_get_protocol() {
    return getProtocolString(IrReceiver.decodedIRData.protocol);
}

void ir_receive_resume() {
    IrReceiver.resume();
}

