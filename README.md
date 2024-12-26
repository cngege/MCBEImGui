# MCBEImGui


### 这是什么？
> 这是一个Minecraft 基岩版UI基础扩展插件, 其负责实现 IMGUI的渲染  
  并通过查找主程序中的所有模块，检查其是否导出了一个用于渲染的函数，并对此函数进行渲染时调用

**你可以理解为这是一个可被任意模块集成的组件。如果其他模块兼容了此模块接口，便可实现UI渲染，而不必进行复杂的D3DHook，仅需将此模块一同注入到游戏中（不分先后顺序）**

### 如果我的模块集成了（兼容了此组件），那是不是我的模块需要依赖它才能工作？
- 此组件与其他模块之间并非是强绑定的, 用户是否加载此组件只会影响ui是否显示。
- 仅有此区别

### 原理
- 此组件被加载后 会寻找主程序下的所有Dll, 并检查其是否导出了 `ImGuiRender` 接口
- 并保存其导出的函数, 待ImGui渲染时执行


### 如何集成
> 项目:`https://github.com/cngege/AutoSprint` 中有完整的例子可供参考

**首选导入ImGUi到项目中 注意版本需要和组件的ImGui版本一致**
- 组件的ImGui版本可以通过`xmake.lua`文件查看，目前使用版本是：`v1.91.6`

**简易版**

_不支持卸载,和日志功能_
```cpp

extern "C" __declspec(dllexport) void __stdcall ImGuiRender(ImGuiIO* io, ImGuiContext* ctx) {
    ImGui::SetCurrentContext(ctx);
    //ImGui::GetIO() = *io;

    if(ImGui::Begin("Window")) {
        ImGui::Text("Hello World")
    }
    ImGui::End();
}

```

**完整版**

```cpp

// 此为ImGui用于渲染的接口
extern "C" __declspec(dllexport) void __stdcall ImGuiRender(ImGuiIO* io, ImGuiContext* ctx) {
    ImGui::SetCurrentContext(ctx);
    //ImGui::GetIO() = *io;

    if(ImGui::Begin("Window")) {
        ImGui::Text("Hello World")
    }
    ImGui::End();
}

// 此为通知标记更改和日志函数的接口
extern "C" __declspec(dllexport) void __stdcall ImGuiUpdateData(bool tag[], int id, void* funptr) {
    Tag = tag;
    Id = id;
    tag[id] = !ModExitTag;
    imgui_print = funptr;
}

// 程序标志
// Tag由 ImGuiUpdateData 传入
// 是此组件申请的1024字节的bool数组, 用于存放各模块是否启用信息
// Tag[0] 表示此组件是否被卸载 false 被卸载
static bool* Tag;
// 当前项目模块的id号, 配合Tag拿到自己的ID位，
// 设置为True会认为模块正常工作中
// 设置为false, 会认为此模块被卸载了, 渲染时会跳过此模块
static int Id;
// 退出信号, 应当在模块 DllMain ul_reason_for_call == DLL_PROCESS_DETACH 时置为 true
// 当退出信号为true时，应当在 ImGuiUpdateData中 为tag[id]赋值false
static bool ModExitTag = false;

// 实现日志函数 注意C++的变量放置位置
void* imgui_print = nullptr;
void LogPrint(const char* fmt, ...) {
    if(Tag && Tag[0] && imgui_print) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        ((void(__fastcall*)(const char*))imgui_print)(buffer);
    }
}

```
