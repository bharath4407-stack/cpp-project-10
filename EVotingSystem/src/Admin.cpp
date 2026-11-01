// Admin.cpp
#include "Admin.h"
#include "Exceptions.h"
#include "Utils.h"

#include <iostream>

Admin::Admin(std::string id, std::string name, std::string passwordHash)
    : User(std::move(id), std::move(name), std::move(passwordHash)) {}

// Overrides User::displayProfile() - same call, different output (polymorphism)
void Admin::displayProfile() const {
    Utils::printHeader("ADMINISTRATOR PROFILE");
    std::cout << "  Admin ID : " << id_ << "\n"
              << "  Name     : " << name_ << "\n"
              << "  Role     : " << role() << "\n"
              << "  Access   : Full control of the simulation\n";
    Utils::printLine('=');
}

// File format: id|name|passwordHash
std::string Admin::serialize() const { return id_ + "|" + name_ + "|" + passwordHash_; }

Admin Admin::deserialize(const std::string& line) {
    const auto f = Utils::split(line, '|');
    if (f.size() != 3) throw ValidationException("admin record must have 3 fields");
    const std::string id = Utils::toLower(Utils::trim(f[0]));
    const std::string name = Utils::trim(f[1]);
    const std::string hash = Utils::trim(f[2]);
    if (!Utils::isValidId(id)) throw ValidationException("bad admin ID");
    if (!Utils::isValidName(name)) throw ValidationException("bad admin name");
    if (!Utils::isValidHash(hash)) throw ValidationException("bad admin password hash");
    return Admin(id, name, hash);
}
