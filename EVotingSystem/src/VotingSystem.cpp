// VotingSystem.cpp - Business rules of the election.
#include "VotingSystem.h"
#include "Exceptions.h"
#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <set>

namespace {
const char* kDefaultAdminId = "admin";
const char* kDefaultAdminPassword = "admin123";
const char* kDemoVoterPassword = "pass123";
}  // namespace

VotingSystem::VotingSystem(const std::string& dataDir)
    : files_(dataDir),
      admin_(kDefaultAdminId, "System Administrator",
             Utils::hashPassword(kDefaultAdminId, kDefaultAdminPassword)) {
    std::random_device rd;
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    rng_.seed((static_cast<std::uint64_t>(rd()) << 32) ^ rd() ^ static_cast<std::uint64_t>(now));
}

// ============================================================ start-up
void VotingSystem::initialize() {
    files_.ensureDirectory();
    const bool firstRun = !files_.exists("voters.txt") && !files_.exists("candidates.txt");

    voters_ = files_.loadVoters();
    candidates_ = files_.loadCandidates();
    votes_ = files_.loadVotes();
    election_ = files_.loadElection();

    if (auto loaded = files_.loadAdmin()) {
        admin_ = *loaded;
    } else {
        // On the very first run this is expected, so only warn on later runs.
        if (!firstRun) startupWarnings_.push_back("admin.txt missing or damaged: default admin account created.");
        files_.saveAdmin(admin_);
    }

    if (firstRun) {
        seedDemoData();  // first launch: create demo voters and candidates
        files_.saveVoters(voters_);
        files_.saveCandidates(candidates_);
    }
    // Make sure every data file exists (only creates missing ones; never overwrites).
    if (!files_.exists("voters.txt")) files_.saveVoters(voters_);
    if (!files_.exists("candidates.txt")) files_.saveCandidates(candidates_);
    if (!files_.exists("votes.txt")) files_.saveVotes(votes_);
    if (!files_.exists("election.txt")) files_.saveElection(election_);

    for (const auto& w : files_.warnings()) startupWarnings_.push_back(w);
    log("SYSTEM_START", firstRun ? "first run - demo data created" : "data loaded");
}

void VotingSystem::seedDemoData() {
    voters_.clear();
    candidates_.clear();
    const char* names[] = {"Aarav Sharma", "Priya Reddy",  "Rahul Verma", "Sneha Iyer", "Karthik Rao",
                           "Ananya Das",   "Vikram Singh", "Meera Nair",  "Rohan Gupta", "Divya Menon"};
    const int ages[] = {25, 31, 22, 28, 45, 19, 38, 52, 29, 34};
    for (int i = 0; i < 10; ++i) {
        const std::string id = "V" + std::to_string(1001 + i);
        voters_.emplace(id, Voter(id, names[i], ages[i], Utils::hashPassword(id, kDemoVoterPassword)));
    }
    candidates_.emplace_back("C101", "Arjun Mehta", "Progress Alliance", "Sun", 45,
                             "Better roads, reliable power and affordable housing for all.");
    candidates_.emplace_back("C102", "Lakshmi Devi", "Green Future Party", "Tree", 52,
                             "Clean energy, urban forests and zero-waste campuses.");
    candidates_.emplace_back("C103", "Sameer Khan", "Unity Front", "Bridge", 38,
                             "Free public Wi-Fi, open data and support for local startups.");
    candidates_.emplace_back("C104", "Neha Kapoor", "Citizens First", "Lamp", 41,
                             "Transparent budgets, quality schools and community healthcare.");
}

void VotingSystem::log(const std::string& event, const std::string& details) {
    files_.appendAudit(event, details);
}

// ============================================================ authentication
Voter& VotingSystem::loginVoter(const std::string& id, const std::string& password) {
    auto it = voters_.find(Utils::toUpper(id));
    if (it == voters_.end() || !it->second.authenticate(password)) {
        log("LOGIN_FAILED", "voter login, id entered: " + id);
        // Same message for "unknown ID" and "wrong password" so attackers learn nothing.
        throw AuthenticationException("Invalid Voter ID or password.");
    }
    log("LOGIN_OK", "voter " + it->first);
    return it->second;
}

