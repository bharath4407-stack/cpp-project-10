// Candidate.cpp
#include "Candidate.h"
#include "Exceptions.h"
#include "Utils.h"

#include <iostream>

Candidate::Candidate(std::string id, std::string name, std::string party, std::string symbol,
                     int age, std::string manifesto)
    : id_(std::move(id)), name_(std::move(name)), party_(std::move(party)),
      symbol_(std::move(symbol)), age_(age), manifesto_(std::move(manifesto)) {}

int Candidate::numericId() const { return std::stoi(id_.substr(1)); }

void Candidate::update(const std::string& name, const std::string& party,
                       const std::string& symbol, int age, const std::string& manifesto) {
    name_ = name;
    party_ = party;
    symbol_ = symbol;
    age_ = age;
    manifesto_ = manifesto;
}

bool Candidate::matches(const std::string& keyword) const {
    return Utils::containsIgnoreCase(id_, keyword) || Utils::containsIgnoreCase(name_, keyword) ||
           Utils::containsIgnoreCase(party_, keyword) || Utils::containsIgnoreCase(symbol_, keyword);
}

// File format: id|name|party|symbol|age|manifesto
std::string Candidate::serialize() const {
    return id_ + "|" + name_ + "|" + party_ + "|" + symbol_ + "|" + std::to_string(age_) + "|" +
           manifesto_;
}

void Candidate::displayCard() const {
    Utils::printLine('-');
    std::cout << "  Candidate ID : " << id_ << "\n"
              << "  Name         : " << name_ << "\n"
              << "  Party/Group  : " << party_ << "\n"
              << "  Symbol       : " << symbol_ << "\n"
              << "  Age          : " << age_ << "\n";
    const auto lines = Utils::wrap(manifesto_, 48);
    for (std::size_t i = 0; i < lines.size(); ++i)
        std::cout << (i == 0 ? "  Manifesto    : " : "                 ") << lines[i] << "\n";
    Utils::printLine('-');
}

std::string Candidate::tableHeader() {
    return Utils::padRight("ID", 6) + " | " + Utils::padRight("Name", 22) + " | " +
           Utils::padRight("Party/Group", 22) + " | " + Utils::padRight("Symbol", 12) + " | Age";
}

std::string Candidate::tableRow() const {
    return Utils::padRight(id_, 6) + " | " + Utils::padRight(name_, 22) + " | " +
           Utils::padRight(party_, 22) + " | " + Utils::padRight(symbol_, 12) + " | " +
           Utils::padLeft(std::to_string(age_), 3);
}

Candidate Candidate::deserialize(const std::string& line) {
    const auto f = Utils::split(line, '|');
    if (f.size() != 6) throw ValidationException("candidate record must have 6 fields");
    const std::string id = Utils::trim(f[0]), name = Utils::trim(f[1]), party = Utils::trim(f[2]),
                      symbol = Utils::trim(f[3]), manifesto = Utils::trim(f[5]);
    int age = 0;
    if (!Utils::isValidCandidateId(id)) throw ValidationException("bad candidate ID '" + id + "'");
    if (!Utils::isValidName(name)) throw ValidationException("bad candidate name for " + id);
    if (!Utils::isValidText(party, 2, 40)) throw ValidationException("bad party for " + id);
    if (!Utils::isValidText(symbol, 2, 20)) throw ValidationException("bad symbol for " + id);
    if (!Utils::tryParseInt(Utils::trim(f[4]), age) || age < 25 || age > 100)
        throw ValidationException("bad age for " + id);
    if (!Utils::isValidText(manifesto, 5, 120)) throw ValidationException("bad manifesto for " + id);
    return Candidate(id, name, party, symbol, age, manifesto);
}
