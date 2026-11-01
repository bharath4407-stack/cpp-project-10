// Utils.h - Helper functions: string tools, validation, hashing, console I/O.
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace Utils {

constexpr int kWidth = 70;  // width of borders / headers in the console UI

// ANSI colour codes (only printed when colour is enabled).
namespace Color {
constexpr const char* Reset  = "\033[0m";
constexpr const char* Bold   = "\033[1m";
constexpr const char* Red    = "\033[31m";
constexpr const char* Green  = "\033[32m";
constexpr const char* Yellow = "\033[33m";
constexpr const char* Cyan   = "\033[36m";
}  // namespace Color

// ---- Console setup -------------------------------------------------------
void initConsole(bool forceNoColor, bool forceAscii);
bool unicodeEnabled();
std::string paint(const std::string& text, const std::string& colorCode);

// ---- String helpers ------------------------------------------------------
std::string trim(const std::string& s);
std::string toLower(std::string s);
std::string toUpper(std::string s);
std::vector<std::string> split(const std::string& s, char delimiter);
std::string sanitize(const std::string& s);                 // removes '|' and newlines (file-safe)
std::string repeat(const std::string& unit, std::size_t n);
std::string padRight(const std::string& s, std::size_t width);  // truncates with ".." if too long
std::string padLeft(const std::string& s, std::size_t width);
std::string centered(const std::string& s, std::size_t width);
std::vector<std::string> wrap(const std::string& text, std::size_t width);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);
bool tryParseInt(const std::string& s, int& out);            // digits only, max 9 chars

// ---- Validation ----------------------------------------------------------
bool isValidName(const std::string& s);
bool isValidId(const std::string& s);                        // 4-12 letters/digits
bool isValidText(const std::string& s, std::size_t minLen, std::size_t maxLen);
bool isValidPassword(const std::string& s);                  // 6-32 printable chars
bool isValidCandidateId(const std::string& s);               // C + digits
bool isValidHash(const std::string& s);                      // 16 hex digits
std::string normalizeCandidateId(const std::string& s);      // "102" / "c102" -> "C102"

// ---- Security (educational only!) ---------------------------------------
std::string hashPassword(const std::string& userId, const std::string& password);

// ---- Time ----------------------------------------------------------------
std::string timestamp();   // 2025-01-31 14:05:09
std::string dateStamp();   // 2025-01-31

// ---- Console output ------------------------------------------------------
void clearScreen();
void printLine(char c = '-');
void printHeader(const std::string& title, const std::string& subtitle = "");
void success(const std::string& msg);
void error(const std::string& msg);
void warn(const std::string& msg);
void info(const std::string& msg);

// ---- Console input (all of these re-prompt until the input is valid) ------
std::string readLine(const std::string& prompt);
int readInt(const std::string& prompt, int minValue, int maxValue);
std::string readValidated(const std::string& prompt,
                          const std::function<bool(const std::string&)>& isValid,
                          const std::string& errorMessage);
bool confirm(const std::string& prompt);
void pause();

}  // namespace Utils
