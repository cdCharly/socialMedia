//
// Created by Charly CATIN--RICO on 13/05/2026.
//
#include <iostream>
#include <sys/socket.h> // Pour les fonctions de base des sockets
#include <netinet/in.h> // Pour les structures d'adresses internet
#include <unistd.h>     // Pour la fonction close() (fermer le socket)
#include <sqlite3.h>
#include <cstring>
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

    // ----------- partie base de données -----------
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
        std::cout << "Base de données ouverte succès" << std::endl;
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


    insertIntoUserTable(db);
    insertIntoMessagesTable(db);
    selectAllTable(db, "User");
    selectAllTable(db, "Messages");

    */



    // ----------- partie réseau -----------
    // af inet correspond à un ipv4
    // sock stream -> protocole tcp

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // verif creation socket réseau
    if (serverSocket < 0) {
        cout << "Erreur avec le socket" << endl;
        return -1; // stopper le serveur car pas de socket pour se connecter
    }

    cout << "socket crée succès" << endl;


    sockaddr_in serverAddr;                     // adresse du serveur
    serverAddr.sin_family = AF_INET;            // meme type que le serversocket
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // ip d'ecoute
    serverAddr.sin_port = htons(8080);          // choix port 8080


    // lier le socket à la connexion
    int bindCheck = ::bind(serverSocket, reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));    // le :: c'est pour le namespace il y avait une erreur

    // verif de la création du lien
    if (bindCheck < 0) {
        cout << "Erreur avec le bind" << endl;
        return -1;  // stopper le serveur car on ne peut pas se connecter
    }
    cout << "socket binder succès" << endl;


    // ecouter le réseau, magic number 5 pour 5 clients max en écoute
    int listenCheck = listen(serverSocket, 5);

    // check pour init de l'ecoute des clients
    if (listenCheck < 0) {
        cout << "Erreur avec le listen" << endl;
        return -1;
    }
    cout << "listen en service succès" << endl;


    // adresse du client pour sauvegarder en ram ses infos réseau
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);       // taille de l'adresse automatique


    // boucle infinie pour que le serveur ne s'arrete pas
    while (true) {

        // creation du socket du client le prog se met en pause le temps que les infos parviennent
        int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);

        if (clientSocket < 0) {
            cout << "Erreur avec le socket client" << endl;
            return -1;
        }
        cout << "accept client succès" << endl;


        // on recupere le message via un buffer
        char buffer[1024]; // 1024 octets de place pour ecrire
        memset(buffer, 0, sizeof(buffer));

        // reception
        int octetsRecus = recv(clientSocket, buffer, 1024, 0);

        // verif reception
        if (octetsRecus <= 0) {
            cout << "Erreur avec le recv" << endl;
        }

        if (octetsRecus > 0) {
            cout << buffer << " | recep succès" << endl;

            send(clientSocket, buffer, octetsRecus, 0); // on renvoie ce que le client à envoyer
            close(clientSocket);                        // on ferme la connexion avec le client
        }





    }
    return 0;
}