// Candidate.h - One candidate standing in the election.
#pragma once
#include <string>

class Candidate {
public:
    Candidate(std::string id, std::string name, std::string party, std::string symbol,
              int age, std::string manifesto);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& party() const { return party_; }
    const std::string& symbol() const { return symbol_; }
    int age() const { return age_; }
    const std::string& manifesto() const { return manifesto_; }
    int numericId() const;  // "C102" -> 102 (used by binary search)

    void update(const std::string& name, const std::string& party, const std::string& symbol,
                int age, const std::string& manifesto);

    bool matches(const std::string& keyword) const;  // case-insensitive search on id/name/party/symbol
    std::string serialize() const;
    void displayCard() const;
    std::string tableRow() const;
    static std::string tableHeader();
    static Candidate deserialize(const std::string& line);

private:
    std::string id_, name_, party_, symbol_;
    int age_;
    std::string manifesto_;
};
