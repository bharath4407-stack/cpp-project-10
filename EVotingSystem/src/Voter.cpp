// Voter.cpp
#include "Voter.h"
#include "Exceptions.h"
#include "Utils.h"

#include <iostream>

Voter::Voter(std::string id, std::string name, int age, std::string passwordHash, bool hasVoted)
    : User(std::move(id), std::move(name), std::move(passwordHash)),  // call the base constructor
      age_(age),
      hasVoted_(hasVoted) {}

void Voter::markVoted() {
    if (hasVoted_) throw DuplicateVoteException("This voter has already cast a vote.");
    hasVoted_ = true;
}

void Voter::resetVoteStatus() { hasVoted_ = false; }

// Overrides User::displayProfile()
void Voter::displayProfile() const {
    Utils::printHeader("VOTER PROFILE");
    std::cout << "  Voter ID : " << id_ << "\n"
              << "  Name     : " << name_ << "\n"
              << "  Age      : " << age_ << "\n"
              << "  Role     : " << role() << "\n"
              << "  Status   : "
              << (hasVoted_ ? Utils::paint("Vote already cast", Utils::Color::Green)
                            : Utils::paint("Has not voted yet", Utils::Color::Yellow))
              << "\n";
    Utils::printLine('=');
}

// File format: id|name|age|passwordHash|hasVoted
std::string Voter::serialize() const {
    return id_ + "|" + name_ + "|" + std::to_string(age_) + "|" + passwordHash_ + "|" +
           (hasVoted_ ? "1" : "0");
}

std::string Voter::tableHeader() {
    return Utils::padRight("Voter ID", 10) + " | " + Utils::padRight("Name", 24) + " | Age | Status";
}

std::string Voter::tableRow() const {
    return Utils::padRight(id_, 10) + " | " + Utils::padRight(name_, 24) + " | " +
           Utils::padLeft(std::to_string(age_), 3) + " | " + (hasVoted_ ? "Voted" : "Not voted");
}

Voter Voter::deserialize(const std::string& line) {
    const auto f = Utils::split(line, '|');
    if (f.size() != 5) throw ValidationException("voter record must have 5 fields");
    const std::string id = Utils::toUpper(Utils::trim(f[0]));
    const std::string name = Utils::trim(f[1]);
    const std::string hash = Utils::trim(f[3]);
    const std::string voted = Utils::trim(f[4]);
    int age = 0;
    if (!Utils::isValidId(id)) throw ValidationException("bad voter ID '" + id + "'");
    if (!Utils::isValidName(name)) throw ValidationException("bad voter name for " + id);
    if (!Utils::tryParseInt(Utils::trim(f[2]), age) || age < 18 || age > 120)
        throw ValidationException("bad age for " + id);
    if (!Utils::isValidHash(hash)) throw ValidationException("bad password hash for " + id);
    if (voted != "0" && voted != "1") throw ValidationException("bad vote flag for " + id);
    return Voter(id, name, age, hash, voted == "1");
}
