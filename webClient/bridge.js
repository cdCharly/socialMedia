const net = require('node:net');
const webSocket = require('ws');

// --- 1. FACE TCP (Connexion au C++) ---
const client = net.createConnection({ port: 8080 }, () => {
    console.log('Connecté au serveur C++ !');
    
    // On se connecte juste en arrière-plan, mais on ne demande pas l'historique tout de suite
    //client.write('LOG:a:c\n');
});

client.on('data', (data) => {
    const texteRecu = data.toString();
    console.log("C++ a dit : \n" + texteRecu);
    
    // On distribue la réponse à la page web
    wss.clients.forEach((clientWeb) => {
        if (clientWeb.readyState === webSocket.OPEN) {
            clientWeb.send(texteRecu);
        }
    });
});

client.on('end', () => {
    console.log('Déconnecté du serveur C++');
});


// --- 2. FACE WEB (Serveur WebSocket) ---
const wss = new webSocket.Server({port: 8081});

wss.on('connection', (ws) => {
    console.log("Un navigateur s'est connecté au relais !");

    // CORRECTION 1 : Dès qu'on ouvre l'onglet web, on demande l'historique !
    //client.write('LST:\n');

    ws.on('message', (message) => {
        console.log("Web a dit : " + message.toString());
        
        // On envoie le message au C++
        client.write(message.toString() + '\n');

        // CORRECTION 2 : On demande la liste à jour juste après, comme sur ton ancien client
        client.write('LST:\n');
    });
});