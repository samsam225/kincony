# Matériel A6v3 et provenance

Vérification documentaire le 11 octobre 2026. Le dépôt était vide ; aucun firmware ni AGENTS.md préexistant. Aucun brochage de l’A6, de l’A6v1.4 ou d’une autre carte n’est repris.

| Élément A6v3 | Configuration retenue | Source |
|---|---|---|
| Microcontrôleur | ESP32-S3, module N16R8, flash 16 Mo / PSRAM 8 Mo | [Fiche A6v3 officielle](https://www.kincony.com/esp32-s3-6-channel-relay-module.html) |
| Alimentation carte | 12 V DC | Même fiche ; ne pas extrapoler les 30/40 V des capteurs |
| RS485 | RX GPIO18, TX GPIO17 | [Brochage officiel A6v3](https://www.kincony.com/forum/showthread.php?tid=7622) |
| I²C | SDA GPIO12, SCL GPIO11 | Même brochage |
| Relais | PCF8574, adresse 7 bits 0x24, P0…P5 → R1…R6 | [Démo Arduino A6v3 relais](https://www.kincony.com/forum/showthread.php?tid=7371) |
| Polarité relais | LOW = bobine commandée ON ; HIGH = OFF | [Configuration A6v3 officielle, inverted: true](https://www.kincony.com/forum/showthread.php?tid=7623) |
| Transceiver | MS2548 mentionné sur la fiche | Fiche A6v3 officielle |

**Direction RS485 :** aucune broche DE/RE n’est documentée sur l’A6v3. La [démo RS485 A6v3](https://www.kincony.com/forum/showthread.php?tid=7374) utilise RX18/TX17 sans pilotage de direction. Le fonctionnement autonome est une **inférence** fondée sur cette démo et le transceiver cité. Le schéma A6v3 n’a pas été trouvé ; le [fabricant indiquait ne pas le fournir](https://www.kincony.com/forum/showthread.php?tid=7859). `hardware::directionPin = -1` isole cette hypothèse. Confirmer la référence du transceiver et son fonctionnement sur le PCB/banc avant `hardwareConfirmed=true`. Si un pilotage DE/RE s’avère nécessaire, renseigner uniquement une broche attestée et sa polarité ; ne pas en inventer une.

La démo RS485 contient un commentaire ancien citant GPIO9/8 et une autre carte. Ses définitions exécutables sont bien 18/17, corroborées par le brochage A6v3 et son YAML ; le commentaire n’est pas utilisé.

PlatformIO utilise la base ESP32-S3 DevKitC et remplace explicitement la taille flash et le mode mémoire pour N16R8. La [base PlatformIO](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html) est N8 sans PSRAM par défaut ; elle ne constitue pas un brochage KinCony. Vérifier la référence exacte du module posé (WROOM-1 / 1U, N16R8), la capacité flash détectée et la variante USB avant téléversement.

## Câblage à confirmer sur le montage

- Carte : alimentation régulée 12 V DC sur ses bornes d’alimentation, selon les marquages réels.
- TS85-B : 9–40 V DC ; suivre les bornes + / − / A / B imprimées.
- NH3 : 10–30 V DC ; vérifier l’étiquette complète et les fils du modèle livré. Ne pas déduire les couleurs d’une autre variante.
- Relier A à A et B à B. Utiliser une paire torsadée en chaîne, limiter les dérivations et éviter une étoile.
- Vérifier si le port RS485 de chaque appareil est isolé. Établir la référence de potentiel/masse suivant leurs notices et l’installation, sans créer de boucle de masse ; ne pas assimiler systématiquement masse logique, blindage et terre.
- Examiner les résistances déjà présentes. Terminer aux deux extrémités physiques du bus, typiquement 120 Ω adaptées au câble ; pas sur chaque capteur. Prévoir la polarisation à un seul endroit si nécessaire.
- Un seul maître : débrancher le maître USB-RS485 pendant l’exploitation par l’A6v3.
- Confirmer COM/NO/NC, courant et nature des charges. Un contact fermé ou une bobine commandée ne prouve pas le fonctionnement de la charge. Un ventilateur/chauffage peut nécessiter contacteur, protection et retour de fonctionnement.

Référence de protocole et de câblage : [Modbus over Serial Line V1.02](https://www.modbus.org/file/secure/modbusoverserial.pdf). Les capteurs imposent 8N1 ; le silence est calculé conservativement sur 11 bits par caractère et au moins 3,5 caractères (1,75 ms au-delà de 19200 bauds). Le garde intertransaction par défaut de 250 ms est nettement supérieur.

## Notices locales utilisées

Les fichiers ont été lus depuis Downloads. Ils ne sont pas republiés dans le dépôt public. Conserver les originaux pour la validation du matériel ; les empreintes identifient exactement les versions utilisées.

| Notice | Taille | SHA256 |
|---|---:|---|
| TS85-B Modbus说明.pdf | 185167 octets | E16DC7368C72FBC46D173DFE0AD34CF4C1B36E8491BD5EBD70942FBBAAAF045E |
| 氨气变送器（4..型）.doc | 3258251 octets | 737B2639815F5DA469A40BEB51FA2CDCA38175C71E3C95C8FDD86B501131F27A |

Données extraites du PDF TS85-B (tables, paramètres série, exemples et délais) et du texte/tableaux du DOC NH3. Le modèle NH3 complet reste à confirmer ; le précédent échange indiquait NH3 seul 0–500 ppm, conservé provisoirement avec `modelConfirmed=false`.