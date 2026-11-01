// FileManager.cpp - Reads/writes the six data files with std::ifstream / std::ofstream.
// Files are plain text, one record per line, fields separated by '|'.
// Lines starting with '#' are comments. Corrupted lines are skipped (with a warning),
// and the damaged file is copied to <name>.corrupt.bak so nothing is lost.
#include "FileManager.h"
#include "Exceptions.h"
#include "Utils.h"

#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& dataDir) : dir_(dataDir) {}

void FileManager::ensureDirectory() {
    std::error_code ec;
    fs::create_directories(dir_, ec);
    if (ec) throw FileException("Cannot create data folder '" + dir_.string() + "': " + ec.message());
}

std::string FileManager::path(const std::string& fileName) const { return (dir_ / fileName).string(); }

bool FileManager::exists(const std::string& fileName) const {
    std::error_code ec;
    return fs::exists(dir_ / fileName, ec);
}

// Reads all non-empty, non-comment lines. Returns false if the file does not exist.
bool FileManager::readDataLines(const std::string& fileName, std::vector<std::string>& lines) const {
    std::ifstream in(path(fileName));
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        line = Utils::trim(line);  // also strips Windows '\r'
        if (line.empty() || line[0] == '#') continue;
        lines.push_back(line);
    }
    return true;
}

// Safe write: write to "<file>.tmp" first, then rename over the real file.
// If the program crashes mid-write, the old file is still intact.
void FileManager::writeDataFile(const std::string& fileName, const std::string& header,
                                const std::vector<std::string>& lines) {
    const std::string finalPath = path(fileName), tmpPath = finalPath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::trunc);
        if (!out) throw FileException("Cannot open '" + tmpPath + "' for writing.");
        out << "# " << header << "\n";
        for (const auto& l : lines) out << l << "\n";
        out.flush();
        if (!out) throw FileException("Failed while writing '" + tmpPath + "'.");
    }
    std::error_code ec;
    fs::rename(tmpPath, finalPath, ec);
    if (ec) {  // fallback for file systems where rename-over-existing fails
        ec.clear();
        fs::copy_file(tmpPath, finalPath, fs::copy_options::overwrite_existing, ec);
        std::error_code ignore;
        fs::remove(tmpPath, ignore);
        if (ec) throw FileException("Cannot update '" + finalPath + "': " + ec.message());
    }
}

void FileManager::backupCorrupted(const std::string& fileName) {
    std::error_code ec;
    fs::copy_file(dir_ / fileName, dir_ / (fileName + ".corrupt.bak"),
                  fs::copy_options::overwrite_existing, ec);
}

// ------------------------------------------------------------------ voters
std::map<std::string, Voter> FileManager::loadVoters() {
    std::map<std::string, Voter> result;
    std::vector<std::string> lines;
    if (!readDataLines("voters.txt", lines)) return result;
    int skipped = 0;
    for (const auto& l : lines) {
        try {
            Voter v = Voter::deserialize(l);
            if (!result.emplace(v.id(), v).second) throw ValidationException("duplicate voter " + v.id());
        } catch (const VotingException& e) {
            ++skipped;
            warnings_.push_back("voters.txt: skipped a corrupted record (" + std::string(e.what()) + ")");
        }
    }
    if (skipped > 0) backupCorrupted("voters.txt");
    return result;
}

void FileManager::saveVoters(const std::map<std::string, Voter>& voters) {
    std::vector<std::string> lines;
    for (const auto& kv : voters) lines.push_back(kv.second.serialize());
    writeDataFile("voters.txt", "voterId|name|age|passwordHash|hasVoted(0/1)", lines);
}

// ------------------------------------------------------------------ candidates
std::vector<Candidate> FileManager::loadCandidates() {
    std::vector<Candidate> result;
    std::vector<std::string> lines;
    if (!readDataLines("candidates.txt", lines)) return result;
    int skipped = 0;
    for (const auto& l : lines) {
        try {
            Candidate c = Candidate::deserialize(l);
            for (const auto& existing : result)
                if (existing.numericId() == c.numericId())
                    throw ValidationException("duplicate candidate " + c.id());
            result.push_back(c);
        } catch (const VotingException& e) {
            ++skipped;
            warnings_.push_back("candidates.txt: skipped a corrupted record (" + std::string(e.what()) + ")");
        }
    }
    if (skipped > 0) backupCorrupted("candidates.txt");
    std::sort(result.begin(), result.end(),
              [](const Candidate& a, const Candidate& b) { return a.numericId() < b.numericId(); });
    return result;
}

