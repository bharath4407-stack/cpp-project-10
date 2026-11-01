// ConsoleUI.h - All menus and screens. Talks to VotingSystem, never to files directly.
#pragma once
#include "VotingSystem.h"
#include <functional>
#include <string>
#include <vector>

class ConsoleUI {
public:
    explicit ConsoleUI(VotingSystem& system) : sys_(system) {}
    void run();

private:
    VotingSystem& sys_;

    // Runs an action and turns any VotingException into a friendly error message.
    void attempt(const std::function<void()>& action);

    // Main menu & public screens
    void showMainMenu() const;
    void voterLoginFlow();
    void adminLoginFlow();
    void registrationFlow(const std::string& title);
    void showElectionInfo();

    // Voter area
    void voterMenu(Voter& voter);
    void showProfile(const User& user) const;  // polymorphic: works for Voter and Admin
    void castVoteFlow(Voter& voter);
    void verifyReceiptFlow();

    // Admin area
    void adminMenu(Admin& admin);
    void listVotersFlow();
    void removeVoterFlow();
    void addCandidateFlow();
    void editCandidateFlow();
    void removeCandidateFlow();
    void searchCandidatesFlow();
    void showResults();
    void showAuditLog();
    void showIntegrityReport();
    void showAccountOverview();
    void changePasswordFlow(Admin& admin);
    void resetFlow();
    void restoreFlow();

    // Shared pieces
    void viewCandidatesFlow();
    void showCandidateTable(const std::vector<Candidate>& list) const;
    void printBarChart(const std::vector<ResultRow>& rows) const;
    void startElectionFlow();
    void stopElectionFlow();
    std::string statusLine() const;
};
