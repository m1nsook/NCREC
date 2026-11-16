#include "core/CameraPath.hpp"
#include "core/Session.hpp"

#include <cmath>
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
        testEmptyPath();
        testPathTimeAndInterpolation();
        testHermiteAndQuaternion();
        testSession();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "NCREC core tests passed\n";
    return 0;
}
