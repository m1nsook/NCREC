#include "core/CameraPath.hpp"
#include "core/CameraRig.hpp"
#include "core/CameraTransport.hpp"
#include "core/ProjectIO.hpp"
#include "core/Session.hpp"
#include "core/EditorInputGate.hpp"
#include "core/ToggleKeyPolicy.hpp"
#include "editor/WorkspaceLayout.hpp"

#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* label) {
    if (!condition) {
        throw std::runtime_error(label);
    }
}

bool near(double actual, double expected, double epsilon = 1e-8) {
    return std::abs(actual - expected) < epsilon;
}

struct FakeInputAdapter {
    bool grant{false};
    int acquired{0};
    int released{0};

    static bool acquire(void* data) noexcept {
        auto& self = *static_cast<FakeInputAdapter*>(data);
        ++self.acquired;
        return self.grant;
    }
    static void release(void* data) noexcept {
        ++static_cast<FakeInputAdapter*>(data)->released;
    }
};

void testSafeInputGate() {
    using ncrec::input::EditorInputGate;
    FakeInputAdapter adapter{};
    {
        EditorInputGate gate;
        require(!gate.open(true, nullptr, &FakeInputAdapter::release, &adapter),
                "missing engine adapter fails closed");
        require(!gate.open(false, &FakeInputAdapter::acquire,
                           &FakeInputAdapter::release, &adapter),
                "focus required for acquisition");
        require(adapter.acquired == 0, "unfocused does not touch the engine");
        require(!gate.open(true, &FakeInputAdapter::acquire,
                           &FakeInputAdapter::release, &adapter),
                "failed acquisition stays passive");
        require(!gate.active() && adapter.released == 0, "no release on failed acquire");
        adapter.grant = true;
        require(gate.open(true, &FakeInputAdapter::acquire,
                          &FakeInputAdapter::release, &adapter),
                "successful engine grant");
        gate.focusChanged(false);
        require(!gate.active() && adapter.released == 1,
                "focus loss releases cursor/input");
        require(gate.open(true, &FakeInputAdapter::acquire,
                          &FakeInputAdapter::release, &adapter),
                "explicit reopen allowed");
        gate.close();
        gate.close();
        require(adapter.released == 2, "hide release idempotent");
        require(gate.open(true, &FakeInputAdapter::acquire,
                          &FakeInputAdapter::release, &adapter),
                "reopen after hide");
    }
    require(adapter.released == 3, "shutdown releases cursor/input");
}

void testToggleKeyPolicy() {
    using namespace ncrec::input;
    require(allowedToggleKey(kDefaultToggleKey), "F8 allowed");
    require(toggleKeyName(kDefaultToggleKey) == "F8", "F8 key label");
    for (const auto& key : kToggleKeys) {
        require(allowedToggleKey(key.vk) && !toggleKeyName(key.vk).empty(),
                "allowed key has a label");
    }
    for (auto key : {0x73u, 0x79u, 0x1Bu, 0x0Du, 0x12u,
                     0x11u, 0x10u, 0x5Bu, 0x41u}) {
        require(!allowedToggleKey(key),
                "F4/F10/Escape/Enter/modifiers/gameplay keys rejected");
    }
}

void testEmptyPath() {
    ncrec::CameraPath path;
    require(!path.evaluate(0.0).has_value(), "empty path");
    require(near(path.durationSeconds(), 0.0), "empty duration");
}

