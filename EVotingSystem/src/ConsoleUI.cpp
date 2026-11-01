// ConsoleUI.cpp - Menus and screens. All rules live in VotingSystem; this file only
// asks questions, validates typing, and prints results.
#include "ConsoleUI.h"
#include "Exceptions.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace U = Utils;

namespace {
std::string percentText(double p) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << p << "%";
    return out.str();
}

std::string coloredState(const Election& e) {
    const char* color = e.state() == ElectionState::Open     ? U::Color::Green
                        : e.state() == ElectionState::Closed ? U::Color::Red
                                                             : U::Color::Yellow;
    return U::paint(e.stateName(), color);
}
}  // namespace

// ============================================================ run loop
void ConsoleUI::attempt(const std::function<void()>& action) {
    try {
        action();
    } catch (const VotingException& e) {  // any expected error -> friendly message
        U::error(e.what());
    }
}

std::string ConsoleUI::statusLine() const { return "Election status: " + coloredState(sys_.election()); }

void ConsoleUI::run() {
    if (!sys_.startupWarnings().empty()) {
        U::printHeader("START-UP WARNINGS");
        for (const auto& w : sys_.startupWarnings()) U::warn(w);
        U::pause();
    }
    bool running = true;
    while (running) {
        showMainMenu();
        switch (U::readInt("  Enter your choice (1-5): ", 1, 5)) {
            case 1: attempt([&] { voterLoginFlow(); }); break;
            case 2: attempt([&] { registrationFlow("VOTER REGISTRATION"); U::pause(); }); break;
            case 3: attempt([&] { adminLoginFlow(); }); break;
            case 4: attempt([&] { showElectionInfo(); U::pause(); }); break;
            case 5: running = false; break;
        }
    }
    std::cout << "\n  Thank you for using the E-Voting System (Local Simulation). Goodbye!\n\n";
}

void ConsoleUI::showMainMenu() const {
    U::clearScreen();
    U::printHeader("E-VOTING SYSTEM", "LOCAL SIMULATION - for education only");
    std::cout << "  " << statusLine() << "\n";
    U::printLine('-');
    std::cout << "  1. Voter Login\n"
                 "  2. Voter Registration\n"
                 "  3. Admin Login\n"
                 "  4. Election Information\n"
                 "  5. Exit\n";
    U::printLine('-');
}

// ============================================================ public screens
void ConsoleUI::voterLoginFlow() {
    U::printHeader("VOTER LOGIN");
    std::cout << "  (Leave the ID empty to go back.)\n";
    const int maxAttempts = 3;
    for (int attempts = 1; attempts <= maxAttempts; ++attempts) {
        const std::string id = U::readLine("  Voter ID : ");
        if (id.empty()) return;
        const std::string password = U::readLine("  Password : ");
        try {
            Voter& voter = sys_.loginVoter(id, password);
            U::success("Welcome, " + voter.name() + "!");
            voterMenu(voter);
            return;
        } catch (const AuthenticationException& e) {
            U::error(e.what() + std::string(" (attempt ") + std::to_string(attempts) + " of " +
                     std::to_string(maxAttempts) + ")");
        }
    }
    U::error("Too many failed attempts. Returning to the main menu.");
    U::pause();
}

void ConsoleUI::adminLoginFlow() {
    U::printHeader("ADMIN LOGIN");
    std::cout << "  (Leave the ID empty to go back.)\n";
    const int maxAttempts = 3;
    for (int attempts = 1; attempts <= maxAttempts; ++attempts) {
        const std::string id = U::readLine("  Admin ID : ");
        if (id.empty()) return;
        const std::string password = U::readLine("  Password : ");
        try {
            Admin& admin = sys_.loginAdmin(id, password);
            U::success("Welcome, " + admin.name() + "!");
            adminMenu(admin);
            return;
        } catch (const AuthenticationException& e) {
            U::error(e.what() + std::string(" (attempt ") + std::to_string(attempts) + " of " +
                     std::to_string(maxAttempts) + ")");
        }
    }
    U::error("Too many failed attempts. Returning to the main menu.");
    U::pause();
}

