#pragma once

// Renderer-independent cursor/input ownership contract.
// The engine adapter must grant BOTH the cursor and gameplay-input suppression.
// Neither ClipCursor nor WndProc interception alone is a valid acquire.
namespace ncrec::input {

class EditorInputGate {
public:
    using Acquire = bool (*)(void*) noexcept;
    using Release = void (*)(void*) noexcept;

    EditorInputGate() = default;
    EditorInputGate(const EditorInputGate&) = delete;
    EditorInputGate& operator=(const EditorInputGate&) = delete;
    ~EditorInputGate() { close(); }

    bool open(bool gameFocused, Acquire acquire, Release release,
              void* context) noexcept {
        if (active_) return gameFocused;
        if (!gameFocused || !acquire || !release || !acquire(context)) return false;
        release_ = release;
        context_ = context;
        active_ = true;
        return true;
    }

    void focusChanged(bool gameFocused) noexcept {
        if (!gameFocused) close();
    }

    // Idempotent. Call on hide, shutdown, focus loss and failed initialization.
    void close() noexcept {
        if (!active_) return;
        active_ = false;
        const auto release = release_;
        void* context = context_;
        release_ = nullptr;
        context_ = nullptr;
        release(context);
    }

    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    bool active_{false};
    Release release_{nullptr};
    void* context_{nullptr};
};

} // namespace ncrec::input
