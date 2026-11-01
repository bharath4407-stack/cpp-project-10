// FileManager.h - All file I/O lives here (fstream). Other classes never touch files.
#pragma once
#include "Admin.h"
#include "Candidate.h"
#include "Election.h"
#include "Vote.h"
#include "Voter.h"
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

class FileManager {
public:
    explicit FileManager(const std::string& dataDir);

    void ensureDirectory();
    bool exists(const std::string& fileName) const;

    std::map<std::string, Voter> loadVoters();
    void saveVoters(const std::map<std::string, Voter>& voters);

    std::vector<Candidate> loadCandidates();
    void saveCandidates(const std::vector<Candidate>& candidates);

    std::vector<Vote> loadVotes();
    void appendVote(const Vote& vote);               // votes are appended immediately (durable)
    void saveVotes(const std::vector<Vote>& votes);

    std::optional<Admin> loadAdmin();
    void saveAdmin(const Admin& admin);

    Election loadElection();
    void saveElection(const Election& election);

    void appendAudit(const std::string& event, const std::string& details);
    std::vector<std::string> readAuditTail(std::size_t count) const;

    // Problems found while loading (missing/corrupted records). The UI shows them.
    const std::vector<std::string>& warnings() const { return warnings_; }

private:
    std::filesystem::path dir_;
    std::vector<std::string> warnings_;

    std::string path(const std::string& fileName) const;
    bool readDataLines(const std::string& fileName, std::vector<std::string>& lines) const;
    void writeDataFile(const std::string& fileName, const std::string& header,
                       const std::vector<std::string>& lines);
    void backupCorrupted(const std::string& fileName);
};
