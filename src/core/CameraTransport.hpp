#pragma once

#include "core/CameraPath.hpp"

namespace ncrec {

// An adapter has sole ownership of version-specific REDengine interaction.
// beginOverride must snapshot external game state; endOverride restores it.
// The transport calls no write method before all safety gates pass.
class IGameCameraAdapter {
public:
    virtual ~IGameCameraAdapter() = default;
    [[nodiscard]] virtual bool isSupported() const noexcept = 0;
    [[nodiscard]] virtual bool read(CameraState& out) noexcept = 0;
    [[nodiscard]] virtual bool beginOverride() noexcept = 0;
    [[nodiscard]] virtual bool apply(const CameraState& pose) noexcept = 0;
    virtual void endOverride() noexcept = 0;
};

class CameraTransport {
public:
    explicit CameraTransport(IGameCameraAdapter& adapter) noexcept : adapter_(adapter) {}
    ~CameraTransport() { stop(); }
    CameraTransport(const CameraTransport&) = delete;
    CameraTransport& operator=(const CameraTransport&) = delete;

    [[nodiscard]] bool start() noexcept;
    [[nodiscard]] bool apply(const CameraState& pose) noexcept;
    void stop() noexcept;
    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    IGameCameraAdapter& adapter_;
    bool active_{};
};

// Deliberate fail-closed stub while actual Cyberpunk camera writes are
// being researched. This never reads or modifies the game.
class UnavailableGameCamera final : public IGameCameraAdapter {
public:
    [[nodiscard]] bool isSupported() const noexcept override { return false; }
    [[nodiscard]] bool read(CameraState&) noexcept override { return false; }
    [[nodiscard]] bool beginOverride() noexcept override { return false; }
    [[nodiscard]] bool apply(const CameraState&) noexcept override { return false; }
    void endOverride() noexcept override {}
};

} // namespace ncrec
