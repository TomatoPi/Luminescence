# Hardware

## TODO

Ajouter des schémas des différents éléments.

## Barres de led

Les barrettes sont fabriquées à partir de ruban de led adressables WS2812B et de profilés en plastique. Les rubans ont une protection contre l'eau qui ajoute également une légère diffusion de la lumière.

Des connecteurs MT30 assurent la liaison entre les barrettes. Il s'agit d'une connectique de modélisme assez robuste et réparable car à souder (contrairement aux connectiques à sertir qui demandent plus des consommables).

## Driver

Le driver des leds est un Arduino due, initialement le pilotage des leds était assuré par deux Arduino Nano Every mais [une limitation matérielle](https://github.com/FastLED/FastLED/wiki/Interrupt-problems) a forcé la migration sur une carte disposant de fonctionnalités supplémentaires.

En particulier cette carte permet de piloter jusqu'à 8 canaux en parallèle, ne souffre pas du problème des interruption et dispose d'un processeur puissant capable de générer plus de 50 images par secondes.

Attention cependant, ce micro-contrôleur fonctionnant en 3.3V, il faut ajouter un adaptateur de niveau logique entre les sorties de l'arduino et les bus de données des leds.

## TCP-Bridge

Programme permettant de faire passer une transmission série via une connection TCP.
Il tourne sur un raspberry pi 3, auquel est connecté d'un côté l'arduino, via usb, et de l'autre le pc du contrôleur, via ethernet.

Un système d'authentification à base de clef SSH permet de connecter facilement le raspberry et le pc [tuto](https://wiki.archlinux.org/title/OpenSSH). De même le raspberry est configuré avec une IP statique afin d'avoir un environnement constant.

En accord avec cette [source](https://adanbucio.net/2017/07/31/how-to-drive-neopixels-from-a-pc-without-a-microcontroller/) Il serait possible de se passer du driver en contrôlant les leds directement depuis le raspberry via un convertisseur USB-UART. Des expérimentations sont en cours mais ceci permettrait de bénéficier de la puissance du pi, en particulier de coder la génération d'image à base de shaders et de le combiner à d'autres types de mapping.

## Contrôleur

Le contrôleur est composé d'un pc faisant simplement le pont entre un contrôleur midi et des commandes pour le driver via l'api [JACK](https://jackaudio.org/).

Une étape de configuration est nécessaire pour pouvoir connecter directement l'ordinateur au pi via un câble ethernet (se référer au manuel de votre [network-manager](https://wiki.archlinux.org/title/Network_configuration#Network_managers))
