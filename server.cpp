//
// Created by Charly CATIN--RICO on 13/05/2026.
//
#include <iostream>
#include <sqlite3.h>
using namespace std;


static int callback_affichage(void* data, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        // On affiche: NomDeLaColonne = Valeur
        cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << " | ";
    }
    cout << endl; // Saut de ligne pour le prochain utilisateur
    return 0;
}



// inserer dans une table
void insertIntoUserTable(sqlite3 *db){

    // variables
    string requete, name, lastname, password = "";
    char* errMsg = nullptr;

    // reception infos
    cout << "Ecrire user name" << endl;
    cin >>name;
    cout << "Ecrire lastname" << endl;
    cin >>lastname;
    cout << "Ecrire password" << endl;
    cin >>password;

    // creation requete + execution
    requete = "INSERT INTO User (name, lastName, password) "
              "VALUES ('" + name + "', '" + lastname + "', '" + password + "');";

    sqlite3_exec(db, requete.c_str(), nullptr, nullptr, &errMsg);

    // conclusion
    if (errMsg != nullptr) {
        cout << "probleme insertion user" << errMsg << endl;
        sqlite3_free(errMsg);
    }
    else {
        cout << "insertion finie" << endl;
        sqlite3_free(errMsg);
    }

    // delete le ptr pour vider la memoire
    //sqlite3_free(errMsg);
}


void insertIntoMessagesTable(sqlite3 *db) {

    // variables
    string requete, contenu, idUser = "";
    char*errMsg = nullptr;

    // reception infos
    cout << "Ecrire message" << endl;
    cin >> contenu;
    cout << "ecrire idUser (int)" << endl;
    cin >> idUser;

    // construction requete
    requete = "INSERT INTO Messages (contenu, userID) VALUES ('" + contenu +"', '" + idUser + "');";
    sqlite3_exec(db, requete.c_str(), callback_affichage, nullptr, &errMsg);

    // conclusion
    if (errMsg != nullptr) {
        cout << "probleme insertion messages " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    else {
        cout << "insertion finie " << endl;
        sqlite3_free(errMsg);
    }

}


void selectAllTable(sqlite3* db, const string tableName) {

    // variables
    char* errMsg = nullptr;

    // construction requete + execution
    string requete = "SELECT * FROM "+tableName;
    sqlite3_exec(db, requete.c_str(), callback_affichage, nullptr, &errMsg);

    // conclusion
    if (errMsg != nullptr) {
        cout << "probleme lecture " << errMsg << endl;
    }
    else {
        cout << "lecture finie " << endl;
    }

    // delete le ptr pour vider la memoire
    sqlite3_free(errMsg);
}

int main() {
    sqlite3* db;
    char* errMsgUserDb = nullptr;
    char* errMsgMessagesDb = nullptr;

    // ouvrir/créer le fichier db
    int initdb = sqlite3_open("social.db", &db);

    if (initdb != SQLITE_OK) {
        // Err
        std::cerr << "Erreur à l'ouverture de la db : " << sqlite3_errmsg(db) << std::endl;

    } else {
        // ok
        std::cout << "Base de données ouverte avec succès !" << std::endl;
    }


    // activer les cles etrangeres
    sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, &errMsgMessagesDb);

    // créer les tables des bases
    sqlite3_exec(db,"CREATE TABLE User (idUser INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "name TEXT, "
                    "lastName TEXT, "
                    "password TEXT)",
        nullptr, nullptr, &errMsgUserDb);


    sqlite3_exec(db,"CREATE TABLE Messages (idMessage INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "contenu TEXT, "
                    "date DATETIME DEFAULT CURRENT_TIMESTAMP, "
                    "userID INTEGER, "
                    "FOREIGN KEY(userID) REFERENCES User(idUser))",
        nullptr, nullptr, &errMsgMessagesDb);


    // insertion de valeurs
    /*
    insertIntoUserTable(db);
    insertIntoUserTable(db);
    selectAllTable(db, "User");
    */

    insertIntoUserTable(db);
    insertIntoMessagesTable(db);
    selectAllTable(db, "User");
    selectAllTable(db, "Messages");









    // fermer la db
    sqlite3_close(db);

    return 0;
}