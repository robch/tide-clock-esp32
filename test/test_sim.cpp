#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <ctime>
#include <string>
#include <sstream>

// Standalone C++ simulation of the tide math and coordinate mapping
namespace TideSim {
    constexpr float TAU = 6.28318530718f;
    constexpr int CX = 233;
    constexpr int CY = 233;
    constexpr float CLOCK_RADIUS = 177.0f;

    struct Point { int x; int y; };

    Point polar(float angle, float radius) {
        return {
            static_cast<int>(std::round(CX + std::sin(angle) * radius)),
            static_cast<int>(std::round(CY - std::cos(angle) * radius))
        };
    }

    float heightRadius(float height, float minH, float maxH) {
        if (maxH <= minH) return CLOCK_RADIUS * 0.5f;
        float normalized = (height - minH) / (maxH - minH);
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        return CLOCK_RADIUS * (0.90f - normalized * 0.80f);
    }

    float timeAngle(int hour12, int minute, int second) {
        float h = (hour12 % 12) + minute / 60.0f + second / 3600.0f;
        return h * TAU / 12.0f;
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   NOAA TIDE CLOCK SIMULATION (Port Townsend Station 9437954)\n";
    std::cout << "========================================================\n\n";

    float minHeight = -2.0f;
    float maxHeight = 10.0f;

    struct Sample {
        std::string timeStr;
        int hour, min;
        float height;
        std::string label;
    };

    std::vector<Sample> timeline = {
        {"06:00 AM", 6, 0, 1.2f, "Low Tide (L)"},
        {"09:00 AM", 9, 0, 5.4f, "Rising"},
        {"12:15 PM", 12, 15, 8.8f, "High Tide (H)"},
        {"03:30 PM", 3, 30, 4.1f, "Falling"},
        {"06:45 PM", 6, 45, -0.5f, "Low Tide (L)"}
    };

    std::cout << std::left << std::setw(12) << "Time"
              << std::setw(10) << "Height"
              << std::setw(14) << "State"
              << std::setw(10) << "Angle"
              << std::setw(10) << "Radius"
              << "Canvas Screen (X, Y)\n";
    std::cout << std::string(70, '-') << "\n";

    for (const auto& s : timeline) {
        float angleRad = TideSim::timeAngle(s.hour, s.min, 0);
        float angleDeg = angleRad * 180.0f / 3.14159265f;
        float r = TideSim::heightRadius(s.height, minHeight, maxHeight);
        TideSim::Point p = TideSim::polar(angleRad, r);

        std::cout << std::left << std::setw(12) << s.timeStr
                  << std::setw(10) << (std::to_string(s.height).substr(0, 4) + " ft")
                  << std::setw(14) << s.label
                  << std::setw(10) << (std::to_string(angleDeg).substr(0, 5) + " deg")
                  << std::setw(10) << (std::to_string(r).substr(0, 5) + " px")
                  << "(" << p.x << ", " << p.y << ")\n";
    }

    std::cout << "\n========================================================\n";
    std::cout << "Simulation completed successfully!\n";
    return 0;
}
