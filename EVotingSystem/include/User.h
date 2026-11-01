// User.h - Abstract base class for everyone who can log in (Voter, Admin).
#pragma once
#include <string>

class User {
public:
    User(std::string id, std::string name, std::string passwordHash);
    virtual ~User() = default;  // virtual destructor: required for polymorphic base classes

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }

    // Checks a plain-text password against the stored hash.
    bool authenticate(const std::string& password) const;
    void setPassword(const std::string& newPassword);

    // ---- Pure virtual functions: every subclass MUST override these (polymorphism) ----
    virtual std::string role() const = 0;
    virtual void displayProfile() const = 0;
    virtual std::string serialize() const = 0;  // one line for the data file

protected:  // visible to Voter / Admin, hidden from the rest of the program
    std::string id_;
    std::string name_;
    std::string passwordHash_;
};