Admin& VotingSystem::loginAdmin(const std::string& id, const std::string& password) {
    if (Utils::toLower(id) != admin_.id() || !admin_.authenticate(password)) {
        log("LOGIN_FAILED", "admin login, id entered: " + id);
        throw AuthenticationException("Invalid Admin ID or password.");
    }
    log("LOGIN_OK", "admin " + admin_.id());
    return admin_;
}

void VotingSystem::changeAdminPassword(const std::string& oldPassword, const std::string& newPassword) {
    if (!admin_.authenticate(oldPassword)) throw AuthenticationException("Current password is incorrect.");
    if (!Utils::isValidPassword(newPassword))
        throw ValidationException("New password must be 6-32 printable characters.");
    admin_.setPassword(newPassword);
    files_.saveAdmin(admin_);
    log("ADMIN_PASSWORD_CHANGED", admin_.id());
}

// ============================================================ voters
std::string VotingSystem::nextVoterId() const {
    int highest = 1000;
    for (const auto& kv : voters_) {
        int n = 0;
        if (kv.first.size() > 1 && kv.first[0] == 'V' && Utils::tryParseInt(kv.first.substr(1), n))
            highest = std::max(highest, n);
    }
    std::string id;
    do { id = "V" + std::to_string(++highest); } while (voters_.count(id) > 0);
    return id;
}

Voter& VotingSystem::registerVoter(const std::string& name, int age, const std::string& password,
                                   const std::string& requestedId) {
    if (election_.state() == ElectionState::Closed)
        throw ElectionStateException("Registration is closed: the election has ended.");
    if (!Utils::isValidName(name))
        throw ValidationException("Name must be 2-40 characters: letters, spaces, . - ' only.");
    if (age < 18 || age > 120) throw ValidationException("Voters must be between 18 and 120 years old.");
    if (!Utils::isValidPassword(password))
        throw ValidationException("Password must be 6-32 printable characters.");

    std::string id;
    if (requestedId.empty()) {
        id = nextVoterId();
    } else {
        id = Utils::toUpper(requestedId);
        if (!Utils::isValidId(id)) throw ValidationException("Voter ID must be 4-12 letters/digits.");
        if (voters_.count(id)) throw ValidationException("Voter ID '" + id + "' is already taken.");
    }
    auto result = voters_.emplace(id, Voter(id, Utils::sanitize(name), age, Utils::hashPassword(id, password)));
    files_.saveVoters(voters_);
    log("VOTER_REGISTERED", id);
    return result.first->second;
}

bool VotingSystem::voterExists(const std::string& id) const { return voters_.count(Utils::toUpper(id)) > 0; }

void VotingSystem::removeVoter(const std::string& rawId) {
    requireNotStarted("Removing voters");
    const std::string id = Utils::toUpper(rawId);
    if (voters_.erase(id) == 0) throw ValidationException("No voter with ID '" + id + "'.");
    files_.saveVoters(voters_);
    log("VOTER_REMOVED", id);
}

std::vector<Voter> VotingSystem::sortedVoters(VoterSort order) const {
    std::vector<Voter> list;
    for (const auto& kv : voters_) list.push_back(kv.second);
    switch (order) {  // std::sort with a lambda comparator
        case VoterSort::ById:
            std::sort(list.begin(), list.end(), [](const Voter& a, const Voter& b) { return a.id() < b.id(); });
            break;
        case VoterSort::ByName:
            std::sort(list.begin(), list.end(), [](const Voter& a, const Voter& b) {
                return Utils::toLower(a.name()) < Utils::toLower(b.name());
            });
            break;
        case VoterSort::ByStatus:  // voted first, then by ID
            std::sort(list.begin(), list.end(), [](const Voter& a, const Voter& b) {
                if (a.hasVoted() != b.hasVoted()) return a.hasVoted();
                return a.id() < b.id();
            });
            break;
    }
    return list;
}

