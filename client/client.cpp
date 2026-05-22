//
// Created by Charly CATIN--RICO on 22/05/2026.
//

#include <iostream>
#include <sys/socket.h> // Pour les fonctions de base des sockets
#include <netinet/in.h> // Pour les structures d'adresses internet
#include <unistd.h>     // Pour la fonction close() (fermer le socket)
#include <sqlite3.h>    // gerer db
#include <arpa/inet.h>  // gerer l'ip avec un string
#include <thread>   // le multitache


void listenServer(int socket) {
    while (true) {

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            std::cout << "\n server disconnection \n" << std::endl;
            break;  // sortir de la boucle
        }
        std::cout << buffer << std::endl;   // afficher la reponse du server
    }
}


// if -1 error
int main() {

    sockaddr_in serverAddr;                     // adresse du serveur
    serverAddr.sin_family = AF_INET;            // meme type que le serversocket
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8080);          // choix port 8080

    int clientSock = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSock < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }


    // server connection
    int connectResult = connect(clientSock, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (connectResult < 0) {
        std::cout << "Connection error" << std::endl;
        return -1;
    }

    std::thread listenThread(listenServer, clientSock);
    listenThread.detach();  // se detacher du thread principal

    std::cout << "Connection established succes" << std::endl;
    //return 0; // debug

    // infos user
    std::cout << "type QUIT to quit the clientApp" << std::endl;
    std::cout << "type LOG:lastname:password to log in" << std::endl;
    std::cout << "type REG:name:lastname:password to register" << std::endl;
    std::cout << "type MSG:your message to write a message on the server" << std::endl;

    while (true) {
        // std::string autoLST = "LST:";
        std::string message;
        std::getline(std::cin, message);

        if (message == "QUIT") {
            break;  // sortir du while
        }

        message+="\n";  // car la recv à un \n à la fin
        send(clientSock, message.c_str(), message.length(), 0); // envoie des donnes par le socket du client

        // send(clientSock, autoLST.c_str(), autoLST.length(), 0); // lister les nouveaux messages tt le temps
    }

    std::cout << "closing the connection... " << std::endl;
    close(clientSock);  // on ferme la connection
    std::cout << "connection closed succes" << std::endl;
    std::cout << "program terminated" << std::endl;


    // 0 -> succes
    return 0;
}