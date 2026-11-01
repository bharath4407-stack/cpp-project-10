// Election.h - Holds the election state machine: NOT STARTED -> OPEN -> CLOSED.
#pragma once
#include <string>

enum class ElectionState { NotStarted, Open, Closed };

class Election {
public:
    explicit Election(std::string title = "General Election (Simulation)");

    ElectionState state() const { return state_; }
    bool isOpen() const { return state_ == ElectionState::Open; }
    const std::string& title() const { return title_; }
    const std::string& startTime() const { return startTime_; }
    const std::string& endTime() const { return endTime_; }

    std::string stateName() const;  // "NOT STARTED" / "OPEN" / "CLOSED"

    // State transitions. Illegal transitions throw ElectionStateException.
    void start();
    void stop();
    void reset();

    std::string serialize() const;
    static Election deserialize(const std::string& line);

private:
    ElectionState state_ = ElectionState::NotStarted;
    std::string title_;
    std::string startTime_ = "-";
    std::string endTime_ = "-";
};