// Used both for self-registration and for "Add voter" by the admin.
void ConsoleUI::registrationFlow(const std::string& title) {
    U::printHeader(title);
    if (sys_.election().state() == ElectionState::Closed)
        throw ElectionStateException("Registration is closed: the election has ended.");
    std::cout << "  Demo only - do NOT enter real personal information.\n\n";

    const std::string name = U::readValidated("  Full name            : ", U::isValidName,
                                              "2-40 characters; letters, spaces, . - ' only.");
    const int age = U::readInt("  Age (18-120)         : ", 18, 120);

    std::string password;
    while (true) {
        password = U::readValidated("  Password (6-32 chars): ", U::isValidPassword,
                                    "Password must be 6-32 printable characters.");
        if (U::readLine("  Confirm password     : ") == password) break;
        U::error("Passwords do not match. Try again.");
    }
    const std::string id = U::readValidated(
        "  Voter ID (Enter = auto-generate, or choose 4-12 letters/digits): ",
        [&](const std::string& s) { return s.empty() || (U::isValidId(s) && !sys_.voterExists(s)); },
        "ID must be 4-12 letters/digits and not already in use.");

    const Voter& voter = sys_.registerVoter(name, age, password, id);
    U::success("Registration complete!");
    std::cout << "\n  Your Voter ID is: " << U::paint(voter.id(), std::string(U::Color::Bold) + U::Color::Green)
              << "   (remember it - you need it to log in)\n";
}

void ConsoleUI::showElectionInfo() {
    const Election& e = sys_.election();
    const Statistics s = sys_.statistics();
    U::printHeader("ELECTION INFORMATION");
    std::cout << "  Title              : " << e.title() << "\n"
              << "  Status             : " << coloredState(e) << "\n"
              << "  Started at         : " << e.startTime() << "\n"
              << "  Ended at           : " << e.endTime() << "\n"
              << "  Candidates         : " << sys_.candidates().size() << "\n"
              << "  Registered voters  : " << s.registered << "\n";
    if (e.state() != ElectionState::NotStarted)
        std::cout << "  Votes cast         : " << s.votesCast << "\n"
                  << "  Voter turnout      : " << percentText(s.turnoutPercent) << "\n";
    U::printLine('-');
    std::cout << "  Rules: one vote per voter | voters must log in | voting only while OPEN\n"
                 "         results are shown only after the election is CLOSED\n";
}

// ============================================================ voter area
void ConsoleUI::showProfile(const User& user) const { user.displayProfile(); }  // virtual call

void ConsoleUI::voterMenu(Voter& voter) {
    bool loggedIn = true;
    while (loggedIn) {
        U::clearScreen();
        U::printHeader("VOTER DASHBOARD", "Logged in as: " + voter.name() + " (" + voter.id() + ")");
        std::cout << "  " << statusLine() << "\n";
        U::printLine('-');
        std::cout << "  1. View my profile\n"
                     "  2. View candidates\n"
                     "  3. Cast my vote\n"
                     "  4. Verify a vote receipt\n"
                     "  5. Election information\n"
                     "  6. Logout\n";
        U::printLine('-');
        const int choice = U::readInt("  Enter your choice (1-6): ", 1, 6);
        if (choice == 6) {
            sys_.log("LOGOUT", "voter " + voter.id());
            U::success("You have been logged out.");
            U::pause();
            loggedIn = false;
            continue;
        }
        attempt([&] {
            switch (choice) {
                case 1: showProfile(voter); break;
                case 2: viewCandidatesFlow(); break;
                case 3: castVoteFlow(voter); break;
                case 4: verifyReceiptFlow(); break;
                case 5: showElectionInfo(); break;
            }
        });
        U::pause();
    }
}

void ConsoleUI::castVoteFlow(Voter& voter) {
    U::printHeader("CAST YOUR VOTE");
    sys_.checkCanVote(voter);  // throws a clear message if closed / not started / already voted
    showCandidateTable(sys_.candidates());

    while (true) {
        const std::string input = U::readLine("\n  Enter Candidate ID to vote for (0 = cancel): ");
        if (input == "0") {
            U::info("Cancelled. No vote was recorded.");
            return;
        }
        const Candidate* candidate = sys_.findCandidate(input);
        if (!candidate) {
            U::error("Invalid candidate ID. Please choose an ID from the list above.");
            continue;
        }
        candidate->displayCard();
        if (!U::confirm("  Confirm your vote for " + candidate->name() + " (" + candidate->party() +
                        ")?  This CANNOT be changed. (y/n): ")) {
            U::info("Vote not submitted. You can choose again or enter 0 to cancel.");
            continue;
        }
        const std::string receipt = sys_.castVote(voter, candidate->id());
        std::cout << "\n" << U::paint("  +" + U::repeat("-", 52) + "+", U::Color::Green) << "\n"
                  << U::paint("  |  VOTE RECORDED SUCCESSFULLY" + std::string(24, ' ') + "|", U::Color::Green) << "\n"
                  << U::paint("  |  Receipt No: " + U::padRight(receipt, 38) + "|", U::Color::Green) << "\n"
                  << U::paint("  +" + U::repeat("-", 52) + "+", U::Color::Green) << "\n";
        U::info("Write this receipt down. It is shown only once and is not linked to your choice.");
        return;
    }
}

