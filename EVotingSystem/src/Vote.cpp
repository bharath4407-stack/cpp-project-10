// Vote.cpp
#include "Vote.h"
#include "Exceptions.h"
#include "Utils.h"

Vote::Vote(std::string receipt, std::string candidateId, std::string date)
    : receipt_(std::move(receipt)), candidateId_(std::move(candidateId)), date_(std::move(date)) {}

// File format: receipt|candidateId|date
std::string Vote::serialize() const { return receipt_ + "|" + candidateId_ + "|" + date_; }

Vote Vote::deserialize(const std::string& line) {
    const auto f = Utils::split(line, '|');
    if (f.size() != 3) throw ValidationException("vote record must have 3 fields");
    const std::string receipt = Utils::trim(f[0]), cid = Utils::trim(f[1]), date = Utils::trim(f[2]);
    if (!Utils::isValidText(receipt, 6, 32) || receipt.find(' ') != std::string::npos)
        throw ValidationException("bad receipt number");
    if (!Utils::isValidCandidateId(cid)) throw ValidationException("bad candidate ID in vote " + receipt);
    if (date.size() != 10) throw ValidationException("bad date in vote " + receipt);
    return Vote(receipt, cid, date);
}
