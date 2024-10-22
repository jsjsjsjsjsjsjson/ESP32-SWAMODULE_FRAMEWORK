#include "module_manager.hpp"

bool ParamManager::registerParam(void* data, param_type type, const char* name, const char* profile) {
    if (paramCount >= MAX_PARAM) {
        printf("Parameter array is full. Cannot register %s.\n", name);
        return false;
    }
    param_t& param = params[paramCount];
    strncpy(param.name, name, sizeof(param.name) - 1);
    strncpy(param.profile, profile, sizeof(param.profile) - 1);
    param.type = type;
    param.data = data;
    paramCount++;
    printf("Parameter %s registered.\n", name);
    return true;
}

param_t* ParamManager::getParam(const char* name) {
    for (int i = 0; i < paramCount; ++i) {
        if (strncmp(params[i].name, name, sizeof(params[i].name)) == 0) {
            return &params[i];
        }
    }
    printf("Parameter %s not found.\n", name);
    return nullptr;
}

void ParamManager::printParams() {
    for (int i = 0; i < paramCount; ++i) {
        printf("Param Name: %s, Profile: %s, Type: %d\n", params[i].name, params[i].profile, params[i].type);
    }
}

int ParamManager::getParamCount() {
    return paramCount;
}

bool PortManager::registerPort(int16_t* data, port_type type, const char* name, const char* profile) {
    if (type == PORT_AIN || type == PORT_DIN) { // Input types
        if (inputPortCount >= MAX_PORT) {
            printf("Input port array is full. Cannot register %s.\n", name);
            return false;
        }
        port_t& port = inputPorts[inputPortCount];
        strncpy(port.name, name, sizeof(port.name) - 1);
        strncpy(port.profile, profile, sizeof(port.profile) - 1);
        port.type = type;
        port.data = data;
        inputPortCount++;
        printf("Input port %s registered.\n", name);
    } else if (type == PORT_AOUT || type == PORT_DOUT) { // Output types
        if (outputPortCount >= MAX_PORT) {
            printf("Output port array is full. Cannot register %s.\n", name);
            return false;
        }
        port_t& port = outputPorts[outputPortCount];
        strncpy(port.name, name, sizeof(port.name) - 1);
        strncpy(port.profile, profile, sizeof(port.profile) - 1);
        port.type = type;
        port.data = data;
        outputPortCount++;
        printf("Output port %s registered.\n", name);
    } else {
        printf("Unknown port type for %s.\n", name);
        return false;
    }
    return true;
}

port_t* PortManager::getPort(const char* name, bool isInput) {
    if (isInput) { // Search input ports
        for (int i = 0; i < inputPortCount; ++i) {
            if (strncmp(inputPorts[i].name, name, sizeof(inputPorts[i].name)) == 0) {
                return &inputPorts[i];
            }
        }
    } else { // Search output ports
        for (int i = 0; i < outputPortCount; ++i) {
            if (strncmp(outputPorts[i].name, name, sizeof(outputPorts[i].name)) == 0) {
                return &outputPorts[i];
            }
        }
    }
    printf("Port %s not found.\n", name);
    return nullptr;
}

void PortManager::printPorts() {
    printf("Input Ports:\n");
    for (int i = 0; i < inputPortCount; ++i) {
        printf("Port Name: %s, Profile: %s, Type: %d\n", inputPorts[i].name, inputPorts[i].profile, inputPorts[i].type);
    }

    printf("Output Ports:\n");
    for (int i = 0; i < outputPortCount; ++i) {
        printf("Port Name: %s, Profile: %s, Type: %d\n", outputPorts[i].name, outputPorts[i].profile, outputPorts[i].type);
    }
}

int PortManager::getInputPortCount() {
    return inputPortCount;
}

int PortManager::getOutputPortCount() {
    return outputPortCount;
}

ModuleManager::~ModuleManager() {
    activeModules.clear();  // 清空活动模块
}

Module_t* ModuleManager::createModule(const char* name) {
    if (moduleCreators.count(name) == 0) {
        printf("Module %s not found.\n", name);
        return nullptr;
    }

    if (activeModules.count(name) > 0) {
        printf("Module %s is already created.\n", name);
        return activeModules[name].get();
    }

    activeModules[name] = moduleCreators[name]();
    activeModules[name]->start();
    activeModuleCount++;
    return activeModules[name].get();
}

void ModuleManager::releaseModule(const char* name) {
    if (activeModules.count(name) > 0) {
        activeModules[name]->stop();  // 停止模块
        activeModules.erase(name);    // 释放实例
        activeModuleCount--;
        printf("Module %s released.\n", name);
    } else {
        printf("Module %s is not active.\n", name);
    }
}

void ModuleManager::printAllRegisteredModules() {
    printf("Registered Modules:\n");
    for (const auto& pair : moduleInfoTable) {
        const module_info_t& info = pair.second;
        printf("Name: %s, Author: %s, Profile: %s, ", info.name, info.author, info.profile);
        if (activeModules.count(info.name) > 0) {
            printf("Status: Active\n");
        } else {
            printf("Status: Not Active\n");
        }
    }
}

void ModuleManager::printModuleInfo(const char* name) {
    if (moduleInfoTable.count(name) > 0) {
        const module_info_t& info = moduleInfoTable[name];
        printf("Module Information:\n");
        printf("Name: %s\n", info.name);
        printf("Author: %s\n", info.author);
        printf("Profile: %s\n", info.profile);
        if (activeModules.count(name) > 0) {
            printf("Status: Active\n");
        } else {
            printf("Status: Not Active\n");
        }
    } else {
        printf("Module %s not found in the information table.\n", name);
    }
}