void ConsoleUI::verifyReceiptFlow() {
    U::printHeader("VERIFY A VOTE RECEIPT");
    const std::string receipt = U::readLine("  Receipt number: ");
    const Vote* vote = sys_.findVote(receipt);
    if (vote) {
        U::success("Receipt is VALID. A ballot with this number was recorded on " + vote->date() + ".");
        U::info("For ballot secrecy, the choice made is never displayed.");
    } else {
        U::error("No ballot found with that receipt number.");
    }
}

// ============================================================ candidates (shared by voter + admin)
void ConsoleUI::showCandidateTable(const std::vector<Candidate>& list) const {
    if (list.empty()) {
        U::info("No candidates to show.");
        return;
    }
    std::cout << "\n  " << Candidate::tableHeader() << "\n  " << U::repeat("-", U::kWidth - 4) << "\n";
    for (const auto& c : list) std::cout << "  " << c.tableRow() << "\n";
}

void ConsoleUI::viewCandidatesFlow() {
    U::printHeader("CANDIDATES");
    std::cout << "  Sort by:  1. ID   2. Name   3. Party\n";
    const int order = U::readInt("  Choose (1-3): ", 1, 3);
    std::vector<Candidate> list = sys_.candidates();
    if (order == 2)
        std::sort(list.begin(), list.end(), [](const Candidate& a, const Candidate& b) {
            return U::toLower(a.name()) < U::toLower(b.name());
        });
    else if (order == 3)
        std::sort(list.begin(), list.end(), [](const Candidate& a, const Candidate& b) {
            return U::toLower(a.party()) < U::toLower(b.party());
        });
    showCandidateTable(list);
    while (!list.empty()) {
        const std::string id = U::readLine("\n  Enter a Candidate ID to read the full manifesto (Enter = back): ");
        if (id.empty()) return;
        const Candidate* c = sys_.findCandidate(id);
        if (c) c->displayCard();
        else U::error("No candidate with that ID.");
    }
}

// ============================================================ admin area
void ConsoleUI::adminMenu(Admin& admin) {
    bool loggedIn = true;
    while (loggedIn) {
        U::clearScreen();
        U::printHeader("ADMIN DASHBOARD", "Logged in as: " + admin.name());
        std::cout << "  " << statusLine() << "\n";
        U::printLine('-');
        std::cout << "  ELECTION CONTROL\n"
                     "     1. Election status & statistics\n"
                     "     2. Start election\n"
                     "     3. Stop election\n"
                     "  VOTERS\n"
                     "     4. View / search registered voters\n"
                     "     5. Add voter\n"
                     "     6. Remove voter\n"
                     "  CANDIDATES\n"
                     "     7. View candidates\n"
                     "     8. Add candidate\n"
                     "     9. Edit candidate\n"
                     "    10. Remove candidate\n"
                     "    11. Search candidates\n"
                     "  RESULTS\n"
                     "    12. View final results & winner\n"
                     "  SYSTEM TOOLS\n"
                     "    13. View audit log\n"
                     "    14. Verify data integrity\n"
                     "    15. Account overview (all users)\n"
                     "    16. Change admin password\n"
                     "    17. Reset simulation\n"
                     "    18. Restore demo data\n"
                     "    19. Logout\n";
        U::printLine('-');
        const int choice = U::readInt("  Enter your choice (1-19): ", 1, 19);
        if (choice == 19) {
            sys_.log("LOGOUT", "admin " + admin.id());
            U::success("You have been logged out.");
            U::pause();
            loggedIn = false;
            continue;
        }
        attempt([&] {
            switch (choice) {
                case 1: showElectionInfo(); break;
                case 2: startElectionFlow(); break;
                case 3: stopElectionFlow(); break;
                case 4: listVotersFlow(); break;
                case 5: registrationFlow("ADD VOTER"); break;
                case 6: removeVoterFlow(); break;
                case 7: viewCandidatesFlow(); break;
                case 8: addCandidateFlow(); break;
                case 9: editCandidateFlow(); break;
                case 10: removeCandidateFlow(); break;
                case 11: searchCandidatesFlow(); break;
                case 12: showResults(); break;
                case 13: showAuditLog(); break;
                case 14: showIntegrityReport(); break;
                case 15: showAccountOverview(); break;
                case 16: changePasswordFlow(admin); break;
                case 17: resetFlow(); break;
                case 18: restoreFlow(); break;
            }
        });
        U::pause();
    }
}

