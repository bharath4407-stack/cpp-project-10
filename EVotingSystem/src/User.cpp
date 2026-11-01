// User.cpp - Shared behaviour of all accounts.
#include "User.h"
#include "Utils.h"

User::User(std::string id, std::string name, std::string passwordHash)
    : id_(std::move(id)), name_(std::move(name)), passwordHash_(std::move(passwordHash)) {}

bool User::authenticate(const std::string& password) const {
    return Utils::hashPassword(id_, password) == passwordHash_;
}

void User::setPassword(const std::string& newPassword) {
    passwordHash_ = Utils::hashPassword(id_, newPassword);
}