std::vector<Voter> VotingSystem::searchVoters(const std::string& keyword) const {
    std::vector<Voter> found;
    for (const auto& kv : voters_)  // linear search
        if (Utils::containsIgnoreCase(kv.second.id(), keyword) || Utils::containsIgnoreCase(kv.second.name(), keyword))
            found.push_back(kv.second);
    return found;
}

std::vector<const User*> VotingSystem::allAccounts() const {
    std::vector<const User*> accounts{&admin_};  // base-class pointers: polymorphism
    for (const auto& kv : voters_) accounts.push_back(&kv.second);
    return accounts;
}

// ============================================================ candidates
void VotingSystem::requireNotStarted(const std::string& action) const {
    if (election_.state() != ElectionState::NotStarted)
        throw ElectionStateException(action + " is only allowed while the election is NOT STARTED (now " +
                                     election_.stateName() + ").");
}

void VotingSystem::validateCandidateFields(const std::string& name, const std::string& party,
                                           const std::string& symbol, int age, const std::string& manifesto) {
    if (!Utils::isValidName(name)) throw ValidationException("Candidate name: 2-40 characters, letters/spaces/. - ' only.");
    if (!Utils::isValidText(party, 2, 40)) throw ValidationException("Party/group must be 2-40 printable characters (no '|').");
    if (!Utils::isValidText(symbol, 2, 20)) throw ValidationException("Symbol must be 2-20 printable characters (no '|').");
    if (age < 25 || age > 100) throw ValidationException("Candidates must be between 25 and 100 years old.");
    if (!Utils::isValidText(manifesto, 5, 120)) throw ValidationException("Manifesto must be 5-120 printable characters (no '|').");
}

std::string VotingSystem::nextCandidateId() const {
    int highest = 100;
    for (const auto& c : candidates_) highest = std::max(highest, c.numericId());
    return "C" + std::to_string(highest + 1);
}

// BINARY SEARCH: candidates_ is always sorted by numeric ID, so we can halve the range each step.
const Candidate* VotingSystem::findCandidate(const std::string& rawId) const {
    const std::string id = Utils::normalizeCandidateId(rawId);
    if (!Utils::isValidCandidateId(id)) return nullptr;
    const int key = std::stoi(id.substr(1));
    std::size_t low = 0, high = candidates_.size();
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        const int value = candidates_[mid].numericId();
        if (value == key) return &candidates_[mid];
        if (value < key) low = mid + 1;
        else high = mid;
    }
    return nullptr;
}

Candidate* VotingSystem::findCandidateMutable(const std::string& id) {
    return const_cast<Candidate*>(static_cast<const VotingSystem*>(this)->findCandidate(id));
}

const Candidate& VotingSystem::addCandidate(const std::string& name, const std::string& party,
                                            const std::string& symbol, int age, const std::string& manifesto) {
    requireNotStarted("Adding candidates");
    validateCandidateFields(name, party, symbol, age, manifesto);
    candidates_.emplace_back(nextCandidateId(), name, party, symbol, age, manifesto);  // IDs only grow -> stays sorted
    files_.saveCandidates(candidates_);
    log("CANDIDATE_ADDED", candidates_.back().id());
    return candidates_.back();
}

void VotingSystem::editCandidate(const std::string& id, const std::string& name, const std::string& party,
                                 const std::string& symbol, int age, const std::string& manifesto) {
    requireNotStarted("Editing candidates");
    Candidate* c = findCandidateMutable(id);
    if (!c) throw ValidationException("No candidate with ID '" + id + "'.");
    validateCandidateFields(name, party, symbol, age, manifesto);
    c->update(name, party, symbol, age, manifesto);
    files_.saveCandidates(candidates_);
    log("CANDIDATE_EDITED", c->id());
}