void testPathTimeAndInterpolation() {
    ncrec::CameraPath path;
    require(path.addKeyframe({10.0, {{10, 0, 0}, {}, 40}}), "add later frame");
    require(path.addKeyframe({0.0, {{0, 0, 0}, {}, 80}}), "add earlier frame");
    require(!path.addKeyframe({0.0, {{1, 2, 3}, {}, 60}}), "reject duplicate time");
    require(!path.addKeyframe({-1.0, {}}), "reject negative time");
    require(!path.addKeyframe({std::numeric_limits<double>::quiet_NaN(), {}}), "reject NaN");
    require(!path.addKeyframe({4.0, {{0, 0, 0}, {0, 0, 0, 0}, 60}}), "reject zero rotation");
    require(path.keyframes().front().timeSeconds == 0.0, "sorted keyframes");

    path.setPositionInterpolation(ncrec::PositionInterpolation::Linear);
    const auto middle = path.evaluate(5.0);
    require(middle.has_value() && near(middle->position.x, 5.0), "linear midpoint");
    require(near(middle->fovDegrees, 60.0), "interpolated fov");
    require(near(path.evaluate(-1.0)->position.x, 0.0), "clamped start");
    require(near(path.evaluate(100.0)->position.x, 10.0), "clamped end");
    require(!path.evaluate(std::numeric_limits<double>::infinity()), "invalid sample time");
}

void testHermiteAndQuaternion() {
    ncrec::CameraPath path;
    require(path.addKeyframe({0, {{0, 0, 0}, {0, 0, 0, 1}, 60}}), "hermite first");
    require(path.addKeyframe({1, {{1, 0, 0}, {0, 0, 1, 0}, 60}}), "hermite second");
    const auto half = path.evaluate(0.5);
    require(half.has_value() && near(half->position.x, 0.5), "two-point cubic is linear");
    require(near(std::abs(half->rotation.z), std::sqrt(0.5)), "quaternion slerp");
    require(near(std::abs(half->rotation.w), std::sqrt(0.5)), "normalized rotation");
}

void testCameraRig() {
    ncrec::CameraRig rig;
    require(!rig.setPose({{}, {0,0,0,0}, 60}), "reject zero quaternion");
    require(!rig.setPose({{}, {}, 190}), "reject FOV > 180");
    require(rig.step({0, 1, 0, 0, 0, 0}, 1.0, 5.0), "move forward");
    require(near(rig.pose().position.y, 5.0), "local Y means forward");
    require(rig.step({1, 1, 0, 0, 0, 0}, 1.0, 5.0), "diagonal move");
    require(near(rig.pose().position.x, 5 / std::sqrt(2.0)), "diagonal normalized");
    require(rig.step({0, 0, 0, 90, 0, 0}, 1.0, 0), "yaw world Z");
    require(rig.step({0, 1, 0, 0, 0, 0}, 1.0, 1), "forward in yaw frame");
    require(near(rig.pose().position.x, 5 / std::sqrt(2.0) - 1), "yaw 90 rotates forward to -X");
    const auto previous = rig.pose();
    require(!rig.step({0, 1, 0, 0, 0, 0}, 2.0, 1.0), "reject huge dt");
    require(near(rig.pose().position.x, previous.position.x), "invalid motion leaves pose unchanged");
}

class FakeGameCamera : public ncrec::IGameCameraAdapter {
public:
    bool supported{true};
    bool readable{true};
    bool enable{true};
    bool writable{true};
    int beginCount{};
    int writeCount{};
    int restoreCount{};
    bool isSupported() const noexcept override { return supported; }
    bool read(ncrec::CameraState& pose) noexcept override {
        if (readable) pose = {};
        return readable;
    }
    bool beginOverride() noexcept override { ++beginCount; return enable; }
    bool apply(const ncrec::CameraState&) noexcept override { ++writeCount; return writable; }
    void endOverride() noexcept override { ++restoreCount; }
};

void testCameraTransport() {
    ncrec::UnavailableGameCamera unavailable;
    ncrec::CameraTransport disabled(unavailable);
    require(!disabled.start(), "unsupported camera refuses start");
    require(!disabled.apply({}), "unsupported camera refuses write");
    disabled.stop();

    FakeGameCamera fake;
    ncrec::CameraTransport transport(fake);
    fake.supported = false;
    require(!transport.start() && fake.beginCount == 0, "unsupported never begins");
    fake.supported = true;
    fake.readable = false;
    require(!transport.start() && fake.beginCount == 0, "read check gates override");
    fake.readable = true;
    require(transport.start() && fake.beginCount == 1, "begin valid override");
    require(!transport.apply({{}, {}, 181}), "invalid output never written");
    require(fake.writeCount == 0 && transport.active(), "invalid pose does not release ownership");
    fake.writable = false;
    require(!transport.apply({}) && !transport.active(), "write failure restores");
    require(fake.restoreCount == 1, "restore on write failure");
    require(transport.start(), "reacquire");
    transport.stop();
    transport.stop();
    require(fake.restoreCount == 2, "single restore per acquisition");
}

