# TCP-Bridge

Programme boilerplate tournant sur le raspberry pi faisant le lien entre le contrôleur (via liaison TCP) et l'arduino (via liaison série).

## Usage

`$ TCP-Bridge port-tcp port-serie`

## Code

### arduino-serial-lib

Librairie simplifiant la gestion d'une connection série. Un correctif a été apporté à la fonction `serialport_flush` : la ligne `148 : sleep(2); //required to make flush work, for some reason` a été commenté  car cet appel à `sleep` rendait la librarie inutilisable dans ce contexte.

### arduino-bridge.c

