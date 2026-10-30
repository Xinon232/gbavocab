#pragma once
// Settings > Flashcard angle: learning mode turned like gbareader's
// Angle. Saved in GBAVOCAB.SYS (one byte, header offset 23); old files read
// Normal. Only the learning screen turns; menus, the dictionary and the
// editor stay upright.
#include <cstdint>

enum class FlashcardAngle : uint8_t { normal, right, left, upside_down };
constexpr int FLASHCARD_ANGLES = 4;

inline bool sideways(FlashcardAngle angle)
{
    return angle == FlashcardAngle::right || angle == FlashcardAngle::left;
}

inline const char* flashcard_angle_name(FlashcardAngle angle)
{
    switch (angle) {
    case FlashcardAngle::right: return "90° right";
    case FlashcardAngle::left: return "90° left";
    case FlashcardAngle::upside_down: return "Upside down";
    default: return "Normal";
    }
}

// The D-pad as seen on the turned screen (gbareader's turn_keys): with the
// GBA turned 90° right the D-pad is on top and its Up points right.
struct TurnedPad { bool up, down, left, right; };
inline TurnedPad turn_pad(TurnedPad p, FlashcardAngle angle)
{
    switch (angle) {
    case FlashcardAngle::right: return {p.left, p.right, p.down, p.up};
    case FlashcardAngle::left: return {p.right, p.left, p.up, p.down};
    case FlashcardAngle::upside_down: return {p.down, p.up, p.right, p.left};
    default: return p;
    }
}
