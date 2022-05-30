# Driver

Programme final tournant sur l'Arduino DUE pilotant les leds.

## Dépendances

- [FastLED](https://github.com/FastLED/FastLED)

## Code

Le code est divisé en deux parties : 

Une librairie de génération de patterns via des compositions et des effets visuels (écrite par [Jules Fouchy](https://github.com/JulesFouchy)) [documentation](_Readme.md)

Le code de gestion de la connection avec le contrôleur, de l'état du driver, du temps et de gestion des compositions via les fichiers suivant :

### clock.h

#### Clock

Bases de temps en huitième de millisecondes stoquées sur des entiers 32bits. Un maximum de 16 horloges statiques sont gérées (changeables via le second argument du template `Instanced`).

La méthode statique `Tick(timestamp)` met à jour toutes les horloges et les avance jusqu'au timestamp donné.

le champ `clock` et les méthodes `get16()` et `get8()` renvoient la phase actuelle de l'horloge avec une résolution de resp. 32, 16 et 8bits.

#### FallDetector

Trigger s'attachant à une `Clock` dont le champ `trigger` passe à `true` lorsque l'horloge attachée repasse par zéro.

La méthode `reset()` remet le champ `trigger` à `false`.

Comme les horloges les `FallDetectors` sont instanciés de façon statique et peuvent être tous mis à jour en appellant la méthode statique `Tick()`

#### FastClock

Base de temps synchrone avec le framerate global. Permet d'éviter en partie le phénomène d'aliasing qui apparait lorsqu'une horloge a une période très courte par rapport à la durée d'une frame.

Ces horloges sont aussi instanciées et mises à jour par la méthode `Tick()` à appeller à chaque frame.

Le champ `coarse_value` contiens `true` lorsque l'horloge est à la première frame d'un cycle et `false` les autres frames.

Le champ `finevalue` évolue linéairement de 0 à la première frame, à 255 à la dernière frame de la prériode.

### driver.ino

Fichier principal du code du driver, en particulier la liaison entre la librairies FastLed et la logique d'assemblage des différentes compositions, ainsi que la gestion de certains effets post-traitement comme le `strobe` ou le `feedback`.