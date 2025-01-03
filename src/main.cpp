﻿#include <iostream>
#include <Windows.h>
#include "imgui.h"

#include <psapi.h>
#include <vector>
#include <shared_mutex>

#include "HookImgui.h"
#include "HookManager/HookManager.hpp"
#include "imgui/imgui_uwp_wndProc.h"
#include "imgui/appConsole.h"
#include "fonts/JNMYT.h"
//#include "utils/signcode.h"

// 原理： 此库方法一个接口，其他DLL通过此接口注册一个函数
// 此库会调用此函数进行渲染

/**
 * @brief 容器中起始检查渲染的模块
 */
int FirstRenderModle = 1;

// 1. 申请 1024字节的内存
bool ModuleTag[1024] = {false};

/**
 * @brief ImGui渲染函数 函数类型
 */
using RenderCall = void(__stdcall*)(ImGuiIO*, ImGuiContext*/*, void*, int id*/);
/**
 * @brief 更新标志指针函数 函数类型
 */
using UpdateDataCall = void(__stdcall*)(void*, int id, void*);

// 3. 定义一个结构 存储找到的模块的信息
struct ModuleInfo
{
    /**
     * @brief 是否启用
     */
    bool Enable = false;
    int id = 0;
    /**
     * @brief 此模块的句柄
     */
    HMODULE hmod = nullptr;
    /**
     * @brief 渲染函数
     */
    RenderCall renderevent = nullptr;
    /**
     * @brief 更新标记地址函数
     */
    UpdateDataCall updateDataevent = nullptr;
};

// 4. 定义一个容器存储模块信息
static std::vector<ModuleInfo> allModules{};
// 5. 读写锁 
static std::shared_mutex rw_mtx_moduleList;

/**
 * @brief 是否显示控制台UI
 */
bool ShowConsole = false;
//auto MouseUpdate(__int64 a1, char mousebutton, char isDown, __int16 mouseX, __int16 mouseY, __int16 relativeMovementX, __int16 relativeMovementY, char a8) -> void;
//static HookInstance* MouseHookInstance;

void LogPrint(char* str) {
    GetImguiConsole()->AddLog("%s", str);
}

void FindModules() {
    HMODULE hMods[1024];
    HANDLE hProcess = GetCurrentProcess();
    DWORD cbNeeded;
    if(EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        std::unique_lock<std::shared_mutex> lock(rw_mtx_moduleList);
        for(int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) { // 第0位跳过，因为是主程序， 这里将他作为自己的标记
            // 找模块是否存在
            auto it = std::find_if(allModules.begin(), allModules.end(), [hMods, i](const ModuleInfo& mod) {
                return mod.hmod == hMods[i];
            });
            if(it == allModules.end()) {
                ModuleInfo mod;
                mod.hmod = hMods[i];
                mod.id = allModules.size() + 1;
                {
                    // 寻找导出call 来决定是否开启
                    auto renderptr = GetProcAddress(hMods[i], "ImGuiRender");
                    auto updateDataptr = GetProcAddress(hMods[i], "ImGuiUpdateData");
                    mod.renderevent = (RenderCall)renderptr;
                    mod.updateDataevent = (UpdateDataCall)updateDataptr;

                    mod.Enable = (bool)renderptr/* && updateidptr*/;
                    ModuleTag[mod.id] = mod.Enable;
                }
                allModules.push_back(mod);

                if(mod.Enable && mod.updateDataevent) {
                    mod.updateDataevent((void*)ModuleTag, mod.id, &LogPrint);
                }
            }
            else {
                if(it->Enable && !ModuleTag[it->id] && it->updateDataevent) {
                    it->updateDataevent((void*)ModuleTag, it->id, &LogPrint);
                }
            }
        }
    }
    CloseHandle(hProcess);
}

/**
 * @brief 此模块开始加载的时候调用
 * @param handle 
 * @return 
 */
