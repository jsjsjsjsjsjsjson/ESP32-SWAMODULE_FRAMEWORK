#ifndef CONNECT_MANAGER_H
#define CONNECT_MANAGER_H

#include <stdint.h>
#include <vector>
#include "src_config.h"
#include "module_manager.hpp"

typedef struct {
    int8_t modules = -1;
    int8_t port = -1;
} output_target_t;

class ConnectionManager {
public:
    ModuleManager module_manager;
    std::vector<Module_t*> modules;
    std::vector<std::array<output_target_t, MAX_PORT>> connect_status;

    size_t getSlotSize() {
        return modules.size();
    }

    int getInputPortCount(int slot) {
        return modules[slot]->portManager.getInputPortCount();
    }

    int getOutputPortCount(int slot) {
        return modules[slot]->portManager.getOutputPortCount();
    }

    port_t& getInputPort(int slot, int portIndex) {
        return modules[slot]->portManager.inputPorts[portIndex];
    }

    port_t& getOutputPort(int slot, int portIndex) {
        return modules[slot]->portManager.outputPorts[portIndex];
    }

    void createModule(const char* name) {
        modules.push_back(module_manager.createModule(name));
        connect_status.push_back({});
    }

    int releaseModule(int slot) {
        if (slot >= modules.size()) {printf("Slot Error\n");return -1;}
        module_manager.releaseModule(modules[slot]);
        // modules[slot] = nullptr;
        modules.erase(modules.begin() + slot);
        connect_status.erase(connect_status.begin() + slot);
        return 0;
    }

    void process_all() {
        for (size_t i = 0; i < modules.size(); i++) {
            Module_t* module = modules[i];
            if (module) {
                for (uint8_t p = 0; p < getOutputPortCount(i); p++) {
                    // printf("MODULE: %d, OUTPUT PORT: %d, INUM=%d ONUM=%d\n", i, p, connect_status[i][p].modules, connect_status[i][p].port);
                    if (connect_status[i][p].modules < 0 || connect_status[i][p].port < 0) continue;
                    // printf("MANAGER: PROCESS_F-> I=%d O=%d\n", *getInputPort(connect_status[i][p].modules, connect_status[i][p].port).data, *getOutputPort(i, p).data);
                    *getInputPort(connect_status[i][p].modules, connect_status[i][p].port).data = *getOutputPort(i, p).data;
                    // printf("MANAGER: PROCESS_B-> I=%d O=%d\n", *getInputPort(connect_status[i][p].modules, connect_status[i][p].port).data, *getOutputPort(i, p).data);
                }
                module->process();
            } else {
                printf("WARNING: MODULE #%d IS NULL!!\n", i);
            }
        }
    }

    int connect(int8_t sourceSlot, int8_t outputPort, int8_t targetSlot, int8_t inputPort) {
        connect_status[sourceSlot][outputPort] = {targetSlot, inputPort};
        // printf("connect[%d][%d] = {%d, %d};\n",sourceSlot ,outputPort ,connect_status[sourceSlot][outputPort].modules, connect_status[sourceSlot][outputPort].port);
        printf("Successfully connected output #%d of module #%d to input #%d of module #%d\n", outputPort, sourceSlot, inputPort, targetSlot);
        return 0;
    }

    void printModuleInfo() {
        for (uint8_t m = 0; m < getSlotSize(); m++) {
            module_info_t info = modules[m]->module_info;
            printf("Module #%d(ID: %X) Information:\n", m, modules[m]);
            printf("Name: %s\n", info.name);
            printf("Author: %s\n", info.author);
            printf("Profile: %s\n", info.profile);
            printf(" Input Port:\n");
            for (uint8_t p = 0; p < getInputPortCount(m); p++) {
                port_t& info = getInputPort(m, p);
                printf("  Input #%d\n", p);
                printf("   Name: %s\n", info.name);
                printf("   Profile: %s\n", info.profile);
                printf("   data: %d\n", *info.data);
            }
            printf(" Output Port:\n");
            for (uint8_t p = 0; p < getOutputPortCount(m); p++) {
                port_t& info = getOutputPort(m, p);
                printf("  Output #%d\n", p);
                printf("   Name: %s\n", info.name);
                printf("   Profile: %s\n", info.profile);
                printf("   data: %d\n", *info.data);
            }
            printf("\n");
        }
    }
};

#endif