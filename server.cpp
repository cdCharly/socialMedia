//
// Created by Charly CATIN--RICO on 13/05/2026.
//
#include <iostream>
#include <sqlite3.h>





int main() {
    sqlite3* db;
    char** errMsgUserDb = nullptr;
    char** errMsgMessagesDb = nullptr;

    // ouvrir/créer le fichier db
    int initdb = sqlite3_open("social.db", &db);

    if (initdb != SQLITE_OK) {
        // Err
        std::cerr << "Erreur à l'ouverture de la db : " << sqlite3_errmsg(db) << std::endl;
        return 1;
    } else {
        // ok
        std::cout << "Base de données ouverte avec succès !" << std::endl;
    }


    // activer les cles etrangeres
    sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, errMsgMessagesDb);

    // créer les tables des bases
    sqlite3_exec(db,"CREATE TABLE User (isUser INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, lastName TEXT, password TEXT)",
        nullptr, nullptr, errMsgUserDb);
    sqlite3_exec(db,"CREATE TABLE Messages (idMessage INTEGER PRIMARY KEY AUTOINCREMENT, contenu TEXT, date DATETIME CURRENT_TIMESTAMP, userID INTEGER, FOREIGN KEY(userID) REFERENCES User(id)",
        nullptr, nullptr, errMsgMessagesDb);



    // fermer la db
    sqlite3_close(db);

    return 0;
}