void ConsoleUI::startElectionFlow() {
    U::printHeader("START ELECTION");
    if (!U::confirm("  Open voting now? Candidates can no longer be changed afterwards. (y/n): ")) {
        U::info("Cancelled.");
        return;
    }
    sys_.startElection();
    U::success("The election is now OPEN. Voters can cast their votes.");
}

void ConsoleUI::stopElectionFlow() {
    U::printHeader("STOP ELECTION");
    const Statistics s = sys_.statistics();
    std::cout << "  Votes cast so far: " << s.votesCast << " of " << s.registered << " voters\n";
    if (!U::confirm("  Close voting permanently? (y/n): ")) {
        U::info("Cancelled.");
        return;
    }
    sys_.stopElection();
    U::success("The election is now CLOSED. Results are available.");
}

void ConsoleUI::listVotersFlow() {
    U::printHeader("REGISTERED VOTERS");
    std::cout << "  1. Sort by Voter ID\n  2. Sort by name\n  3. Sort by voting status\n  4. Search by name or ID\n";
    const int choice = U::readInt("  Choose (1-4): ", 1, 4);
    std::vector<Voter> list;
    if (choice == 4) {
        const std::string keyword = U::readValidated("  Search text: ",
            [](const std::string& s) { return !s.empty(); }, "Please type something to search for.");
        list = sys_.searchVoters(keyword);
    } else {
        list = sys_.sortedVoters(choice == 1 ? VoterSort::ById : choice == 2 ? VoterSort::ByName : VoterSort::ByStatus);
    }
    if (list.empty()) {
        U::info("No matching voters.");
        return;
    }
    std::cout << "\n  " << Voter::tableHeader() << "\n  " << U::repeat("-", U::kWidth - 4) << "\n";
    for (const auto& v : list) std::cout << "  " << v.tableRow() << "\n";
    const Statistics s = sys_.statistics();
    std::cout << "\n  Shown: " << list.size() << " | Total registered: " << s.registered << "\n";
}

void ConsoleUI::removeVoterFlow() {
    U::printHeader("REMOVE VOTER");
    const std::string id = U::readLine("  Voter ID to remove (Enter = cancel): ");
    if (id.empty()) return;
    if (!sys_.voterExists(id)) throw ValidationException("No voter with ID '" + U::toUpper(id) + "'.");
    if (!U::confirm("  Really remove voter " + U::toUpper(id) + "? (y/n): ")) {
        U::info("Cancelled.");
        return;
    }
    sys_.removeVoter(id);
    U::success("Voter removed.");
}

void ConsoleUI::addCandidateFlow() {
    U::printHeader("ADD CANDIDATE");
    const std::string name = U::readValidated("  Name (2-40)       : ", U::isValidName, "Letters, spaces, . - ' only (2-40).");
    const std::string party = U::readValidated("  Party/group (2-40): ",
        [](const std::string& s) { return U::isValidText(s, 2, 40); }, "2-40 printable characters, no '|'.");
    const std::string symbol = U::readValidated("  Symbol (2-20)     : ",
        [](const std::string& s) { return U::isValidText(s, 2, 20); }, "2-20 printable characters, no '|'.");
    const int age = U::readInt("  Age (25-100)      : ", 25, 100);
    const std::string manifesto = U::readValidated("  Manifesto (5-120) : ",
        [](const std::string& s) { return U::isValidText(s, 5, 120); }, "5-120 printable characters, no '|'.");
    const Candidate& c = sys_.addCandidate(name, party, symbol, age, manifesto);
    U::success("Candidate added with ID " + c.id() + ".");
}

