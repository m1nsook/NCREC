#include "platform/OverlayWindow.hpp"

#include "core/Session.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

using Microsoft::WRL::ComPtr;

// imgui_impl_win32.h deliberately does not declare this handler in the
// public header, because most apps don't need to call it directly.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window, UINT message, WPARAM wparam, LPARAM lparam);

namespace ncrec::platform {
namespace {

constexpr int kToggleEditor = 1;
constexpr wchar_t kWindowClass[] = L"NCREC.Prototype.Editor";

struct HotkeyChoice {
    const char* label;
    const wchar_t* setting;
    UINT virtualKey;
};

// Every option is a single key, with no modifiers. The initial key is
// deliberately close to the left hand. Users can choose a different one
// if CET or another mod already uses it.
constexpr std::array<HotkeyChoice, 10> kHotkeys{{
    {"Grave / tilde (left of 1)", L"Grave", VK_OEM_3},
    {"Insert", L"Insert", VK_INSERT},
    {"Home", L"Home", VK_HOME},
    {"End", L"End", VK_END},
    {"F6", L"F6", VK_F6},
    {"F7", L"F7", VK_F7},
    {"F8", L"F8", VK_F8},
    {"F9", L"F9", VK_F9},
    {"Backspace", L"Backspace", VK_BACK},
    {"Caps Lock (may conflict with scanning)", L"CapsLock", VK_CAPITAL},
}};

std::filesystem::path moduleDirectory(HMODULE module) {
    std::array<wchar_t, 32768> buffer{};
    const DWORD count = GetModuleFileNameW(module, buffer.data(),
                                          static_cast<DWORD>(buffer.size()));
    if (count == 0 || count >= buffer.size()) {
        return {};
    }
    return std::filesystem::path(std::wstring(buffer.data(), count)).parent_path();
}

class OverlayWindow {
public:
    explicit OverlayWindow(HMODULE module)
        : module_(module),
          configPath_((moduleDirectory(module) / L"NCREC.ini").wstring()),
          logfile_(moduleDirectory(module) / L"NCREC.log", std::ios::app) {}

    [[nodiscard]] bool initialize() {
        WNDCLASSEXW klass{};
        klass.cbSize = sizeof(klass);
        klass.style = CS_HREDRAW | CS_VREDRAW;
        klass.lpfnWndProc = &OverlayWindow::windowProc;
        klass.hInstance = module_;
        klass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        klass.lpszClassName = kWindowClass;

        if (!RegisterClassExW(&klass)) {
            log("ERROR: Win32 window class registration failed.");
            return false;
        }
        classRegistered_ = true;

        // A separate native window instead of injecting a renderer into the
        // game's DX12 swapchain. Works best in borderless/windowed mode.
        window_ = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            kWindowClass, L"NCREC - Cinematic Camera Editor (Prototype)",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1080, 700,
            nullptr, nullptr, module_, this);
        if (!window_) {
            log("ERROR: Win32 editor window creation failed.");
            return false;
        }

        if (!createD3D11()) {
            log("ERROR: D3D11 initialization failed.");
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        contextCreated_ = true;
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // Prototype: avoid writing a global imgui.ini.
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.Colors[ImGuiCol_Button] = ImVec4(0.42f, 0.32f, 0.03f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.70f, 0.50f, 0.02f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.42f, 0.32f, 0.03f, 0.9f);

        if (!ImGui_ImplWin32_Init(window_)) {
            log("ERROR: Dear ImGui Win32 backend initialization failed.");
            return false;
        }
        win32Ready_ = true;
        if (!ImGui_ImplDX11_Init(device_.Get(), context_.Get())) {
            log("ERROR: Dear ImGui D3D11 backend initialization failed.");
            return false;
        }
        imguiDxReady_ = true;

        wchar_t requestedKey[64]{};
        GetPrivateProfileStringW(L"Hotkeys", L"Toggle", L"Grave",
            requestedKey, static_cast<DWORD>(std::size(requestedKey)), configPath_.c_str());
        int requestedIndex = 0;
        for (int i = 0; i < static_cast<int>(kHotkeys.size()); ++i) {
            if (std::wstring_view(requestedKey) == kHotkeys[i].setting) {
                requestedIndex = i;
                break;
            }
        }

        if (!switchHotkey(requestedIndex, false)) {
            // Try Insert when a CET/other binding already owns the grave key.
            if (requestedIndex == 1 || !switchHotkey(1, false)) {
                log("WARNING: No single-key toggle could be registered. Editor opens automatically.");
                show(true);
            } else {
                log("WARNING: Preferred key unavailable; temporarily using Insert.");
            }
        }

        log("NCREC ASI loaded. Game camera adapter is NOT implemented.");
        log("Press your configured single toggle key to open/close the editor.");
        log("The console is inside the editor; no additional shortcut required.");
        log("Type 'path demo', then 'path play' to preview the timeline math.");
        return true;
    }

