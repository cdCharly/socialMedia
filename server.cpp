//
// Created by Charly CATIN--RICO on 13/05/2026.
//
#include <iostream>
#include <sqlite3.h>





int main() {
    sqlite3* userDb;
    sqlite3* messagesDb;
    char** errMsgUserDb = nullptr;
    char** errMsgMessagesDb = nullptr;

    // ouvrir/créer le fichier user
    int initUser = sqlite3_open("user.db", &userDb);
    int initMessages = sqlite3_open("messages.db", &messagesDb);

    if (initUser != SQLITE_OK) {
        // Err
        std::cerr << "Erreur à l'ouverture de la BD User : " << sqlite3_errmsg(userDb) << std::endl;
        return 1;
    } else {
        // ok
        std::cout << "Base de données User ouverte avec succès !" << std::endl;
    }

    if (initMessages != SQLITE_OK) {
        // Err
        std::cerr << "Erreur à l'ouverture de la BD Messages : " << sqlite3_errmsg(messagesDb) << std::endl;
        return 1;
    } else {
        // ok
        std::cout << "Base de données Messages ouverte avec succès !" << std::endl;
    }

    // activer les cles etrangeres
    sqlite3_exec(messagesDb, "PRAGMA foreign_keys = ON", nullptr, nullptr, errMsgMessagesDb);

    // créer les tables des bases
    sqlite3_exec(userDb,"CREATE TABLE User (isUser INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, lastName TEXT, password TEXT)",
        nullptr, nullptr, errMsgUserDb);
    sqlite3_exec(messagesDb,"CREATE TABLE Messages (idMessage INTEGER PRIMARY KEY AUTOINCREMENT, contenu TEXT, date DATETIME CURRENT_TIMESTAMP, userID INTEGER, FOREIGN KEY(userID) REFERENCES User(id)",
        nullptr, nullptr, errMsgMessagesDb);



    // fermer la db
    sqlite3_close(userDb);
    sqlite3_close(messagesDb);

    return 0;
}