static auto start(HMODULE handle)->void {


    IMGUI_CHECKVERSION();
    ImguiHooks::InitImgui();
    ModuleTag[0] = true;

    //SignCode sign("MouseUpdate");
    //sign << "48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 44 0F";
    //sign.AddSignCall("E8 ? ? ? ? 40 B7 01 48 85 DB 74 ? 48");
    //if(sign) {
    //    //MouseHookInstance = HookManager::getInstance()->addHook(*sign, MouseUpdate, "MouseUpdate");
    //    //MouseHookInstance->hook();
    //}
    //else {
    //    GetImguiConsole()->AddLog("[Error] %s", "鼠标事件Hook失败");
    //}
    FindModules();
    registerCoreWindowEventHandle();
}

/**
 * @brief 此模块退出的时候调用
 * @param handle 
 * @return 
 */
static auto exit(HMODULE handle) -> void {
    //memset(ModuleTag, 0x00, sizeof(ModuleTag));
    ModuleTag[0] = false;
    HookManager::getInstance()->disableAllHook();
    unregisterCoreWindowEventHandle();
    ImguiHooks::CloseImGui();
}

/**
 * @brief ImGui首次加载的时候调用
 */
void ImGuiRenderLoad() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.Fonts->AddFontFromMemoryTTF((void*)JNMYT_IMGUI_data, JNMYT_IMGUI_size, 14.5f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    if(_access("C:\\Windows\\Fonts\\msyh.ttc", 0 /*F_OK*/) != -1) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    }
}

/**
 * @brief imgui 渲染的时候调用
 */
void ImGuiRender() {
    
    if(ImGui::IsKeyChordPressed(ImGuiKey_ModCtrl | ImGuiKey::ImGuiKey_Insert))
        ShowConsole = !ShowConsole;

    if(ShowConsole) GetImguiConsole()->Draw("日志控制台", &ShowConsole);

    // 遍历其他模块进行渲染
    if(ModuleTag[0]) {
        std::shared_lock<std::shared_mutex> lock(rw_mtx_moduleList);
        if(allModules.size() > FirstRenderModle && !allModules[FirstRenderModle].Enable) {
            FirstRenderModle++;
        }
        for(size_t i = FirstRenderModle; i < allModules.size(); i++) {
            auto& mod = allModules[i];
            if(mod.Enable && mod.hmod && ModuleTag[mod.id] && mod.renderevent) {
                try {
                    ImGuiIO& io = ImGui::GetIO(); (void)io;
                    ImGuiContext* ctx = ImGui::GetCurrentContext();
                    mod.renderevent(&io, ctx);
                }
                catch(std::runtime_error& e) {
                    GetImguiConsole()->AddLog("[Error][MCImRenderer] 模块渲染调用出现异常 %s", std::string(e.what()).c_str());
                }
            }
        }
    }
}

//static auto MouseUpdate(__int64 a1, char mousebutton, char isDown, __int16 mouseX, __int16 mouseY, __int16 relativeMovementX, __int16 relativeMovementY, char a8)->void {
//    try {
//        if(ImGui::GetCurrentContext() != nullptr) {
//            ImGuiMouseSource mouse_source = GetMouseSourceFromMessageExtraInfo();
//            ImGuiIO& io = ImGui::GetIO();
//            io.AddMouseSourceEvent(mouse_source);
//            switch(mousebutton) {
//            case 1:
//            case 2:
//            case 3:
//                io.AddMouseButtonEvent(mousebutton - 1, isDown);
//                break;
//            case 4:
//                io.AddMouseWheelEvent(0.f, isDown < 0 ? -1.f : 1.f);
//                break;
//            default:
//                io.AddMousePosEvent(mouseX, mouseY);
//                break;
//            }
//            if(/*io.WantCaptureMouse && */io.WantCaptureMouseUnlessPopupClose)
//                return;
//        }
//    }
//    catch(const std::exception& ex) {
//        GetImguiConsole()->AddLog("[Error] %s", std::string(ex.what()).c_str());
//    }
//    MouseHookInstance->oriForSign(MouseUpdate)(a1, mousebutton, isDown, mouseX, mouseY, relativeMovementX, relativeMovementY, a8);
//}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch(ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)start, hModule, NULL, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        exit(hModule);
        break;
    case DLL_THREAD_ATTACH:
        FindModules();
        break;
    }
    return TRUE;
}