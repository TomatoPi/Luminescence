# Qu'est-ce qu'Optopoulpe

Optopoulpe est un système de lumières de scène basé sur les leds adressables [WS2812B](https://www.digikey.fr/en/datasheets/parallaxinc/parallax-inc-28085-ws2812b-rgb-led-datasheet) et la librairie [FastLed](http://fastled.io/).

# Hardware

Côté hardware le système est composé de cinq appareils :

- Les barres de leds.
- Un arduino due [(doc)](https://docs.arduino.cc/hardware/due).
- Un raspberry pi 3 [(doc)](https://www.raspberrypi.com/products/raspberry-pi-3-model-b-plus/)
- Un laptop classique
- Un contrôleur MIDI APC40 [(doc)](https://www.akaipro.com/apc40)

Ils forment ensemble une chaîne transformant des évènements MIDI en commandes envoyées au générateur d'images manipulant les leds.

# Contenu du Git

## Driver

Programme tournant sur l'arduino due contenant le générateur d'images. Celui ci fonctionne à la manière d'un logiciel de *compositing* classique, et manipule divers calques et effets en fonction des actions de l'opérateur sur la console.

## Controller

Programme tournant sur le laptop, il fait le lien entre le contrôleur MIDI et le driver des leds.

## JackProxy

Programme tournant sur le raspberry faisant le pont entre le laptop et l'arduino. Il a été rajouté pour insérer une liaison TCP entre le contrôleur et le driver, pour répondre aux contraintes de distance entre la régie et le plateau.