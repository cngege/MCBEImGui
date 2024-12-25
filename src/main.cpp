#include <iostream>
#include <Windows.h>
#include "imgui.h"

#include <stdlib.h>
#include <vector>
#include <shared_mutex>

#include "HookImgui.h"
#include "HookManager/HookManager.hpp"
#include "imgui/imgui_uwp_wndProc.h"
#include "imgui/appConsole.h"
#include "JNMYT.h"
#include "utils/signcode.h"

// 原理： 此库方法一个接口，其他DLL通过此接口注册一个函数
// 此库会调用此函数进行渲染


// 整个列表容器
static std::vector<void*> renderCallList;
// 读写锁 
static std::shared_mutex rw_mtx_renderList;


bool ShowConsole = true;
auto MouseUpdate(__int64 a1, char mousebutton, char isDown, __int16 mouseX, __int16 mouseY, __int16 relativeMovementX, __int16 relativeMovementY, char a8) -> void;
static HookInstance* MouseHookInstance;

static auto start(HMODULE handle)->void {
    IMGUI_CHECKVERSION();
    ImguiHooks::InitImgui();


    SignCode sign("MouseUpdate");
    sign << "48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 44 0F";
    if(sign) {
        MouseHookInstance = HookManager::getInstance()->addHook(*sign, MouseUpdate, "MouseUpdate");
        MouseHookInstance->hook();
    }
    else {
        GetImguiConsole()->AddLog("[Error] %s", "鼠标事件Hook失败");
    }
}

static auto exit(HMODULE handle) -> void {
    HookManager::getInstance()->disableAllHook();
    unregisterCoreWindowEventHandle();
    ImguiHooks::CloseImGui();
}

void ImGuiRenderLoad() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.Fonts->AddFontFromMemoryTTF((void*)JNMYT_IMGUI_data, JNMYT_IMGUI_size, 14.5f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    if(_access("C:\\Windows\\Fonts\\msyh.ttc", 0 /*F_OK*/) != -1) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    }
}

void ImGuiRender() {
    
    if(ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Insert)) {
        ShowConsole = !ShowConsole;
    }
    
    if(ShowConsole) GetImguiConsole()->Draw("Logger Console:", &ShowConsole);

    // 调用回调
    std::shared_lock<std::shared_mutex> lock(rw_mtx_renderList);
    try {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        for(auto& call : renderCallList) {
            if(call == NULL) return;
            ((void(__stdcall*)(ImGuiIO*, ImGuiContext*))call)(&io, ctx);
        }
    }
    catch(std::runtime_error& e) {
        GetImguiConsole()->AddLog("[Error] %s" ,std::string(e.what()).c_str());
    }
}

static auto MouseUpdate(__int64 a1, char mousebutton, char isDown, __int16 mouseX, __int16 mouseY, __int16 relativeMovementX, __int16 relativeMovementY, char a8)->void {
    static bool onecall = false;
    if(!onecall) {
        onecall = true;
        registerCoreWindowEventHandle();
    }

    try {
        if(ImGui::GetCurrentContext() != nullptr) {
            ImGuiMouseSource mouse_source = GetMouseSourceFromMessageExtraInfo();
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseSourceEvent(mouse_source);
            switch(mousebutton) {
            case 1:
            case 2:
            case 3:
                io.AddMouseButtonEvent(mousebutton - 1, isDown);
                break;
            case 4:
                io.AddMouseWheelEvent(0.f, isDown < 0 ? -1.f : 1.f);
                break;
            default:
                io.AddMousePosEvent(mouseX, mouseY);
                break;
            }
            if(/*io.WantCaptureMouse && */io.WantCaptureMouseUnlessPopupClose)
                return;
        }
    }
    catch(const std::exception& ex) {
        GetImguiConsole()->AddLog("[Error] %s", std::string(ex.what()).c_str());
    }
    MouseHookInstance->oriForSign(MouseUpdate)(a1, mousebutton, isDown, mouseX, mouseY, relativeMovementX, relativeMovementY, a8);
}


extern "C" __declspec(dllexport) void __stdcall ImGuiRender(void* call) {
    std::unique_lock<std::shared_mutex> lock(rw_mtx_renderList);
    // 在容器中找，不存在则加
    auto it = std::find(renderCallList.begin(), renderCallList.end(), call);
    if(it == renderCallList.end()) {
        renderCallList.push_back(call);
    }
}

extern "C" __declspec(dllexport) void __stdcall ImGuiUnRender(void* call) {
    std::unique_lock<std::shared_mutex> lock(rw_mtx_renderList);
    // 在容器中找，存在则删
    auto it = std::find(renderCallList.begin(), renderCallList.end(), call);
    if(it != renderCallList.end()) {
        renderCallList.erase(it);
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hModule);//应用程序及其DLL的线程创建与销毁不再对此DLL进行通知
    switch(ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)start, hModule, NULL, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        exit(hModule);
        break;
    }
    return TRUE;
}