void testCameraRecording() {
    ncrec::Session session;
    require(session.execute("camera pos 2 3 4") == "Virtual camera position updated.", "camera pos");
    require(session.execute("camera fov 45") == "Virtual camera FOV updated.", "camera fov");
    require(session.execute("camera rotate 90 0 0") == "Virtual camera orientation updated.", "camera rotate");
    require(session.execute("path add 0") == "Virtual camera pose captured as keyframe.", "add at zero");
    require(session.execute("path add 0").find("duplicate") != std::string::npos, "reject duplicate");
    require(session.execute("camera pos 8 7 6") == "Virtual camera position updated.", "second pose");
    require(session.execute("path add 5") == "Virtual camera pose captured as keyframe.", "add at five");
    require(session.execute("path list").find("5.000") != std::string::npos, "list real keyframe");
    session.seek(2.5);
    require(near(session.rig().pose().position.x, 5.0), "scrub updates virtual camera pose");
    require(session.execute("path remove 0") == "Keyframe removed.", "delete indexed keyframe");
    require(session.path().size() == 1, "one keyframe remains");
    require(session.execute("path remove 99").find("not found") != std::string::npos, "reject invalid index");
    require(!session.addKeyframe(-1), "negative timestamp guarded");
    require(session.execute("camera pos nan 1 2").find("Usage") != std::string::npos, "nonfinite coordinate guarded");
}


void testProjectFiles() {
    namespace fs = std::filesystem;
    const auto unique = std::to_string(std::chrono::steady_clock::now()
                                          .time_since_epoch().count());
    const fs::path directory = fs::temp_directory_path() / ("ncrec_test_" + unique);
    fs::create_directories(directory);
    const fs::path file = directory / "shot_1.ncrec.json";

    ncrec::CameraPath path;
    path.setPositionInterpolation(ncrec::PositionInterpolation::Linear);
    require(path.addKeyframe({0.0, {{1, 2, 3}, {}, 48}}), "project frame 0");
    require(path.addKeyframe({3.0, {{5, 7, 9}, {0, 0, 1, 1}, 67}}), "project frame 1");
    std::string error;
    require(ncrec::ProjectIO::save(path, file, error), "save JSON project");
    auto loaded = ncrec::ProjectIO::load(file, error);
    require(loaded.has_value() && loaded->size() == 2, "load JSON project");
    require(loaded->positionInterpolation() == ncrec::PositionInterpolation::Linear,
        "interpolation round trip");
    require(near(loaded->evaluate(1.5)->position.x, 3.0), "project coordinates round trip");

    require(ncrec::ProjectIO::save(path, file, error), "replace existing project");
    {
        std::ofstream damaged(file, std::ios::trunc);
        damaged << "{ this is not JSON }";
    }
    require(!ncrec::ProjectIO::load(file, error), "malformed file rejected");
    require(!error.empty(), "malformed file explains failure");

    ncrec::Session session;
    session.setProjectDirectory(directory);
    require(session.execute("camera pos 9 8 7") == "Virtual camera position updated.",
        "prepare session pose");
    require(session.execute("path add 0") == "Virtual camera pose captured as keyframe.",
        "add pose for save");
    require(session.execute("path save shot_2").find("Saved camera") != std::string::npos,
        "console save");
    session.execute("path clear");
    require(session.execute("path load shot_2").find("Loaded camera") != std::string::npos,
        "console load");
    require(session.path().size() == 1 && near(session.rig().pose().position.x, 9.0),
        "console load restores pose");
    require(session.execute("path save ../evil").find("Project names") != std::string::npos,
        "path traversal rejected");
    require(session.execute("path load unknown").find("not found") != std::string::npos,
        "unknown shot rejected");
    fs::remove_all(directory);
}


