# Controller

Ce programme tournant sur un simple laptop fait le pont entre le contrôleur en régie et le driver sur le plateau.

## Dépendances

- [JACK](https://jackaudio.org/api/)

## Usage

Ce projet est normalement compilable avec CMake, sous linux uniquement et génère l'executable `Controller` qui se lance avec la commande :

`$ Controller setup-file save-file driver-ip driver-port`.

Le serveur JACK doit avoir été lancé avant.
Le driver peut être lancé après (attention au port).
Une fois lancé il faut connecter les ports midi du programme et du contrôleur midi (via un gestionnaire de connections comme [qjackctl](https://qjackctl.sourceforge.io/) ou [patchage](http://drobilla.net/software/patchage.html))

## Code

Le fichier `controller.cpp` contiens le main et fait le pont entre quatre modules :

### jack-bridge

Pont entre le contrôleur matériel (via l'api [JACK](https://jackaudio.org/api/)) et le prorgamme

- les messages midi sont représentés par des `std::vector<uint8_t>`
- la methode `incomming_midi()` renvois la liste des messages capturés par le contrôleur depuis le dernier appel
- la méthode `send_midi(msg)` met en queue un message à destination du contrôleur
- le constructeur de `JackBridge(name)` prends en argument le nom du [client JACK](https://jackaudio.org/api/group__ClientFunctions.html#gabbd2041bca191943b6ef29a991a131c5) à créer.
- les messages sont passés du thread audio au thread principal du programme via une queue FIFO.

### mapper 

Transforme les messages midi en commandes pour le générateur d'images

- les messages midi sont représentés par des `std::vector<uint8_t>`
- les commandes sont représentées par des `std::string`
- les methodes `midimsg_to_command` et `command_to_midimsg` transforment resp. des messages midi en liste de commandes, et des commandes en liste de message midi.
- l'objet `binding_t` représente un lien entre un type d'évènement et une commande.
- l'objet `Mapper` stocke une table de liens.

### mannager

Gère l'état du contrôleur et transforme les commandes en messages pour le driver

- l'objet `control_t` représente un lien entre une commande et des paramètres du driver
    - ils sont identifiés par leur addresse dans la table de paramètres du driver
    - la méthode `to_command_string()` renvois la commande à envoyer au contrôleur pour afficher l'état du contrôle
    - la méthode `to_raw_message()` renvois le message à envoyer au driver pour mettre à jour le paramètre contrôlé
    - le callback `on_update` est appellé lorsqu'une commande change l'état du contrôle.
- l'objet `Manager` stocke une table de contrôles et gère la sauvegarde de l'état du contrôleur
    - le constructeur prends en arguments le chemin du fichier de sauvegarde et le lien du fichier de configuration
    - la méthode `process_command(cmd)` traite une commande et renvois l'ensemble des contrôles à mettre à jour pour répondre à cette commande.

### arduino-bridge

Gère la connection TCP avec le driver des leds

- les messages envoyés sont au format binaire `std::vector<uint8_t>` via la méthode `send(addr, packet)`
- les messages reçus sont au format texte (log de l'état du driver) via la méthode `receive()`
- le constructeur prends en argument l'addresse IP du driver ainsi que le port de la connection

