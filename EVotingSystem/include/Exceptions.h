// Exceptions.h - Custom exception hierarchy for the E-Voting System.
// Every "expected" failure (bad input, wrong password, closed election...) is
// reported with a dedicated exception type so the UI layer can show a clear message.
#pragma once
#include <stdexcept>
#include <string>

// Base class for all application-level errors.
class VotingException : public std::runtime_error {
public:
    explicit VotingException(const std::string& message) : std::runtime_error(message) {}
};

// Invalid user data (bad name, age out of range, unknown candidate ...).
class ValidationException : public VotingException {
public:
    explicit ValidationException(const std::string& m) : VotingException(m) {}
};

// Wrong ID / password.
class AuthenticationException : public VotingException {
public:
    explicit AuthenticationException(const std::string& m) : VotingException(m) {}
};

// Action not allowed in the current election state (e.g. voting while CLOSED).
class ElectionStateException : public VotingException {
public:
    explicit ElectionStateException(const std::string& m) : VotingException(m) {}
};

// A voter tried to vote twice.
class DuplicateVoteException : public VotingException {
public:
    explicit DuplicateVoteException(const std::string& m) : VotingException(m) {}
};

// File could not be read/written.
class FileException : public VotingException {
public:
    explicit FileException(const std::string& m) : VotingException(m) {}
};

// Standard input was closed (Ctrl+D / Ctrl+Z / end of piped input).
// Deliberately NOT a VotingException so it travels all the way up to main().
class InputClosedException : public std::runtime_error {
public:
    explicit InputClosedException(const std::string& m) : std::runtime_error(m) {}
};
