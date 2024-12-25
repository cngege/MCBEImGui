#pragma once

#ifndef MCBE_IMGUI_HEADER
#define MCBE_IMGUI_HEADER
#include <Windows.h>

#ifdef IMGUI_INCLUDE
#include IMGUI_INCLUDE
#endif // IMGUI_INCLUDE


class RegisterRender {
    const LPCSTR DLL = "MCBEImGui.dll";
    const LPCSTR RENDER = "ImGuiRender";
    const LPCSTR UNRENDER = "ImGuiUnRender";
    const LPCSTR ADDLOG = "ImGuiAddLog";

public:
    using RenderFunBACK = void(__stdcall*)(ImGuiIO* io, ImGuiContext* ctx);
    using RemoteCallBack = void(__stdcall*)(RenderFunBACK);
    using RemoteAddLogCallBack = void(__stdcall*)(const char*);

public:
    /**
     * @brief 获取单例
     * @return 
     */
    static RegisterRender* getInstance() {
        static RegisterRender instance;
        return &instance;
    }

    /**
     * @brief 注册渲染回调
     * @param fun
     */
    void on(RenderFunBACK fun){
        if(!renderFun) {
            renderFun = fun;

            if(regRenderCall) {
                regRenderCall(renderFun);
            }
        }
    }

    /**
     * @brief 移除注册的渲染回调
     */
    void remove() {
        if(unRegRenderCall && renderFun) {
            unRegRenderCall(renderFun);
        }
    }

    /**
     * @brief 查找远程函数
     */
    void UpdateFind(){
        if(!regRenderCall) {
            regRenderCall = getRemoteRenderCallback();
            if(regRenderCall && renderFun) {
                regRenderCall(renderFun);
            }
        }

        if(!unRegRenderCall) {
            unRegRenderCall = getRemoteUnRenderCallback();
        }
        if(!addLogCall) {
            addLogCall = getRemoteAddLogCallback();
        }
    }

    /**
     * @brief 添加日志
     * @param str 
     */
    void AddLog(const char* str) {
        if(addLogCall) {
            addLogCall(str);
        }
    }

    /**
     * @brief 添加日志 格式化
     * @param fmt
     */
    void AddLogfmt(const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        AddLog(buffer);
    }
private:
    RemoteCallBack getRemoteRenderCallback() {
        auto render_module = GetModuleHandleA(DLL);
        if(render_module) {
            return (RemoteCallBack)GetProcAddress(render_module, RENDER);
        }
        return NULL;
    }

    RemoteCallBack getRemoteUnRenderCallback() {
        if(!unRegRenderCall) {
            auto render_module = GetModuleHandleA(DLL);
            if(render_module) {
                return (RemoteCallBack)GetProcAddress(render_module, UNRENDER);
            }
        }
    }

    RemoteAddLogCallBack getRemoteAddLogCallback() {
        auto render_module = GetModuleHandleA(DLL);
        if(render_module) {
            return (RemoteAddLogCallBack)GetProcAddress(render_module, ADDLOG);
        }
    }

    ~RegisterRender() {
        remove();
    }

private:

    /**
     * @brief 用于注册到远程进行渲染的函数
     */
    RenderFunBACK renderFun = nullptr;

    /**
     * @brief 远程函数
     */
    RemoteCallBack regRenderCall = nullptr;
    RemoteCallBack unRegRenderCall = nullptr;
    RemoteAddLogCallBack addLogCall = nullptr;
};

#endif // !MCBE_IMGUI_HEADER