void ConsoleUI::editCandidateFlow() {
    U::printHeader("EDIT CANDIDATE");
    showCandidateTable(sys_.candidates());
    const std::string input = U::readLine("\n  Candidate ID to edit (Enter = cancel): ");
    if (input.empty()) return;
    const Candidate* c = sys_.findCandidate(input);
    if (!c) throw ValidationException("No candidate with ID '" + input + "'.");
    const std::string id = c->id(), oldName = c->name(), oldParty = c->party(), oldSymbol = c->symbol(),
                      oldManifesto = c->manifesto();
    const int oldAge = c->age();
    c->displayCard();
    U::info("Press Enter to keep the current value.");

    // Helper: empty input keeps the old value, otherwise the input must be valid.
    auto ask = [](const std::string& label, const std::string& current,
                  const std::function<bool(const std::string&)>& valid, const std::string& err) {
        while (true) {
            const std::string v = U::readLine("  " + label + " [" + current + "]: ");
            if (v.empty()) return current;
            if (valid(v)) return v;
            U::error(err);
        }
    };
    const std::string name = ask("Name", oldName, U::isValidName, "Letters, spaces, . - ' only (2-40).");
    const std::string party = ask("Party", oldParty, [](const std::string& s) { return U::isValidText(s, 2, 40); }, "2-40 printable characters.");
    const std::string symbol = ask("Symbol", oldSymbol, [](const std::string& s) { return U::isValidText(s, 2, 20); }, "2-20 printable characters.");
    const std::string ageText = ask("Age", std::to_string(oldAge),
        [](const std::string& s) { int a = 0; return U::tryParseInt(s, a) && a >= 25 && a <= 100; }, "Age must be a number from 25 to 100.");
    const std::string manifesto = ask("Manifesto", oldManifesto, [](const std::string& s) { return U::isValidText(s, 5, 120); }, "5-120 printable characters.");

    sys_.editCandidate(id, name, party, symbol, std::stoi(ageText), manifesto);
    U::success("Candidate " + id + " updated.");
}

void ConsoleUI::removeCandidateFlow() {
    U::printHeader("REMOVE CANDIDATE");
    showCandidateTable(sys_.candidates());
    const std::string input = U::readLine("\n  Candidate ID to remove (Enter = cancel): ");
    if (input.empty()) return;
    const Candidate* c = sys_.findCandidate(input);
    if (!c) throw ValidationException("No candidate with ID '" + input + "'.");
    const std::string id = c->id();
    c->displayCard();
    if (!U::confirm("  Really remove this candidate? (y/n): ")) {
        U::info("Cancelled.");
        return;
    }
    sys_.removeCandidate(id);
    U::success("Candidate removed.");
}

void ConsoleUI::searchCandidatesFlow() {
    U::printHeader("SEARCH CANDIDATES");
    const std::string keyword = U::readValidated("  Search (ID, name, party or symbol): ",
        [](const std::string& s) { return !s.empty(); }, "Please type something to search for.");
    const auto found = sys_.searchCandidates(keyword);
    if (found.empty()) {
        U::info("No candidate matches '" + keyword + "'.");
        return;
    }
    showCandidateTable(found);
    std::cout << "\n  " << found.size() << " match(es) found.\n";
}

// ============================================================ results
void ConsoleUI::showResults() {
    const auto rows = sys_.results();  // throws unless the election is CLOSED
    const Statistics s = sys_.statistics();

    U::printHeader("FINAL ELECTION RESULTS", sys_.election().title());
    std::cout << "  " << U::padRight("Candidate Name", 22) << " | " << U::padRight("Party", 22) << " | Votes | Percentage\n"
              << "  " << U::repeat("-", U::kWidth - 4) << "\n";
    for (const auto& r : rows)
        std::cout << "  " << U::padRight(r.candidate.name(), 22) << " | " << U::padRight(r.candidate.party(), 22)
                  << " | " << U::padLeft(std::to_string(r.votes), 5) << " | " << U::padLeft(percentText(r.percentage), 10) << "\n";

    std::cout << "\n  Total registered voters : " << s.registered
              << "\n  Total votes cast        : " << s.votesCast
              << "\n  Votes not cast          : " << s.notCast
              << "\n  Voter turnout           : " << percentText(s.turnoutPercent) << "\n";

    std::cout << "\n  " << U::paint("WINNER: " + sys_.winnerSummary(rows), std::string(U::Color::Bold) + U::Color::Green) << "\n";

    U::printHeader("VOTES BAR CHART");
    printBarChart(rows);
}

