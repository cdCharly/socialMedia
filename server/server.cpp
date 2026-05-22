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
#include <mutex>    // pour la file d'attente si deux connection au mee moment
//#include <cstdlib> //pour utiliser atoi()
using namespace std;
std::mutex dbMutex;


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
    int clientSock = *static_cast<int *>(data);

    // 2. On prépare une ligne de texte pour ce message
    string reponse;
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
        int* pointeurId = static_cast<int *>(data);

        // 2. On convertit le texte renvoyé par SQLite (argv[0]) en vrai entier (int)
        // et on le stocke à l'adresse mémoire de notre pointeur
        *pointeurId = atoi(argv[0]);
    }

    return 0;
}


// inserer dans une table
void insertIntoUserTable(sqlite3 *db){

    // variables
    string requete, name, lastname, password;
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

    {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_exec(db, requete.c_str(), nullptr, nullptr, &errMsg);
    }


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
    string requete, contenu, idUser;
    char*errMsg = nullptr;

    // reception infos
    cout << "Ecrire message" << endl;
    cin >> contenu;
    cout << "ecrire idUser (int)" << endl;
    cin >> idUser;

    // construction requete
    requete = "INSERT INTO Messages (contenu, userID) VALUES ('" + contenu +"', '" + idUser + "');";

    {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_exec(db, requete.c_str(), callback_affichage, nullptr, &errMsg);
    }


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


void selectAllTable(sqlite3* db, const string &tableName) {

    // variables
    char* errMsg = nullptr;

    // construction requete + execution
    string requete = "SELECT * FROM "+tableName;

    {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_exec(db, requete.c_str(), callback_affichage, nullptr, &errMsg);
    }


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
void insertIntoMessagesTable(sqlite3 *db, const string &msg, const int idUser) {

    char* errMsg = nullptr;
    string id = to_string(idUser);
    string requete = "INSERT INTO Messages (contenu, userID) VALUES ('"+msg+"', '" + id + "');";

    {
        std::lock_guard<std::mutex> lock(dbMutex);
        sqlite3_exec(db, requete.c_str(), nullptr, nullptr, &errMsg);
    }


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
        char buffer[1024] = {}; // 1024 octets de place pour ecrire
        size_t octRecus = recv(clientSocket, buffer, sizeof(buffer), 0);




        /*
        const char* instructions = "REG:name:lastname:password | LST: | MSG:contenu | LOG:name:password \n";
        send(clientSocket, instructions, strlen(instructions), 0);
        */

        if (octRecus > 0) {

            // verif si le message est LIST alors on donne la liste de tout les messages
            const char* lister = "LST:";        // commande client pour avoir la liste des messages
            const char* ecrire = "MSG:";        // commande client pour ecrire un message
            const char* login = "LOG:";       // commande pour se log
            const char* reg = "REG:";           // commande pour register

            // recuperer la liste des messages
            if (strncmp(buffer, lister, 4) == 0) {
                if (idUser != -1) {

                    {
                        std::lock_guard<std::mutex> lock(dbMutex);
                        //sqlite3_exec(db, "SELECT contenu FROM Messages ;", callback_affichageClient, &clientSocket, &errMsgMessagesDb);
                        sqlite3_exec(db, "SELECT User.name, Messages.contenu FROM Messages LEFT JOIN User ON User.idUser = Messages.userID;", callback_affichageClient, &clientSocket, &errMsgMessagesDb);
                    }

                    string fin = "---- fin des messages -----\n";
                    send(clientSocket, fin.c_str(), strlen(fin.c_str()), 0);        // renvoyer le message de fin
                }
                else {
                    send(clientSocket, "err log \n", strlen("err log \n"), 0);

                }
            }

            // envoyer un message dans la db
            else if (strncmp(buffer, ecrire, 4) == 0) {
                if (idUser != -1) {
                    string m = reinterpret_cast<const char*>(buffer);
                    const int len = static_cast<int>(m.length());
                    string messageClient = parseStr(4, len - 1 , buffer);
                    insertIntoMessagesTable(db, messageClient, idUser);     // on met le message dans la db
                }
                else {
                    send(clientSocket, "err log \n", strlen("err log \n"), 0);
                }
            }

            else if (strncmp(buffer, login, 4) == 0) {
                string m = reinterpret_cast<const char*>(buffer);

                size_t premierSlice = m.find(':');
                size_t deuxiemeSlice = m.find(':', premierSlice + 1);   // à partir du premier slice char après

                if (deuxiemeSlice != std::string::npos) {

                    // recuperer nom
                    size_t indDebutNom = premierSlice + 1;
                    size_t longeurNom = deuxiemeSlice - indDebutNom;
                    string nom = m.substr(indDebutNom, longeurNom);

                    // recuperer mdp
                    size_t indDebutMdp = deuxiemeSlice + 1;
                    size_t longeurMdp = m.length() - indDebutMdp - 1;
                    string mdp = m.substr(indDebutMdp, longeurMdp);

                    cout << "login \n" << nom << " / " << mdp << endl;


                    idUser = -1;
                    // partie verif db
                    string requete = "SELECT idUser FROM User WHERE name = '" + nom + "' AND PASSWORD = '" + mdp + "' ;";
                    {
                        std::lock_guard<std::mutex> lock(dbMutex);
                        sqlite3_exec(db, requete.c_str(), callback_login, &idUser, &errMsgUser);
                    }

                    if (idUser != -1) {
                        string ok = "Connexion reussie ! Ton ID est " + to_string(idUser) + "\n";
                        send(clientSocket, ok.c_str(), ok.length(), 0);
                    } else {
                        string ko = "Erreur: Nom ou mot de passe incorrect.\n";
                        send(clientSocket, ko.c_str(), ko.length(), 0);
                    }



                }

                else {
                    string erreurFormat = "Erreur: Format attendu LOGIN:nom:password\n";
                    send(clientSocket, erreurFormat.c_str(), erreurFormat.length(), 0);
                }
            }


            else if (strncmp(buffer, reg, 4) == 0) {
                string m(buffer);
                // separateurs
                size_t p1 = m.find(':');
                size_t p2 = m.find(':', p1 + 1);
                size_t p3 = m.find(':', p2 + 1);

                if (p2 != std::string::npos && p3 != std::string::npos) {

                    // prenom
                    size_t debPre = p1 + 1;
                    size_t longPre = p2 - debPre;
                    string pre = m.substr(debPre, longPre);

                    // nom de famille
                    size_t debNom = p2 + 1;
                    size_t longNom = p3 - debNom;
                    string nom = m.substr(debNom, longNom);

                    // password
                    size_t debMdp = p3 + 1;
                    size_t longMdp = m.length() - debMdp - 1;
                    string mdp = m.substr(debMdp, longMdp);

                    cout << "new register \n" << pre << nom << endl;

                    // DB request
                    char* errREG = nullptr;
                    string requete = "INSERT INTO User (name, lastName, password) VALUES ('"+ pre + "', '" + nom + "', '" + mdp + "')";
                    {
                        std::lock_guard<std::mutex> lock(dbMutex);
                        sqlite3_exec(db, requete.c_str(), nullptr, nullptr, &errREG);
                    }

                    // err d'insertion
                    if (errREG != nullptr) {
                        string e = errREG;
                        e.append("\n");
                        cout << "probleme insertion messages \n" << errREG << endl;
                        send(clientSocket, errREG, e.length(), 0);
                        sqlite3_free(errREG);
                    }
                    else {
                        cout << "register ok" << endl;
                        string valid = "inscription valide\n";
                        send(clientSocket, valid.c_str(), valid.length(), 0);
                    }
                }
                else {
                    string err = "erreur de format\n";
                    send(clientSocket, err.c_str(), err.length(), 0);
                }
            }
        }

        else {
            cout << "client deconnecte" << endl;
            break; // sortie du client
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

    // return 0;
    }
