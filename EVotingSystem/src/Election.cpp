// Election.cpp - A tiny state machine: NOT STARTED -> OPEN -> CLOSED (-> reset).
#include "Election.h"
#include "Exceptions.h"
#include "Utils.h"

Election::Election(std::string title) : title_(std::move(title)) {}

std::string Election::stateName() const {
    switch (state_) {
        case ElectionState::NotStarted: return "NOT STARTED";
        case ElectionState::Open:       return "OPEN";
        case ElectionState::Closed:     return "CLOSED";
    }
    return "UNKNOWN";
}

void Election::start() {
    if (state_ != ElectionState::NotStarted)
        throw ElectionStateException("The election can only be started when it is NOT STARTED (now " +
                                     stateName() + ").");
    state_ = ElectionState::Open;
    startTime_ = Utils::timestamp();
}

void Election::stop() {
    if (state_ != ElectionState::Open)
        throw ElectionStateException("Only an OPEN election can be stopped (now " + stateName() + ").");
    state_ = ElectionState::Closed;
    endTime_ = Utils::timestamp();
}

void Election::reset() {
    state_ = ElectionState::NotStarted;
    startTime_ = endTime_ = "-";
}

// File format: STATE_KEY|title|startTime|endTime
std::string Election::serialize() const {
    const char* key = state_ == ElectionState::Open     ? "OPEN"
                      : state_ == ElectionState::Closed ? "CLOSED"
                                                        : "NOT_STARTED";
    return std::string(key) + "|" + title_ + "|" + startTime_ + "|" + endTime_;
}

Election Election::deserialize(const std::string& line) {
    const auto f = Utils::split(line, '|');
    if (f.size() != 4) throw ValidationException("election record must have 4 fields");
    const std::string key = Utils::trim(f[0]), title = Utils::trim(f[1]);
    Election e(title);
    if (!Utils::isValidText(title, 3, 60)) throw ValidationException("bad election title");
    if (key == "OPEN") e.state_ = ElectionState::Open;
    else if (key == "CLOSED") e.state_ = ElectionState::Closed;
    else if (key == "NOT_STARTED") e.state_ = ElectionState::NotStarted;
    else throw ValidationException("unknown election state '" + key + "'");
    e.startTime_ = Utils::trim(f[2]);
    e.endTime_ = Utils::trim(f[3]);
    return e;
}