void VotingSystem::removeCandidate(const std::string& id) {
    requireNotStarted("Removing candidates");
    const Candidate* c = findCandidate(id);
    if (!c) throw ValidationException("No candidate with ID '" + id + "'.");
    const std::string realId = c->id();
    candidates_.erase(std::remove_if(candidates_.begin(), candidates_.end(),
                                     [&](const Candidate& x) { return x.id() == realId; }),
                      candidates_.end());
    files_.saveCandidates(candidates_);
    log("CANDIDATE_REMOVED", realId);
}

std::vector<Candidate> VotingSystem::searchCandidates(const std::string& keyword) const {
    std::vector<Candidate> found;
    std::copy_if(candidates_.begin(), candidates_.end(), std::back_inserter(found),
                 [&](const Candidate& c) { return c.matches(keyword); });  // linear search
    std::sort(found.begin(), found.end(), [](const Candidate& a, const Candidate& b) {
        return Utils::toLower(a.name()) < Utils::toLower(b.name());
    });
    return found;
}

// ============================================================ election control
void VotingSystem::startElection() {
    if (election_.state() != ElectionState::NotStarted)
        throw ElectionStateException("Cannot start: the election is already " + election_.stateName() + ".");
    if (candidates_.size() < 2) throw ValidationException("At least 2 candidates are required to start the election.");
    if (voters_.empty()) throw ValidationException("At least 1 registered voter is required to start the election.");
    election_.start();
    files_.saveElection(election_);
    log("ELECTION_STARTED", std::to_string(voters_.size()) + " voters, " + std::to_string(candidates_.size()) + " candidates");
}

void VotingSystem::stopElection() {
    election_.stop();
    // Shuffle ballots so the order in votes.txt reveals nothing about who voted when.
    std::shuffle(votes_.begin(), votes_.end(), rng_);
    files_.saveVotes(votes_);
    files_.saveElection(election_);
    log("ELECTION_STOPPED", std::to_string(votes_.size()) + " votes recorded");
}

Statistics VotingSystem::statistics() const {
    Statistics s{};
    s.registered = voters_.size();
    s.votesCast = votes_.size();
    s.notCast = s.registered > s.votesCast ? s.registered - s.votesCast : 0;
    s.turnoutPercent = s.registered ? 100.0 * static_cast<double>(s.votesCast) / static_cast<double>(s.registered) : 0.0;
    return s;
}

// ============================================================ voting
// Rule check shared by the UI (to give early, friendly messages) and castVote().
void VotingSystem::checkCanVote(const Voter& voter) const {
    if (election_.state() == ElectionState::NotStarted)
        throw ElectionStateException("Voting has not started yet. Please come back when the election is OPEN.");
    if (election_.state() == ElectionState::Closed)
        throw ElectionStateException("Voting is closed. The election has ended.");
    if (voter.hasVoted())
        throw DuplicateVoteException("You have already voted. Each voter may vote only once.");
}

std::string VotingSystem::generateReceipt() {
    static const char* hex = "0123456789ABCDEF";
    std::string day = Utils::dateStamp();
    day.erase(std::remove(day.begin(), day.end(), '-'), day.end());  // 20250131
    while (true) {
        std::string code;
        for (int i = 0; i < 8; ++i) code += hex[rng_() % 16];
        const std::string receipt = "RCP-" + day + "-" + code;
        if (!findVote(receipt)) return receipt;  // regenerate on the (very unlikely) collision
    }
}

std::string VotingSystem::castVote(Voter& voter, const std::string& candidateId) {
    checkCanVote(voter);  // 1) election OPEN?  2) not voted yet?
    const Candidate* candidate = findCandidate(candidateId);
    if (!candidate) throw ValidationException("Invalid candidate. Please choose an ID from the list.");

    Vote vote(generateReceipt(), candidate->id(), Utils::dateStamp());
    files_.appendVote(vote);          // 3) save the ballot permanently FIRST ...
    votes_.push_back(vote);
    voter.markVoted();                // 4) ... then mark the voter as done
    files_.saveVoters(voters_);
    log("VOTE_CAST", "voter " + voter.id());  // the log never records the choice or the receipt
    return vote.receipt();
}

