// Utils.cpp - Implementation of helper functions.
#include "Utils.h"
#include "Exceptions.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Utils {

namespace {
bool g_color = true;    // are ANSI colours / screen clearing allowed?
bool g_unicode = true;  // may we print the block character for bar charts?

bool stdoutIsTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

std::string formatNow(const char* format) {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof buffer, format, &local);
    return buffer;
}

bool isPrintableAscii(const std::string& s) {
    for (unsigned char c : s)
        if (c < 32 || c > 126) return false;
    return true;
}
}  // namespace

// ---------------------------------------------------------------- console setup
void initConsole(bool forceNoColor, bool forceAscii) {
    g_unicode = !forceAscii;
    // Colours are only used when output goes to a real terminal (not a pipe/file).
    g_color = !forceNoColor && stdoutIsTerminal();
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        if (!SetConsoleMode(h, mode | 0x0004)) g_color = false;  // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    } else {
        g_color = false;
    }
#endif
}

bool unicodeEnabled() { return g_unicode; }

std::string paint(const std::string& text, const std::string& colorCode) {
    return g_color ? colorCode + text + Color::Reset : text;
}

// ---------------------------------------------------------------- strings
std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);  // keeps empty fields too
    return parts;
}

std::string sanitize(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '|') out += '/';
        else if (c == '\n' || c == '\r') out += ' ';
        else out += c;
    }
    return trim(out);
}

std::string repeat(const std::string& unit, std::size_t n) {
    std::string out;
    out.reserve(unit.size() * n);
    for (std::size_t i = 0; i < n; ++i) out += unit;
    return out;
}

std::string padRight(const std::string& s, std::size_t width) {
    if (s.size() > width) return width > 2 ? s.substr(0, width - 2) + ".." : s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

std::string padLeft(const std::string& s, std::size_t width) {
    if (s.size() >= width) return s;
    return std::string(width - s.size(), ' ') + s;
}

std::string centered(const std::string& s, std::size_t width) {
    if (s.size() >= width) return s;
    return std::string((width - s.size()) / 2, ' ') + s;
}

std::vector<std::string> wrap(const std::string& text, std::size_t width) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word, line;
    while (words >> word) {
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool tryParseInt(const std::string& s, int& out) {
    if (s.empty() || s.size() > 9) return false;
    for (unsigned char c : s)
        if (!std::isdigit(c)) return false;
    out = std::stoi(s);
    return true;
}

// ---------------------------------------------------------------- validation
bool isValidName(const std::string& s) {
    if (s.size() < 2 || s.size() > 40) return false;
    if (!std::isalpha(static_cast<unsigned char>(s[0]))) return false;
    for (unsigned char c : s)
        if (!(std::isalpha(c) || c == ' ' || c == '.' || c == '-' || c == '\'')) return false;
    return true;
}

bool isValidId(const std::string& s) {
    if (s.size() < 4 || s.size() > 12) return false;
    for (unsigned char c : s)
        if (!std::isalnum(c)) return false;
    return true;
}

bool isValidText(const std::string& s, std::size_t minLen, std::size_t maxLen) {
    return s.size() >= minLen && s.size() <= maxLen && isPrintableAscii(s) &&
           s.find('|') == std::string::npos;
}

bool isValidPassword(const std::string& s) {
    return s.size() >= 6 && s.size() <= 32 && isPrintableAscii(s);
}

bool isValidCandidateId(const std::string& s) {
    if (s.size() < 2 || s.size() > 7 || s[0] != 'C') return false;
    for (std::size_t i = 1; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}

bool isValidHash(const std::string& s) {
    if (s.size() != 16) return false;
    for (unsigned char c : s)
        if (!std::isxdigit(c)) return false;
    return true;
}

std::string normalizeCandidateId(const std::string& s) {
    std::string id = toUpper(trim(s));
    bool allDigits = !id.empty();
    for (unsigned char c : id)
        if (!std::isdigit(c)) allDigits = false;
    return allDigits ? "C" + id : id;  // allow the user to type just "102"
}

// ---------------------------------------------------------------- security
// FNV-1a 64-bit hash with a per-user salt. This is FOR LEARNING ONLY: real systems
// use slow, vetted algorithms such as bcrypt / Argon2.
std::string hashPassword(const std::string& userId, const std::string& password) {
    const std::string salted = "evs-sim:" + toLower(userId) + ":" + password;
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : salted) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

// ---------------------------------------------------------------- time
std::string timestamp() { return formatNow("%Y-%m-%d %H:%M:%S"); }
std::string dateStamp() { return formatNow("%Y-%m-%d"); }

// ---------------------------------------------------------------- output
void clearScreen() {
    if (g_color) std::cout << "\033[2J\033[H";  // only on a real terminal
}

void printLine(char c) { std::cout << repeat(std::string(1, c), kWidth) << "\n"; }

void printHeader(const std::string& title, const std::string& subtitle) {
    const std::string bar = repeat("=", kWidth);
    std::cout << "\n" << paint(bar, Color::Cyan) << "\n"
              << paint(centered(title, kWidth), std::string(Color::Bold) + Color::Cyan) << "\n";
    if (!subtitle.empty()) std::cout << centered(subtitle, kWidth) << "\n";
    std::cout << paint(bar, Color::Cyan) << "\n";
}

void success(const std::string& msg) { std::cout << paint("  [OK] " + msg, Color::Green) << "\n"; }
void error(const std::string& msg) { std::cout << paint("  [ERROR] " + msg, Color::Red) << "\n"; }
void warn(const std::string& msg) { std::cout << paint("  [WARNING] " + msg, Color::Yellow) << "\n"; }
void info(const std::string& msg) { std::cout << "  [INFO] " << msg << "\n"; }

// ---------------------------------------------------------------- input
std::string readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosedException("Input stream closed");
    return trim(line);
}

int readInt(const std::string& prompt, int minValue, int maxValue) {
    while (true) {
        int value = 0;
        if (tryParseInt(readLine(prompt), value) && value >= minValue && value <= maxValue)
            return value;
        error("Invalid input. Please enter a whole number between " + std::to_string(minValue) +
              " and " + std::to_string(maxValue) + ".");
    }
}

std::string readValidated(const std::string& prompt,
                          const std::function<bool(const std::string&)>& isValid,
                          const std::string& errorMessage) {
    while (true) {
        std::string value = readLine(prompt);
        if (isValid(value)) return value;
        error(errorMessage);
    }
}

bool confirm(const std::string& prompt) {
    while (true) {
        const std::string answer = toLower(readLine(prompt));
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        error("Please answer with y (yes) or n (no).");
    }
}

void pause() { readLine("\n  Press Enter to continue..."); }

}  // namespace Utils
