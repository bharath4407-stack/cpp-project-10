// Admin.h - The administrator account (inherits from User).
#pragma once
#include "User.h"

class Admin : public User {
public:
    Admin(std::string id, std::string name, std::string passwordHash);

    std::string role() const override { return "Administrator"; }
    void displayProfile() const override;
    std::string serialize() const override;

    static Admin deserialize(const std::string& line);
};
