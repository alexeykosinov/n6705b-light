#include <string>
#include <iostream>
#include <windows.h>
#include <tclap/CmdLine.h>
#include <nlohmann/json.hpp>
#include "visa.h"
#include <fstream>

using json = nlohmann::json;

char buf[256] = { 0 };

void SetVoltage(const ViSession& inst_channel, const std::string& newVoltage, const int& channel) {
    std::string sum = "SOURce:VOLTage " + newVoltage + ", (@" + std::to_string(channel) + ")\n";
    const char* instrunction = sum.c_str();
    viPrintf(inst_channel, instrunction);
}

void SetCurrent(const ViSession& inst_channel, const std::string& newVoltage, const int& channel) {
    std::string sum = "SOURce:CURRent " + newVoltage + ", (@" + std::to_string(channel) + ")\n";
    const char* instrunction = sum.c_str();
    viPrintf(inst_channel, instrunction);
}

char* ObtainVoltage(const ViSession& inst_channel, const int& channel) {
    viPrintf(inst_channel, "MEASure:SCALar:VOLTage? (@%d)\n", channel);
    viScanf(inst_channel, "%t", buf);
    buf[std::strlen(buf) - 1] = '\0';
    return buf;
}

char* ObtainCurrent(const ViSession& inst_channel, const int& channel) {
    viPrintf(inst_channel, "MEASure:SCALar:CURRent? (@%d)\n", channel);
    viScanf(inst_channel, "%t", buf);
    buf[std::strlen(buf) - 1] = '\0';
    return buf;
}

char* ObtainPower(const ViSession& inst_channel, const int& channel) {
    viPrintf(inst_channel, "MEASure:SCALar:POWEr? (@%d)\n", channel);
    viScanf(inst_channel, "%t", buf);
    buf[strlen(buf) - 1] = '\0';

    return buf;
}


void ViewMeas(const ViSession& inst_channel) {
        
    std::cout << std::left << std::setw(10) << std::setfill(' ') << "Channel";
    std::cout << std::left << std::setw(10) << std::setfill(' ') << "Voltage";
    std::cout << std::left << std::setw(10) << std::setfill(' ') << "Current";
    std::cout << std::left << std::setw(10) << std::setfill(' ') << "Power";
    std::cout << std::endl;

    for (int i = 1; i < 4; i++) {

        std::string conv_volt  = ObtainVoltage(inst_channel, i);
        std::string conv_curr  = ObtainCurrent(inst_channel, i);
        std::string conv_power = ObtainPower(inst_channel, i);
        std::string::size_type sz = {};

        double result_v = std::stod(conv_volt, &sz);
        double result_i = std::stod(conv_curr, &sz);
        double result_p = std::stod(conv_power, &sz);

        std::cout << std::left << std::setw(10) << std::setfill(' ') << i;
        std::cout << std::left << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(4) << (float)result_v;
        std::cout << std::left << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(4) << (float)result_i;
        std::cout << std::left << std::setw(10) << std::setfill(' ') << std::fixed << std::setprecision(2) << (float)result_p;
        std::cout << std::endl;
    }
    std::cout << std::endl;
}


