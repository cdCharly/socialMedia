//
// Created by Charly CATIN--RICO on 13/05/2026.
//
#include <iostream>
#include <sys/socket.h> // Pour les fonctions de base des sockets
#include <netinet/in.h> // Pour les structures d'adresses internet
#include <unistd.h>     // Pour la fonction close() (fermer le socket)
#include <sqlite3.h>
#include <cstring>
#include <thread>
#include <cstdlib> //pour utiliser atoi()
using namespace std;


static int callback_affichage(void* data, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        // On affiche: NomDeLaColonne = Valeur
        cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << " | ";
    }
    cout << endl; // Saut de ligne pour le prochain utilisateur
    return 0;
}


static int callback_affichageClient(void* data, int argc, char** argv, char** azColName) {
    // 1. On récupère le socket client
    int clientSock = *(int*)data;

    // 2. On prépare une ligne de texte pour ce message
    string reponse = "";
    for (int i = 0; i < argc; i++) {
        reponse += azColName[i];
        reponse += ": ";
        reponse += (argv[i] ? argv[i] : "NULL");
        reponse += " | ";
    }
    reponse += "\n"; // Très important pour la lisibilité chez le client

    // 3. ON ENVOIE AU CLIENT au lieu de faire un cout
    send(clientSock, reponse.c_str(), reponse.length(), 0);

    return 0; // On dit à SQLite de continuer pour la ligne suivante
}



static int callback_login(void* data, int argc, char** argv, char** azColName) {
    // Sécurité : on vérifie qu'on a bien reçu au moins une colonne (l'idUser)
    if (argc > 0 && argv[0] != nullptr) {

        // 1. On transforme le void* en un pointeur d'entier (int*)
        int* pointeurId = (int*)data;

        // 2. On convertit le texte renvoyé par SQLite (argv[0]) en vrai entier (int)
        // et on le stocke à l'adresse mémoire de notre pointeur
        *pointeurId = atoi(argv[0]);
    }

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

// sert à couper le message en commencant par indStart jusqua msg.length -1
string parseStr(int indStart, int indEnd,char* msg) {
    string m = static_cast<string>(msg);
    if (m.empty()) return ""; // si le message est vide on renvoie vide
    else {
        return m.substr(indStart, indEnd); // on enleve le debut et le \n à la fin
    }
}

// m.substr(indStart, m.length() - indStart - 1)

// inserer un message dans Messages en parametres
void insertIntoMessagesTable(sqlite3* db, string msg) {
    char* errMsg = nullptr;
    string requete = "INSERT INTO Messages (contenu) VALUES ('"+msg+"');";
    sqlite3_exec(db, requete.c_str(), nullptr, nullptr, &errMsg);

    if (errMsg != nullptr) {
        cout << "probleme insertion messages " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    else {
        cout << "insertion message succès" << endl;
        sqlite3_free(errMsg);
    }
}


void gererClient(int clientSocket, sqlite3* db) {


    int idUser = -1; // -1 le client est pas co

    char* errMsgMessagesDb = nullptr;
    char* errMsgUser = nullptr;

    while (true) {
        // on recupere le message via un buffer
        char buffer[1024]; // 1024 octets de place pour ecrire
        memset(buffer, 0, sizeof(buffer));
        int octRecus = recv(clientSocket, buffer, sizeof(buffer), 0);

        // verif si le message est LIST alors on donne la liste de tout les messages
        const char* lister = "LIST";        // commande client pour avoir la liste des messages
        const char* ecrire = "MSG:";        // commande client pour ecrire un message
        const char* login = "LOGIN:";

        if (octRecus > 0) {

            // recuperer la liste des messages
            if (strncmp(buffer, lister, 4) == 0) {

                sqlite3_exec(db, "SELECT contenu, date FROM Messages;", callback_affichageClient, &clientSocket, &errMsgMessagesDb); // mettre le resultat dans le socket client avec le callback
                string fin = "---- fin des messages -----";
                send(clientSocket, fin.c_str(), strlen(fin.c_str()), 0);        // renvoyer le message de fin
            }

            // envoyer un message dans la db
            if (strncmp(buffer, ecrire, 4) == 0) {
                string m = reinterpret_cast<const char*>(buffer);
                string messageClient = parseStr(4, m.length() - 1 , buffer);
                insertIntoMessagesTable(db, messageClient);     // on met le message dans la db
            }

            if (strncmp(buffer, login, 4) == 0) {
                string m = reinterpret_cast<const char*>(buffer);

                size_t premierSlice = m.find(':');
                size_t deuxiemeSlice = m.find(':', premierSlice + 1);   // à partir du premier slice char après

                if (deuxiemeSlice != std::string::npos) {

                    // recuperer nom
                    int indDebutNom = premierSlice + 1;
                    int longeurNom = deuxiemeSlice - indDebutNom;
                    string nom = m.substr(indDebutNom, longeurNom);

                    // recuperer mdp
                    int indDebutMdp = deuxiemeSlice + 1;
                    int longeurMdp = m.length() - indDebutMdp - 1;
                    string mdp = m.substr(indDebutMdp, longeurMdp);

                    cout << "login " << nom << " / " << mdp << endl;


                    // partie verif db
                    string requete = "SELECT idUser FROM User WHERE name = '" + nom + "' AND PASSWORD = '" + mdp + "' ;";
                    sqlite3_exec(db, requete.c_str(), callback_login, &idUser, &errMsgUser);


                }

                else {
                    string erreurFormat = "Erreur: Format attendu LOGIN:nom:password\n";
                    send(clientSocket, erreurFormat.c_str(), erreurFormat.length(), 0);
                }


            }
        }

        else {
            cout << "ecriture probleme" << endl;
        }
    }
    close(clientSocket);
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

    selectAllTable(db, "User");
    // boucle infinie pour que le serveur ne s'arrete pas
    while (true) {

        // creation du socket du client le prog se met en pause le temps que les infos parviennent
        int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);

        if (clientSocket < 0) {
            cout << "Erreur avec le socket client" << endl;
            continue;   //  pas de return pour pas faire crash à cause d'un client
        }

        cout << "accept client succès" << endl;

        // lancer le threading
        std::thread threadClient(gererClient,clientSocket , db);
        threadClient.detach();  // detacher le client pour liberer la place


        }

    return 0;
    }