void ConsoleUI::printBarChart(const std::vector<ResultRow>& rows) const {
    const int maxBar = 30;
    int maxVotes = 0;
    for (const auto& r : rows) maxVotes = std::max(maxVotes, r.votes);
    const std::string block = U::unicodeEnabled() ? "\xE2\x96\x88" : "#";  // UTF-8 for the full-block character
    for (std::size_t i = 0; i < rows.size(); ++i) {
        int length = maxVotes ? static_cast<int>(std::lround(static_cast<double>(rows[i].votes) * maxBar / maxVotes)) : 0;
        if (rows[i].votes > 0 && length == 0) length = 1;
        const char* color = (i == 0 && rows[i].votes > 0) ? U::Color::Green : U::Color::Cyan;
        std::cout << "  " << U::padRight(rows[i].candidate.name(), 22) << " "
                  << U::paint(U::repeat(block, length), color) << std::string(maxBar - length, ' ') << " "
                  << rows[i].votes << "\n";
    }
}

// ============================================================ system tools
void ConsoleUI::showAuditLog() {
    U::printHeader("AUDIT LOG", "last 25 events");
    const auto lines = sys_.auditTail(25);
    if (lines.empty()) {
        U::info("The audit log is empty.");
        return;
    }
    for (const auto& line : lines) {
        const auto f = U::split(line, '|');
        if (f.size() == 3)
            std::cout << "  " << f[0] << "  " << U::padRight(f[1], 22) << f[2] << "\n";
    }
}

void ConsoleUI::showIntegrityReport() {
    U::printHeader("DATA INTEGRITY CHECK");
    const auto issues = sys_.verifyIntegrity();
    const Statistics s = sys_.statistics();
    std::cout << "  Registered voters: " << s.registered << " | Ballots stored: " << s.votesCast << "\n\n";
    if (issues.empty()) {
        U::success("No problems found. Ballots and voter records are consistent.");
    } else {
        for (const auto& i : issues) U::warn(i);
    }
}

void ConsoleUI::showAccountOverview() {
    U::printHeader("ACCOUNT OVERVIEW", "Admin and voters handled through User pointers");
    std::cout << "  " << U::padRight("Role", 15) << " | " << U::padRight("ID", 10) << " | Name\n  "
              << U::repeat("-", U::kWidth - 4) << "\n";
    for (const User* account : sys_.allAccounts())  // polymorphism: role() is resolved at run time
        std::cout << "  " << U::padRight(account->role(), 15) << " | " << U::padRight(account->id(), 10)
                  << " | " << account->name() << "\n";
}

void ConsoleUI::changePasswordFlow(Admin& admin) {
    U::printHeader("CHANGE ADMIN PASSWORD");
    (void)admin;
    const std::string oldPassword = U::readLine("  Current password : ");
    const std::string newPassword = U::readValidated("  New password     : ", U::isValidPassword,
                                                     "Password must be 6-32 printable characters.");
    if (U::readLine("  Confirm password : ") != newPassword) throw ValidationException("Passwords do not match.");
    sys_.changeAdminPassword(oldPassword, newPassword);
    U::success("Password changed.");
}

void ConsoleUI::resetFlow() {
    U::printHeader("RESET SIMULATION");
    std::cout << "  This deletes ALL votes and sets the election back to NOT STARTED.\n"
                 "  Voters and candidates are kept.\n";
    if (U::readLine("  Type RESET to confirm (anything else cancels): ") != "RESET") {
        U::info("Cancelled. Nothing was changed.");
        return;
    }
    sys_.resetSimulation();
    U::success("Simulation reset. You can start a new election.");
}

void ConsoleUI::restoreFlow() {
    U::printHeader("RESTORE DEMO DATA");
    std::cout << "  This replaces ALL voters and candidates with the original demo data,\n"
                 "  deletes all votes and resets the election.\n";
    if (U::readLine("  Type RESTORE to confirm (anything else cancels): ") != "RESTORE") {
        U::info("Cancelled. Nothing was changed.");
        return;
    }
    sys_.restoreDemoData();
    U::success("Demo data restored (voters V1001-V1010, password 'pass123').");
}
