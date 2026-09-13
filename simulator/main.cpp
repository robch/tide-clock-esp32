#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace {
constexpr int W = 466, H = 466, CX = W / 2, CY = H / 2;
constexpr float PI = 3.14159265358979323846f;
constexpr float TAU = 2.0f * PI;
constexpr float CLOCK_RADIUS = 177.0f;

struct Point { float x, y; };
struct Color { Uint8 r, g, b, a = 255; };

Point polar(float angle, float radius) {
    return {CX + std::sin(angle) * radius, CY - std::cos(angle) * radius};
}

float tideHeight(float hours) {
    // Plausible semidiurnal tide used for layout work when hardware is absent.
    return 4.0f + 4.1f * std::sin((hours - 2.0f) * TAU / 12.42f)
                + 0.55f * std::sin((hours + 1.0f) * TAU / 24.0f);
}

float heightRadius(float height) {
    const float normalized = std::clamp((height + 2.0f) / 12.0f, 0.0f, 1.0f);
    return CLOCK_RADIUS * (0.90f - normalized * 0.80f);
}

void setColor(SDL_Renderer* r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a); }
void line(SDL_Renderer* r, Point a, Point b, Color c, int width = 1) {
    setColor(r, c);
    for (int i = -(width / 2); i <= width / 2; ++i)
        SDL_RenderDrawLineF(r, a.x + i, a.y, b.x + i, b.y);
}
void circle(SDL_Renderer* r, Point p, float radius, Color c) {
    setColor(r, c);
    constexpr int segments = 180;
    Point previous = {p.x, p.y - radius};
    for (int i = 1; i <= segments; ++i) {
        const float a = i * TAU / segments;
        Point next = {p.x + std::sin(a) * radius, p.y - std::cos(a) * radius};
        SDL_RenderDrawLineF(r, previous.x, previous.y, next.x, next.y);
        previous = next;
    }
}
void filledCircle(SDL_Renderer* r, Point p, int radius, Color c) {
    setColor(r, c);
    for (int y = -radius; y <= radius; ++y) {
        int x = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(r, static_cast<int>(p.x) - x, static_cast<int>(p.y) + y,
                           static_cast<int>(p.x) + x, static_cast<int>(p.y) + y);
    }
}
void quad(SDL_Renderer* r, const std::array<Point, 4>& p, Color c) {
    SDL_Vertex vertices[4]{};
    for (int i = 0; i < 4; ++i) {
        vertices[i].position = {p[i].x, p[i].y};
        vertices[i].color = {c.r, c.g, c.b, c.a};
    }
    const int indices[] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, nullptr, vertices, 4, indices, 6);
}

// Compact 3x5 glyphs, sufficient for the clock numerals and status text.
const char* glyph(char ch) {
    switch (ch) {
    case '0': return "111101101101111"; case '1': return "010110010010111";
    case '2': return "111001111100111"; case '3': return "111001111001111";
    case '4': return "101101111001001"; case '5': return "111100111001111";
    case '6': return "111100111101111"; case '7': return "111001001001001";
    case '8': return "111101111101111"; case '9': return "111101111001111";
    case 'A': return "010101111101101"; case 'D': return "110101101101110";
    case 'E': return "111100110100111"; case 'F': return "111100110100100";
    case 'I': return "111010010010111"; case 'L': return "100100100100111";
    case 'N': return "101111111111101"; case 'O': return "111101101101111";
    case 'T': return "111010010010010"; case 'V': return "101101101101010";
    default: return "000000000000000";
    }
}
void text(SDL_Renderer* r, int centerX, int top, const std::string& value, Color c, int scale = 2) {
    const int glyphW = 3 * scale, gap = scale;
    const int total = value.empty() ? 0 : static_cast<int>(value.size()) * (glyphW + gap) - gap;
    int x = centerX - total / 2;
    setColor(r, c);
    for (char ch : value) {
        const char* bits = glyph(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col)
                if (bits[row * 3 + col] == '1') {
                    SDL_Rect px{x + col * scale, top + row * scale, scale, scale};
                    SDL_RenderFillRect(r, &px);
                }
        x += glyphW + gap;
    }
}

