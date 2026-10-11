# Arduino IDE — KinCony A6v3

Extraire toute l’archive, puis ouvrir Serre_Avicole_A6v3/Serre_Avicole_A6v3.ino dans Arduino IDE 2. Garder tous les fichiers du dossier ensemble. Le programme principal est dans l’onglet firmware.cpp ; le fichier .ino sert de point d’entrée au projet Arduino.

## Installation

Dans les préférences, ajouter à « URL de gestionnaire de cartes supplémentaires » :
https://espressif.github.io/arduino-esp32/package_esp32_index.json

Installer « esp32 by Espressif Systems », version **2.0.17**, dans le gestionnaire de cartes, et **ArduinoJson 7.3.1** dans le gestionnaire de bibliothèques. WiFi, WebServer, Preferences et Wire sont fournis par le paquet ESP32.

## Réglages du menu Outils

- Carte : **ESP32S3 Dev Module**.
- Flash Size : **16MB (128Mb)**.
- Flash Mode : **QIO 80MHz**.
- PSRAM : **OPI PSRAM**.
- CPU Frequency : **240MHz (WiFi)**.
- USB Mode : **Hardware CDC and JTAG**.
- USB CDC On Boot : **Enabled**.
- USB DFU On Boot et USB Firmware MSC On Boot : **Disabled**.
- Partition Scheme : **16M Flash (3MB APP/9.9MB FATFS)**. Le fichier partitions.csv livré dans le sketch remplace la table standard lors de la compilation.
- Upload Speed : **115200** ; moniteur série : **115200**.

Le fichier build_opt.h active C++17. Ne pas supprimer ce fichier ni partitions.csv.

Cliquer sur **Vérifier** pour compiler. Le téléversement sur la carte est une opération physique séparée, à effectuer seulement après validation du matériel et instruction explicite. Aucun téléversement n’est effectué par les scripts ou la CI.

Les sorties restent bloquées dans la configuration initiale : matériel et équipements non confirmés, profil non validé, mode observation. Le modèle NH3 et l’affectation réelle des relais restent à confirmer. Les notices, procédures de câblage et de mise en service restent dans le README et les autres documents du dépôt.

## Régénération depuis les sources

Depuis la racine du dépôt : node scripts/export-arduino.mjs. Modifier les sources src/, include/, web/ et config/example.json puis régénérer, afin de conserver la même version pour Arduino IDE et PlatformIO.

## Sources

- Carte A6v3 et module ESP32-S3-WROOM-1 N16R8 : https://www.kincony.com/esp32-s3-6-channel-relay-module.html
- Options exactes ESP32 2.0.17 : https://github.com/espressif/arduino-esp32/blob/2.0.17/boards.txt
- Compilation, build_opt.h et partitions.csv : https://github.com/espressif/arduino-esp32/blob/2.0.17/platform.txt
- Installation ArduinoJson : https://arduinojson.org/v7/how-to/install-arduinojson/
