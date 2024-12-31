add_rules("mode.debug", "mode.release")

add_requires("hookmanager", {configs = {lighthook = true}})
add_requires("imgui v1.91.6", {configs = {dx11 = true, dx12 = true, win32 = true}})

add_rules("plugin.vsxmake.autoupdate")

add_imports("net.http")

target("MCImRenderer")
    set_kind("shared")
    set_languages("c++20")
    set_runtimes(is_mode("debug") and "MDd" or "MD")
    
    add_files("src/*.cpp")
    add_files("include/imgui/*.cpp")
    add_files("include/imgui_kiero/*.cpp")
    add_files("include/utils/mem/*.cpp")
    add_files("include/utils/*.cpp")
    add_defines("NOMINMAX")
    add_cxflags("/utf-8")
    add_shflags("/NODEFAULTLIB:LIBCMT",{force = true})
    --pdb
    set_symbols("debug")
    add_packages("hookmanager")
    add_packages("imgui")
    add_includedirs("include")
    -- WinrtLib
    add_syslinks("onecore")