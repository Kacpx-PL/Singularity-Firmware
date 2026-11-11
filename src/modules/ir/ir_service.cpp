#include "ir_service.h"
#include "../../core/pins.h"
#define IR_TX_PIN SG_PIN_IR_TX
#define IR_RX_PIN SG_PIN_IR_RX
#include <IRremote.hpp>
#include <cctype>

void ir_init() {
    IrSender.begin(DISABLE_LED_FEEDBACK);
    IrSender.setSendPin(IR_TX_PIN);

    IrReceiver.begin(IR_RX_PIN, DISABLE_LED_FEEDBACK);
}

// "NECext", "necext", "NEC-EXT" and "NEC_ext" all become "NECEXT"; this makes matching
// independent of case and separators, so both IRremote names (what ir.receiveGetProtocol()
// returns) and Flipper .ir names (Samsung32, SIRC15, RC5X, ...) work.
static std::string normalize_protocol(const std::string& in) {
    std::string out;
    for (char c : in) {
        if (std::isalnum((unsigned char)c)) out += (char)std::toupper((unsigned char)c);
    }
    return out;
}

bool ir_send_protocol(const std::string& protocol_name, uint32_t address, uint32_t command) {
    const std::string p = normalize_protocol(protocol_name);

    if (p == "NEC" || p == "NEC2" || p == "NECEXT" || p == "APPLE") {
        // an address above 0xFF is sent as an extended 16-bit address (Flipper "NECext")
        IrSender.sendNEC((uint16_t)address, (uint8_t)command, 0);
    } else if (p == "ONKYO") {
        // Onkyo = NEC framing with a 16-bit address AND a 16-bit command (no inverted bytes)
        IrSender.sendNECRaw((address & 0xFFFF) | ((command & 0xFFFF) << 16), 0);
    } else if (p == "DENON" || p == "SHARP" || p == "MARANTZ") {
        IrSender.sendDenon((uint16_t)address, (uint8_t)command, 0);
    } else if (p == "PANASONIC" || p == "KASEIKYO") {
        IrSender.sendPanasonic((uint16_t)address, (uint8_t)command, 0);
    } else if (p == "JVC") {
        IrSender.sendJVC((uint16_t)address, (uint8_t)command, 0);
    } else if (p == "LG") {
        IrSender.sendLG((uint16_t)address, (uint32_t)command, 0);
    } else if (p == "RC5" || p == "RC5X") {
        IrSender.sendRC5((uint8_t)address, (uint8_t)command, 0);
    } else if (p == "RC6") {
        IrSender.sendRC6((uint8_t)address, (uint8_t)command, 0);
    } else if (p == "SAMSUNG" || p == "SAMSUNG32") {
        IrSender.sendSamsung((uint16_t)address, (uint16_t)command, 0);
    } else if (p == "SONY" || p == "SIRC") {
        IrSender.sendSony((uint16_t)address, (uint8_t)command, 0, 12);
    } else if (p == "SIRC15") {
        IrSender.sendSony((uint16_t)address, (uint8_t)command, 0, 15);
    } else if (p == "SIRC20") {
        IrSender.sendSony((uint16_t)address, (uint8_t)command, 0, 20);
    } else if (p == "BOSEWAVE") {
        IrSender.sendBoseWave((uint8_t)command, 0);
    } else if (p == "LEGOPF" || p == "LEGO") {
        IrSender.sendLegoPowerFunctions((uint16_t)address, (uint8_t)command, 0, true);
    } else if (p == "FAST") {
        IrSender.sendFAST((uint8_t)command, 0);
    } else if (p == "WHYNTER") {
        IrSender.sendWhynter((uint16_t)address, (uint8_t)command);
    } else if (p == "MAGIQUEST") {
        IrSender.sendMagiQuest((uint32_t)address, (uint16_t)command);
    } else {
        Serial.printf("[IR] unsupported protocol: %s\n", protocol_name.c_str());
        return false;
    }
    return true;
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
