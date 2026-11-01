// Voter.h - A registered voter (inherits from User).
#pragma once
#include "User.h"

class Voter : public User {
public:
    Voter(std::string id, std::string name, int age, std::string passwordHash, bool hasVoted = false);

    int age() const { return age_; }
    bool hasVoted() const { return hasVoted_; }

    // Encapsulation: the flag can only change through these two controlled functions.
    void markVoted();         // throws DuplicateVoteException if already voted
    void resetVoteStatus();   // used by "Reset simulation"

    // Overrides of the pure virtual functions in User
    std::string role() const override { return "Voter"; }
    void displayProfile() const override;
    std::string serialize() const override;

    std::string tableRow() const;
    static std::string tableHeader();
    static Voter deserialize(const std::string& line);  // throws ValidationException if corrupted

private:
    int age_;
    bool hasVoted_;
};