int main(int argc, char* argv[]) {

    std::ifstream ifs("settings.json");

    if (!ifs.is_open()) {
        std::cout << "[Error] Can't find settings.json" << std::endl;
        std::cin.get();
        return -1;
    }

    json file = json::parse(ifs);

    ifs.close();

    std::string get_address;

    for (auto& array : file["base"]) {
        get_address = array["address"];
    }

    std::string combine_addr = "TCPIP0::" + get_address;
    const char* c = combine_addr.c_str();


    std::cout << std::endl;
    std::cout << "Keysight N6700 DC Power Analyzer command line application" << std::endl;
    std::cout << std::endl;

    ViStatus status;
    ViSession defaultRM;
    ViSession instr;

    char identifier[256] = { 0 };

    status = viOpenDefaultRM(&defaultRM);
    if (status < VI_SUCCESS) {
        std::cout << "[Error] When trying to initialize VISA...exiting" << std::endl;
        std::cin.get();
        return -1;
    }

    status = viOpen(defaultRM, c, VI_NULL, VI_NULL, &instr);
    if (status < VI_SUCCESS) {
        std::cout << "[Error] Could not open device, status = " << std::hex << status << std::endl;
        std::cin.get();
        return -1;
    }
    
    viPrintf(instr, "*IDN?\n");
    viScanf(instr, "%t", identifier);

    std::string NewVoltage, NewCurrent;

    std::cout << "Identification device: " << identifier << std::endl;
    std::cout << std::endl;

    std::cout << "Instant measurements:" << std::endl;
    ViewMeas(instr);

    try {

        TCLAP::CmdLine cmd("Powered by TCLAP", ' ', "0.9");
        TCLAP::ValueArg<unsigned short> dcpa_c("c", "channel", "Channel selection", true, 0, "1, 2, 3");
        TCLAP::ValueArg<bool> dcpa_o("o", "output", "Turn ON/OFF output", false, false, "0, 1");
        cmd.add(dcpa_o);
        TCLAP::ValueArg<float> dcpa_v("v", "voltage", "Voltage, availible range [0...50] V", false, 0, "0...50");
        cmd.add(dcpa_v);
        TCLAP::ValueArg<float> dcpa_i("i", "current", "Current limitation, availible range [0...10] A", false, 0, "0...10");
        cmd.add(dcpa_i);
        TCLAP::SwitchArg dcpa_p("p", "preset", "Load preset from settings.json", true);
        TCLAP::ValueArg<bool> dcpa_all("a", "all", "Turn ON/OFF all outputs", false, false, "0, 1");
        cmd.add(dcpa_all);

        cmd.xorAdd(dcpa_c, dcpa_p);
        cmd.parse(argc, argv);

        if (dcpa_c.isSet()) {
            if (dcpa_c.getValue() > 0 && dcpa_c.getValue() < 4) std::cout << "Channel set to " << dcpa_c.getValue() << std::endl;
            else std::cout << "[Error] Channel unknown" << std::endl;
        }

        if (dcpa_v.isSet()) {
            if (dcpa_v.getValue() > 0 && dcpa_v.getValue() < 50) {
                SetVoltage(instr, std::to_string(dcpa_v.getValue()), dcpa_c.getValue());
                std::cout << "Voltage set to " << dcpa_v.getValue() << std::endl;
            }
            else { 
                std::cout << "[Error] Value is out of range" << std::endl; 
            }
        }

        if (dcpa_i.isSet()) {
            if (dcpa_i.getValue() > 0 && dcpa_i.getValue() < 20) {
                SetCurrent(instr, std::to_string(dcpa_i.getValue()), dcpa_c.getValue());
                std::cout << "Current set to " << dcpa_i.getValue() << std::endl;
            }
            else {
                std::cout << "[Error] Value is out of range" << std::endl;
            }
        }       

        if (dcpa_o.isSet()) {
            if (dcpa_o.getValue()) {
                std::string sum_out = "OUTPut:STATe ON, (@" + std::to_string(dcpa_c.getValue()) + ")\n";
                const char* instrunction = sum_out.c_str();
                viPrintf(instr, instrunction);
                Sleep(1000);
                std::cout << std::endl;
                std::cout << "Updated measurements:" << std::endl;
                ViewMeas(instr);
            }
            else {
                std::string sum_out = "OUTPut:STATe OFF, (@" + std::to_string(dcpa_c.getValue()) + ")\n";
                const char* instrunction = sum_out.c_str();
                viPrintf(instr, instrunction);
                Sleep(1000);
                std::cout << std::endl;
                std::cout << "Updated measurements:" << std::endl;
                ViewMeas(instr);
            }

        }


        if (dcpa_p.isSet()) {



            std::string get_channel;
            std::string get_voltage;
            std::string get_current;

            std::cout << std::endl;
            std::cout << "New settings from preset loaded" << std::endl;
            std::cout << std::endl;

            for (auto& array : file["preset"]) {
                get_channel = array["channel"];
                get_voltage = array["voltage"];
                get_current = array["current"];

                SetVoltage(instr, get_voltage, std::stoi(get_channel));
                SetCurrent(instr, get_current, std::stoi(get_channel));

                std::cout << "Channel: " << get_channel << std::endl;
                std::cout << "Voltage: " << get_voltage << std::endl;
                std::cout << "Current: " << get_current << std::endl;
                std::cout << std::endl;
            }
        }

        if (dcpa_all .isSet()) {

            if (dcpa_all.getValue()) {
                std::cout << "Second" << std::endl;
                viPrintf(instr, "OUTPut:STATe ON, (@1)\n");
                viPrintf(instr, "OUTPut:STATe ON, (@2)\n");
                viPrintf(instr, "OUTPut:STATe ON, (@3)\n");
                Sleep(1000);
                std::cout << std::endl;
                std::cout << "Updated measurements:" << std::endl;
                ViewMeas(instr);
            }
            else {
                viPrintf(instr, "OUTPut:STATe OFF, (@1)\n");
                viPrintf(instr, "OUTPut:STATe OFF, (@2)\n");
                viPrintf(instr, "OUTPut:STATe OFF, (@3)\n");
            }

        }



    }
    catch (TCLAP::ArgException& e) { 
        std::cerr << "[Error] " << e.error() << " for arg " << e.argId() << std::endl; 
        viClose(instr);
        viClose(defaultRM);
        std::cin.get();
    }
    

    viClose(instr);
    viClose(defaultRM);
    return 0;
}