bool rectContained(const ncrec::editor::Rect& r, float w, float h) {
    constexpr float epsilon = 0.01f;
    return r.x >= -epsilon && r.y >= -epsilon
        && r.width >= -epsilon && r.height >= -epsilon
        && r.x + r.width <= w + epsilon
        && r.y + r.height <= h + epsilon;
}

void testFullscreenWorkspace() {
    using ncrec::editor::calculateWorkspace;
    const auto desktop = calculateWorkspace(1744, 864);
    require(desktop.showProjectPanel && desktop.showInspector, "desktop sidebars");
    require(near(desktop.menuBar.height, 36), "top bar matches HTML");
    require(near(desktop.statusBar.height, 26), "status bar matches HTML");
    require(near(desktop.timeline.height, 240), "timeline matches HTML");
    require(near(desktop.projectPanel.width, 260), "project pane matches HTML");
    require(near(desktop.inspector.width, 320), "inspector matches HTML");
    require(near(desktop.playback.height, 44), "transport matches HTML");
    require(near(desktop.preview.width, 1164), "center fills available width");
    require(near(desktop.preview.height, 518), "center fills available height");
    require(near(desktop.playback.y, desktop.preview.y + desktop.preview.height),
        "playback directly below preview");
    require(near(desktop.timeline.y, desktop.playback.y + desktop.playback.height),
        "timeline directly below main workspace");

    const auto compact = calculateWorkspace(800, 600);
    require(!compact.showProjectPanel && compact.showInspector,
        "collapse project sidebar on compact screen");
    require(near(compact.preview.width, 480), "compact preview remains wide");
    const auto narrow = calculateWorkspace(640, 480);
    require(!narrow.showProjectPanel && !narrow.showInspector,
        "both sidebars collapse for small screens");
    require(near(narrow.preview.width, 640), "small preview is fullscreen");
    const auto scaled = calculateWorkspace(1920, 1080, 1.5f);
    require(near(scaled.menuBar.height, 54), "DPI scale applies to top bar");
    require(near(scaled.projectPanel.width, 390), "DPI scale applies to sidebar");
    const auto tiny = calculateWorkspace(320, 180);
    for (const auto& rect : {tiny.menuBar, tiny.projectPanel, tiny.preview,
                             tiny.playback, tiny.inspector, tiny.timeline,
                             tiny.statusBar}) {
        require(rectContained(rect, 320, 180), "tiny layout stays within display");
    }
    const auto bad = calculateWorkspace(-1, 300);
    require(near(bad.screen.width, 0) && near(bad.timeline.height, 0),
        "invalid framebuffer returns empty layout");
}

void testSession() {
    ncrec::Session session;
    require(session.execute("path play").find("needs") != std::string::npos, "empty play guarded");
    require(session.execute("path demo").find("Demo") != std::string::npos, "demo loaded");
    require(session.path().size() == 3, "demo point count");
    require(session.execute("path seek 2.5") == "Timeline cursor updated.", "seek command");
    require(near(session.cursorSeconds(), 2.5), "cursor changed");
    require(session.execute("path seek nan").find("Expected") != std::string::npos, "invalid seek");
    require(session.execute("path play").find("Playing") != std::string::npos, "start preview");
    session.advance(100.0);
    require(!session.playing(), "playback completes");
    require(near(session.cursorSeconds(), 5.0), "playback clamps to end");
    session.execute("path clear");
    require(session.path().size() == 0, "clear path");
}

} // namespace

int main() {
    try {
        testSafeInputGate();
        testToggleKeyPolicy();
        testEmptyPath();
        testPathTimeAndInterpolation();
        testHermiteAndQuaternion();
        testCameraRig();
        testCameraTransport();
        testCameraRecording();
        testProjectFiles();
        testSession();
        testFullscreenWorkspace();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "NCREC core tests passed\n";
    return 0;
}
