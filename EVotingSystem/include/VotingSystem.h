// VotingSystem.h - The "brain" of the application: owns all data and enforces all rules.
// It never prints menus (that is ConsoleUI's job); it reports problems by throwing exceptions.
#pragma once
#include "Admin.h"
#include "Candidate.h"
#include "Election.h"
#include "FileManager.h"
#include "Vote.h"
#include "Voter.h"
#include <cstdint>
#include <map>
#include <random>
#include <string>
#include <vector>

struct ResultRow {
    Candidate candidate;
    int votes;
    double percentage;
};

struct Statistics {
    std::size_t registered;
    std::size_t votesCast;
    std::size_t notCast;
    double turnoutPercent;
};

enum class VoterSort { ById, ByName, ByStatus };

class VotingSystem {
public:
    explicit VotingSystem(const std::string& dataDir);

    void initialize();  // load files, create missing ones, seed demo data on first run
    const std::vector<std::string>& startupWarnings() const { return startupWarnings_; }

    // ---- Authentication ----
    Voter& loginVoter(const std::string& id, const std::string& password);
    Admin& loginAdmin(const std::string& id, const std::string& password);
    void changeAdminPassword(const std::string& oldPassword, const std::string& newPassword);

    // ---- Voter management ----
    Voter& registerVoter(const std::string& name, int age, const std::string& password,
                         const std::string& requestedId = "");
    bool voterExists(const std::string& id) const;
    void removeVoter(const std::string& id);
    std::vector<Voter> sortedVoters(VoterSort order) const;
    std::vector<Voter> searchVoters(const std::string& keyword) const;
    std::vector<const User*> allAccounts() const;  // Admin + all voters as base-class pointers

    // ---- Candidate management ----
    const std::vector<Candidate>& candidates() const { return candidates_; }
    const Candidate* findCandidate(const std::string& id) const;  // binary search
    const Candidate& addCandidate(const std::string& name, const std::string& party,
                                  const std::string& symbol, int age, const std::string& manifesto);
    void editCandidate(const std::string& id, const std::string& name, const std::string& party,
                       const std::string& symbol, int age, const std::string& manifesto);
    void removeCandidate(const std::string& id);
    std::vector<Candidate> searchCandidates(const std::string& keyword) const;  // linear search

    // ---- Election control ----
    const Election& election() const { return election_; }
    void startElection();
    void stopElection();
    Statistics statistics() const;

    // ---- Voting ----
    void checkCanVote(const Voter& voter) const;
    std::string castVote(Voter& voter, const std::string& candidateId);  // returns the receipt
    const Vote* findVote(const std::string& receipt) const;

    // ---- Results (only after CLOSED) ----
    std::vector<ResultRow> results() const;
    std::string winnerSummary(const std::vector<ResultRow>& rows) const;

    // ---- System tools ----
    void resetSimulation();   // clears votes + election state, keeps voters & candidates
    void restoreDemoData();   // restores the original demo voters/candidates
    std::vector<std::string> verifyIntegrity() const;
    std::vector<std::string> auditTail(std::size_t count) const;
    void log(const std::string& event, const std::string& details);

private:
    FileManager files_;
    Election election_;
    Admin admin_;
    std::map<std::string, Voter> voters_;  // key = Voter ID (fast lookup)
    std::vector<Candidate> candidates_;    // always kept sorted by numeric ID
    std::vector<Vote> votes_;
    std::vector<std::string> startupWarnings_;
    std::mt19937_64 rng_;

    Candidate* findCandidateMutable(const std::string& id);
    std::string nextVoterId() const;
    std::string nextCandidateId() const;
    std::string generateReceipt();
    void seedDemoData();
    void requireNotStarted(const std::string& action) const;
    static void validateCandidateFields(const std::string& name, const std::string& party,
                                        const std::string& symbol, int age,
                                        const std::string& manifesto);
};
