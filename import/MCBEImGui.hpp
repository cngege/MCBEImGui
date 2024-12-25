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

public:
    using RenderCALLBACK = void(__stdcall*)(ImGuiIO* io, ImGuiContext* ctx);
    using RemoteCallBack = void(__stdcall*)(RenderCALLBACK);

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
    void on(RenderCALLBACK fun){
        if(!renderFun) {
            renderFun = fun;
        }
        if(renderCall) {
            renderCall(renderFun);
        }
    }

    /**
     * @brief 移除注册的渲染回调
     */
    void remove() {
        if(unRenderCall && renderFun) {
            unRenderCall(renderFun);
        }
    }

    /**
     * @brief 查找远程函数
     */
    void UpdateFind(){
        if(!renderCall) {
            renderCall = getRemoteRenderCallback();
            if(renderCall) {
                renderCall(renderFun);
            }
        }

        if(!unRenderCall) {
            unRenderCall = getRemoteUnRenderCallback();
        }
    }
private:
    RemoteCallBack getRemoteRenderCallback() {
        if(!renderCall) {
            auto render_module = GetModuleHandleA(DLL);
            if(render_module) {
                return (RemoteCallBack)GetProcAddress(render_module, RENDER);
            }
        }
        return NULL;
    }

    RemoteCallBack getRemoteUnRenderCallback() {
        if(!renderCall) {
            auto render_module = GetModuleHandleA(DLL);
            if(render_module) {
                return (RemoteCallBack)GetProcAddress(render_module, UNRENDER);
            }
        }
        return NULL;
    }

    ~RegisterRender() {
        remove();
    }

private:

    /**
     * @brief 用于注册到远程进行渲染的函数
     */
    RenderCALLBACK renderFun = nullptr;

    /**
     * @brief 远程函数
     */
    RemoteCallBack renderCall = nullptr;
    RemoteCallBack unRenderCall = nullptr;
};

#endif // !MCBE_IMGUI_HEADER
