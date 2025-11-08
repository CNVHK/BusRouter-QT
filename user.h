#ifndef USER_H
#define USER_H

#include <QString>

struct User {
    QString name;
    QString role; // "admin" or "user"

    bool isLoggedIn() const { return !name.isEmpty(); }
    bool isAdmin() const { return role == "admin"; }
};

#endif // USER_H
