#pragma once

enum Phase { WORK, SHORT_BREAK, LONG_BREAK };
enum State { READY, RUNNING, PAUSED };
enum Screen { MAIN, SETTINGS, ANIM };

struct Button { int16_t x, y, w, h; };

inline bool hit(const Button &b, int x, int y) {
  return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
}

// Coordonnée écran = a * valeur brute + b ; swap = axes X/Y inversés
struct Calibration { bool swap; float ax, bx, ay, by; };