    void run() {
        auto previous = std::chrono::steady_clock::now();
        bool active = true;
        while (active) {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    active = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (!active) {
                break;
            }

            const auto now = std::chrono::steady_clock::now();
            session_.advance(std::chrono::duration<double>(now - previous).count());
            previous = now;

            // Keep the editor out of the way when switching away from the game.
            if (visible_ && !foregroundBelongsToThisProcess()) {
                show(false);
            }

            if (visible_ && !IsIconic(window_)) {
                render();
            } else {
                MsgWaitForMultipleObjectsEx(
                    0, nullptr, session_.playing() ? 16 : INFINITE,
                    QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
        }
    }

    void shutdown() {
        if (editorHotkeyRegistered_) {
            UnregisterHotKey(window_, kToggleEditor);
        }
        if (imguiDxReady_) {
            ImGui_ImplDX11_Shutdown();
        }
        if (win32Ready_) {
            ImGui_ImplWin32_Shutdown();
        }
        if (contextCreated_) {
            ImGui::DestroyContext();
        }
        target_.Reset();
        swapchain_.Reset();
        context_.Reset();
        device_.Reset();
        if (window_) {
            DestroyWindow(window_);
            window_ = nullptr;
        }
        if (classRegistered_) {
            UnregisterClassW(kWindowClass, module_);
        }
    }

    LRESULT handle(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == WM_HOTKEY && wparam == kToggleEditor
            && (visible_ || foregroundBelongsToThisProcess())) {
            show(!visible_);
            return 0;
        }
        if (message == WM_KEYDOWN && wparam == VK_ESCAPE && visible_) {
            show(false);
            return 0;
        }
        if (contextCreated_ && ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam)) {
            return 1;
        }
        switch (message) {
            case WM_CLOSE:
                show(false);
                return 0;
            case WM_SIZE:
                if (swapchain_ && wparam != SIZE_MINIMIZED && LOWORD(lparam) > 0
                    && HIWORD(lparam) > 0) {
                    target_.Reset();
                    const HRESULT result = swapchain_->ResizeBuffers(
                        0, LOWORD(lparam), HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0);
                    if (SUCCEEDED(result)) {
                        if (!createRenderTarget()) {
                            log("ERROR: D3D11 render target recreation failed.");
                        }
                    } else {
                        log("ERROR: D3D11 swapchain resize failed.");
                    }
                }
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(window, message, wparam, lparam);
        }
    }

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == WM_NCCREATE) {
            auto* creation = reinterpret_cast<CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(creation->lpCreateParams));
        }
        auto* self = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        return self ? self->handle(hwnd, message, wparam, lparam)
                    : DefWindowProcW(hwnd, message, wparam, lparam);
    }

    [[nodiscard]] bool foregroundBelongsToThisProcess() const {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return process == GetCurrentProcessId();
    }

    void show(bool showWindow) {
        if (!window_) {
            return;
        }
        if (showWindow) {
            if (!visible_) {
                HWND foreground = GetForegroundWindow();
                if (foreground != window_) {
                    returnFocus_ = foreground;
                }
            }
            visible_ = true;
            ShowWindow(window_, SW_SHOW);
            SetForegroundWindow(window_);
        } else {
            visible_ = false;
            ShowWindow(window_, SW_HIDE);
            if (returnFocus_ && IsWindow(returnFocus_)) {
                SetForegroundWindow(returnFocus_);
            }
        }
    }

    [[nodiscard]] bool switchHotkey(int index, bool persist) {
        if (index < 0 || index >= static_cast<int>(kHotkeys.size())) {
            return false;
        }

        const int previousIndex = selectedHotkey_;
        if (editorHotkeyRegistered_) {
            UnregisterHotKey(window_, kToggleEditor);
            editorHotkeyRegistered_ = false;
        }

        if (!RegisterHotKey(window_, kToggleEditor, MOD_NOREPEAT,
                            kHotkeys[index].virtualKey)) {
            if (previousIndex >= 0 && previousIndex != index) {
                editorHotkeyRegistered_ = RegisterHotKey(
                    window_, kToggleEditor, MOD_NOREPEAT,
                    kHotkeys[previousIndex].virtualKey) != 0;
            }
            log("WARNING: This key is already registered or unavailable.");
            return false;
        }

        editorHotkeyRegistered_ = true;
        selectedHotkey_ = index;
        if (persist && !WritePrivateProfileStringW(
                L"Hotkeys", L"Toggle", kHotkeys[index].setting, configPath_.c_str())) {
            log("WARNING: Unable to save selected key to NCREC.ini.");
        }
        return true;
    }

    void log(std::string message) {
        if (messages_.size() >= 500) {
            messages_.erase(messages_.begin());
        }
        if (logfile_) {
            logfile_ << message << '\n';
            logfile_.flush();
        }
        messages_.push_back(std::move(message));
    }

    void execute(const char* command) {
        const std::string input{command};
        if (input.empty()) {
            return;
        }
        log(std::string("> ") + input);
        log(session_.execute(input));
    }

    [[nodiscard]] bool createD3D11() {
        DXGI_SWAP_CHAIN_DESC description{};
        description.BufferCount = 2;
        description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        description.OutputWindow = window_;
        description.SampleDesc.Count = 1;
        description.Windowed = TRUE;
        description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
        D3D_FEATURE_LEVEL chosen{};
        const HRESULT result = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            levels, 1, D3D11_SDK_VERSION, &description,
            swapchain_.GetAddressOf(), device_.GetAddressOf(), &chosen,
            context_.GetAddressOf());
        return SUCCEEDED(result) && createRenderTarget();
    }

    [[nodiscard]] bool createRenderTarget() {
        ComPtr<ID3D11Texture2D> buffer;
        if (FAILED(swapchain_->GetBuffer(0, IID_PPV_ARGS(buffer.GetAddressOf())))) {
            return false;
        }
        return SUCCEEDED(device_->CreateRenderTargetView(
            buffer.Get(), nullptr, target_.GetAddressOf()));
    }

    void render() {
        if (!target_) {
            return;
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("NCREC Workspace", nullptr, flags);

        ImGui::TextColored(ImVec4(1.0f, 0.83f, 0.05f, 1.0f), "NCREC  /  v0.1.0");
        ImGui::SameLine();
        ImGui::TextDisabled("Native ASI + independent ImGui editor");
        if (ImGui::Button("Close editor")) {
            show(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Focus console")) {
            focusConsole_ = true;
        }

        int selected = std::max(selectedHotkey_, 0);
        if (ImGui::BeginCombo("Toggle key (one press)", kHotkeys[selected].label)) {
            for (int i = 0; i < static_cast<int>(kHotkeys.size()); ++i) {
                if (ImGui::Selectable(kHotkeys[i].label, i == selected)) {
                    switchHotkey(i, true);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::TextDisabled("Shortcut settings are saved in NCREC.ini; Esc closes the editor.");
        ImGui::Separator();
        ImGui::TextWrapped(
            "PROTOTYPE: the game camera is not connected. Playback only previews "
            "the interpolation engine, it does not change the Cyberpunk viewpoint.");

        if (ImGui::Button("Load demo camera path")) {
            execute("path demo");
        }
        ImGui::SameLine();
        if (ImGui::Button(session_.playing() ? "Pause" : "Play")) {
            execute(session_.playing() ? "path pause" : "path play");
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear path")) {
            execute("path clear");
        }

        float cursor = static_cast<float>(session_.cursorSeconds());
        const float duration = static_cast<float>(session_.path().durationSeconds());
        if (ImGui::SliderFloat("Timeline (s)", &cursor, 0.0f, std::max(duration, 0.01f), "%.3f")) {
            session_.seek(static_cast<double>(cursor));
        }
        ImGui::Text("Keyframes: %zu   |   Duration: %.3fs   |   %s",
                    session_.path().size(), duration,
                    session_.playing() ? "Preview playing" : "Paused");

        if (const auto sample = session_.path().evaluate(session_.cursorSeconds())) {
            ImGui::Text("Preview XYZ: (%.2f, %.2f, %.2f)   FOV: %.1f",
                        sample->position.x, sample->position.y,
                        sample->position.z, sample->fovDegrees);
        }

        ImGui::SeparatorText("NCREC Console");
        const float commandHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
        ImGui::BeginChild("Console output", ImVec2(0, -commandHeight), true);
        for (const auto& line : messages_) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (scrollToBottom_) {
            ImGui::SetScrollHereY(1.0f);
            scrollToBottom_ = false;
        }
        ImGui::EndChild();

        if (focusConsole_) {
            ImGui::SetKeyboardFocusHere();
            focusConsole_ = false;
        }
        if (ImGui::InputText("##console", command_.data(), command_.size(),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            execute(command_.data());
            command_.fill('\0');
            scrollToBottom_ = true;
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("help / path demo / path play");

        ImGui::End();
        ImGui::Render();

        constexpr float background[] = {0.045f, 0.045f, 0.050f, 1.0f};
        context_->OMSetRenderTargets(1, target_.GetAddressOf(), nullptr);
        context_->ClearRenderTargetView(target_.Get(), background);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapchain_->Present(1, 0);
    }

    HMODULE module_{};
    std::wstring configPath_;
    HWND window_{};
    HWND returnFocus_{};
    bool visible_{};
    bool focusConsole_{};
    bool scrollToBottom_{true};
    bool classRegistered_{};
    bool contextCreated_{};
    bool win32Ready_{};
    bool imguiDxReady_{};
    bool editorHotkeyRegistered_{};
    int selectedHotkey_{-1};

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain> swapchain_;
    ComPtr<ID3D11RenderTargetView> target_;

    Session session_;
    std::ofstream logfile_;
    std::vector<std::string> messages_;
    std::array<char, 512> command_{};
};

} // namespace

DWORD runOverlay(HMODULE module) {
    OverlayWindow overlay(module);
    if (!overlay.initialize()) {
        OutputDebugStringA("NCREC: native editor failed to initialize\n");
        overlay.shutdown();
        return 1;
    }
    overlay.run();
    overlay.shutdown();
    return 0;
}

} // namespace ncrec::platform