void FileManager::saveCandidates(const std::vector<Candidate>& candidates) {
    std::vector<std::string> lines;
    for (const auto& c : candidates) lines.push_back(c.serialize());
    writeDataFile("candidates.txt", "candidateId|name|party|symbol|age|manifesto", lines);
}

// ------------------------------------------------------------------ votes
std::vector<Vote> FileManager::loadVotes() {
    std::vector<Vote> result;
    std::vector<std::string> lines;
    if (!readDataLines("votes.txt", lines)) return result;
    int skipped = 0;
    for (const auto& l : lines) {
        try {
            result.push_back(Vote::deserialize(l));
        } catch (const VotingException& e) {
            ++skipped;
            warnings_.push_back("votes.txt: skipped a corrupted record (" + std::string(e.what()) + ")");
        }
    }
    if (skipped > 0) backupCorrupted("votes.txt");
    return result;
}

void FileManager::appendVote(const Vote& vote) {
    if (!exists("votes.txt")) saveVotes({});
    std::ofstream out(path("votes.txt"), std::ios::app);
    if (!out) throw FileException("Cannot open votes.txt - your vote was NOT recorded.");
    out << vote.serialize() << "\n";
    out.flush();
    if (!out) throw FileException("Failed to write votes.txt - your vote was NOT recorded.");
}

void FileManager::saveVotes(const std::vector<Vote>& votes) {
    std::vector<std::string> lines;
    for (const auto& v : votes) lines.push_back(v.serialize());
    writeDataFile("votes.txt", "receipt|candidateId|date   (no voter ID stored: secret ballot)", lines);
}

// ------------------------------------------------------------------ admin
std::optional<Admin> FileManager::loadAdmin() {
    std::vector<std::string> lines;
    if (!readDataLines("admin.txt", lines) || lines.empty()) return std::nullopt;
    try {
        return Admin::deserialize(lines.front());
    } catch (const VotingException& e) {
        warnings_.push_back("admin.txt: corrupted (" + std::string(e.what()) + ")");
        backupCorrupted("admin.txt");
        return std::nullopt;
    }
}

void FileManager::saveAdmin(const Admin& admin) {
    writeDataFile("admin.txt", "adminId|name|passwordHash", {admin.serialize()});
}

// ------------------------------------------------------------------ election
Election FileManager::loadElection() {
    std::vector<std::string> lines;
    if (!readDataLines("election.txt", lines) || lines.empty()) return Election();
    try {
        return Election::deserialize(lines.front());
    } catch (const VotingException& e) {
        warnings_.push_back("election.txt: corrupted (" + std::string(e.what()) +
                            "); election reset to NOT STARTED");
        backupCorrupted("election.txt");
        return Election();
    }
}

void FileManager::saveElection(const Election& election) {
    writeDataFile("election.txt", "state|title|startTime|endTime", {election.serialize()});
}

// ------------------------------------------------------------------ audit log
void FileManager::appendAudit(const std::string& event, const std::string& details) {
    const bool isNew = !exists("audit.txt");
    std::ofstream out(path("audit.txt"), std::ios::app);
    if (!out) return;  // a failing log must never stop the voting process
    if (isNew) out << "# timestamp|event|details\n";
    out << Utils::timestamp() << "|" << Utils::sanitize(event) << "|" << Utils::sanitize(details) << "\n";
}

std::vector<std::string> FileManager::readAuditTail(std::size_t count) const {
    std::vector<std::string> lines;
    readDataLines("audit.txt", lines);
    if (lines.size() > count) lines.erase(lines.begin(), lines.end() - static_cast<std::ptrdiff_t>(count));
    return lines;
}
