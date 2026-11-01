// Vote.h - One anonymous ballot.
// SECRET BALLOT DESIGN: a vote stores only receipt + candidate + date. It does NOT
// store the voter's ID, so votes.txt cannot be used to see who voted for whom.
#pragma once
#include <string>

class Vote {
public:
    Vote(std::string receipt, std::string candidateId, std::string date);

    const std::string& receipt() const { return receipt_; }
    const std::string& candidateId() const { return candidateId_; }
    const std::string& date() const { return date_; }

    std::string serialize() const;
    static Vote deserialize(const std::string& line);

private:
    std::string receipt_, candidateId_, date_;
};
