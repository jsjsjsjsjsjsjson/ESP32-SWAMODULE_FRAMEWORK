#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "src_config.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <cstring>
#include <functional>

typedef enum {
    PORT_NONE,
    PORT_AIN,
    PORT_AOUT,
    PORT_DIN,
    PORT_DOUT,
    PORT_AIN_FLOAT,
    PORT_AOUT_FLOAT
} port_type;

typedef struct {
    char name[32] = "NAME";
    char profile[64] = "PROFILE";
    port_type type = PORT_NONE;
    int16_t *data;
} port_t;

typedef enum {
    PARAM_NONE,
    PARAM_INT,
    PARAM_FLOAT,
    PARAM_ARY
} param_type;

typedef struct {
    char name[32] = "NAME";
    char profile[64] = "PROFILE";
    param_type type = PARAM_NONE;
    void *data;
} param_t;

typedef struct {
    char name[32] = "NAME";
    char author[16] = "AUTHOR";
    char profile[128] = "PROFILE";
    bool customSetting = false;
    bool customView = false;
} module_info_t;

class ParamManager {
public:
    param_t params[MAX_PARAM];
    int paramCount = 0;

    bool registerParam(void* data, param_type type, const char* name, const char* profile);
    param_t* getParam(const char* name);
    void printParams();
    int getParamCount();
};

class PortManager {
public:
    port_t inputPorts[MAX_PORT];
    port_t outputPorts[MAX_PORT];
    int inputPortCount = 0;
    int outputPortCount = 0;

    bool registerPort(int16_t* data, port_type type, const char* name, const char* profile);
    port_t* getPort(const char* name, bool isInput);
    void printPorts();
    int getInputPortCount();
    int getOutputPortCount();
};

class Module_t {
public:
    ParamManager paramManager;
    PortManager portManager;

    bool registerParam(void* data, param_type type, const char* name, const char* profile) {
        return paramManager.registerParam(data, type, name, profile);
    }

    bool registerPort(int16_t* data, port_type type, const char* name, const char* profile) {
        return portManager.registerPort(data, type, name, profile);
    }

    module_info_t module_info;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void process() = 0;
    virtual void customSettingPage() = 0;
    virtual void customViewPage() = 0;
    virtual ~Module_t() {};
};

class ModuleManager {
public:
    using ModulePtr = std::unique_ptr<Module_t>;
    using CreateModuleFunc = std::function<ModulePtr()>;

    std::unordered_map<std::string, module_info_t> moduleInfoTable;
    std::unordered_map<std::string, CreateModuleFunc> moduleCreators;
    std::unordered_map<Module_t*, ModulePtr> activeModules;

    // 用于记录每个模块类型的活动实例数量
    std::unordered_map<std::string, int> moduleInstanceCount;

    int activeModuleCount = 0;

    template<typename T>
    void registerModule() {
        auto mod = std::make_unique<T>();
        module_info_t info = mod->module_info;

        moduleInfoTable[info.name] = info;

        moduleCreators[info.name] = []() {
            return std::make_unique<T>();
        };

        // 初始化该模块类型的计数为0
        moduleInstanceCount[info.name] = 0;

        printf("Module %s registered.\n", info.name);
    }

    // 创建模块实例
    Module_t* createModule(const char* name);

    // 释放模块实例
    void releaseModule(Module_t* modulePtr);

    // 打印所有已注册的模块信息和实例数量
    void printAllRegisteredModules();

    // 打印特定模块的信息
    // void printModuleInfo(const char* name);

    ~ModuleManager();
};

#endif // MODULE_MANAGER_H