const Vote* VotingSystem::findVote(const std::string& receipt) const {
    const std::string key = Utils::toUpper(Utils::trim(receipt));
    for (const auto& v : votes_)
        if (v.receipt() == key) return &v;
    return nullptr;
}

// ============================================================ results
std::vector<ResultRow> VotingSystem::results() const {
    if (election_.state() != ElectionState::Closed)
        throw ElectionStateException("Results are available only after the election is CLOSED (now " +
                                     election_.stateName() + ").");
    std::map<std::string, int> tally;  // candidateId -> number of votes
    int total = 0;
    for (const auto& v : votes_) {
        if (findCandidate(v.candidateId())) { ++tally[v.candidateId()]; ++total; }
    }
    std::vector<ResultRow> rows;
    for (const auto& c : candidates_) {
        const int n = tally.count(c.id()) ? tally[c.id()] : 0;
        rows.push_back({c, n, total ? 100.0 * n / total : 0.0});
    }
    std::sort(rows.begin(), rows.end(), [](const ResultRow& a, const ResultRow& b) {
        if (a.votes != b.votes) return a.votes > b.votes;  // most votes first
        return a.candidate.name() < b.candidate.name();
    });
    return rows;
}

std::string VotingSystem::winnerSummary(const std::vector<ResultRow>& rows) const {
    if (rows.empty() || rows.front().votes == 0) return "No votes were cast - there is no winner.";
    const int top = rows.front().votes;
    std::vector<const ResultRow*> leaders;
    for (const auto& r : rows)
        if (r.votes == top) leaders.push_back(&r);
    if (leaders.size() == 1)
        return leaders[0]->candidate.name() + " (" + leaders[0]->candidate.party() + ") with " +
               std::to_string(top) + " vote(s)";
    std::string text = "TIE between ";
    for (std::size_t i = 0; i < leaders.size(); ++i)
        text += (i ? ", " : "") + leaders[i]->candidate.name();
    return text + " - " + std::to_string(top) + " vote(s) each";
}

// ============================================================ system tools
void VotingSystem::resetSimulation() {
    election_.reset();
    votes_.clear();
    for (auto& kv : voters_) kv.second.resetVoteStatus();
    files_.saveVotes(votes_);
    files_.saveVoters(voters_);
    files_.saveElection(election_);
    log("SIMULATION_RESET", "votes cleared, election set to NOT STARTED");
}

void VotingSystem::restoreDemoData() {
    seedDemoData();
    election_.reset();
    votes_.clear();
    files_.saveVoters(voters_);
    files_.saveCandidates(candidates_);
    files_.saveVotes(votes_);
    files_.saveElection(election_);
    log("DEMO_DATA_RESTORED", "10 demo voters and 4 demo candidates");
}

// Cross-checks the data files. Returns a list of problems (empty list = healthy).
std::vector<std::string> VotingSystem::verifyIntegrity() const {
    std::vector<std::string> issues;
    std::set<std::string> receipts;
    for (const auto& v : votes_) {
        if (!receipts.insert(v.receipt()).second) issues.push_back("Duplicate receipt number: " + v.receipt());
        if (!findCandidate(v.candidateId())) issues.push_back("Vote " + v.receipt() + " refers to unknown candidate " + v.candidateId());
    }
    std::size_t flagged = 0;
    for (const auto& kv : voters_)
        if (kv.second.hasVoted()) ++flagged;
    if (flagged != votes_.size())
        issues.push_back("Voters marked as voted (" + std::to_string(flagged) + ") != ballots stored (" +
                         std::to_string(votes_.size()) + ")");
    if (votes_.size() > voters_.size()) issues.push_back("More ballots than registered voters");
    if (!votes_.empty() && election_.state() == ElectionState::NotStarted)
        issues.push_back("Ballots exist although the election is NOT STARTED");
    return issues;
}

std::vector<std::string> VotingSystem::auditTail(std::size_t count) const { return files_.readAuditTail(count); }