void render(SDL_Renderer* r, bool dataVisible, double elapsedSeconds) {
    const Color background{8, 17, 32}, pale{220, 240, 248}, grid{38, 73, 99};
    const Color cyan{79, 214, 255}, fill{45, 170, 214, 90}, yellow{255, 209, 102};
    const Color red{255, 107, 107}, offline{210, 95, 95};
    setColor(r, background); SDL_RenderClear(r);

    circle(r, {CX, CY}, CLOCK_RADIUS, {127, 168, 189});
    for (float feet = -2; feet <= 10; feet += 2) circle(r, {CX, CY}, heightRadius(feet), grid);

    if (dataVisible) {
        const float nowHours = static_cast<float>(elapsedSeconds / 3600.0);
        for (int i = 0; i < 120; ++i) {
            float h1 = i * 12.0f / 120.0f, h2 = (i + 1) * 12.0f / 120.0f;
            float a1 = h1 * TAU / 12.0f, a2 = h2 * TAU / 12.0f;
            Point outer1 = polar(a1, CLOCK_RADIUS), outer2 = polar(a2, CLOCK_RADIUS);
            Point curve1 = polar(a1, heightRadius(tideHeight(nowHours + h1)));
            Point curve2 = polar(a2, heightRadius(tideHeight(nowHours + h2)));
            quad(r, {outer1, outer2, curve2, curve1}, fill);
            line(r, curve1, curve2, cyan, 3);
        }
        for (float h : {2.0f, 8.2f}) {
            float a = h * TAU / 12.0f;
            Point p = polar(a, heightRadius(tideHeight(nowHours + h)));
            filledCircle(r, p, 5, yellow);
        }
    }

    for (int minute = 0; minute < 60; ++minute) {
        const float angle = minute * TAU / 60.0f;
        const bool hour = minute % 5 == 0;
        line(r, polar(angle, CLOCK_RADIUS - (hour ? 12.0f : 6.0f)),
             polar(angle, CLOCK_RADIUS + 4.0f), hour ? pale : grid, hour ? 2 : 1);
    }
    for (int hour = 1; hour <= 12; ++hour) {
        Point p = polar((hour % 12) * TAU / 12.0f, CLOCK_RADIUS + 21.0f);
        text(r, static_cast<int>(p.x), static_cast<int>(p.y) - 5, std::to_string(hour), pale, 2);
    }

    std::time_t now = std::time(nullptr);
    std::tm local{}; localtime_s(&local, &now);
    const float second = local.tm_sec * TAU / 60.0f;
    const float minute = (local.tm_min + local.tm_sec / 60.0f) * TAU / 60.0f;
    const float hour = ((local.tm_hour % 12) + local.tm_min / 60.0f + local.tm_sec / 3600.0f) * TAU / 12.0f;
    line(r, {CX, CY}, polar(hour, CLOCK_RADIUS * .50f), pale, 5);
    line(r, {CX, CY}, polar(minute, CLOCK_RADIUS * .75f), pale, 3);
    line(r, polar(second, -CLOCK_RADIUS * .12f), polar(second, CLOCK_RADIUS * .85f), red, 2);
    filledCircle(r, {CX, CY}, 6, red);

    if (!dataVisible) text(r, CX, CY - 8, "NO TIDE DATA", offline, 3);
    text(r, CX, H - 24, dataVisible ? "NOAA LIVE" : "NOAA OFFLINE", dataVisible ? cyan : offline, 2);
    SDL_RenderPresent(r);
}
}

int main(int argc, char** argv) {
    bool smoke = false;
    for (int i = 1; i < argc; ++i) smoke |= std::string(argv[i]) == "--smoke";
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Respi Tide Clock Simulator - click to toggle NOAA data",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!renderer && window) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!window || !renderer) {
        std::fprintf(stderr, "SDL window/renderer failed: %s\n", SDL_GetError());
        if (window) SDL_DestroyWindow(window); SDL_Quit(); return 2;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    bool running = true, dataVisible = true;
    const Uint64 start = SDL_GetPerformanceCounter();
    int frames = 0;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)) dataVisible = !dataVisible;
        }
        const double elapsed = static_cast<double>(SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
        render(renderer, dataVisible, elapsed);
        if (smoke && ++frames >= 10) running = false;
        SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}
