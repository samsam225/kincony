# Gestion locale d’une serre avicole avec KinCony A6v3

Firmware Arduino / PlatformIO pour **KC868-A6v3 ESP32-S3 N16R8**, six relais, TS85-B et transmetteur NH3 RS485. Interface française adaptée au téléphone, fonctionnement local, modes observation/manuels/automatiques, simulateur sans matériel et tests. Le dépôt était vide lors de l’inspection.

**État initial : observation, relais et politiques non confirmés, matériel et bus non confirmés, modèle NH3 provisoire, consignes non validées. Aucun équipement ne peut être actionné avec cette configuration.** Aucun firmware n’a été téléversé et aucun essai matériel n’est revendiqué. Voir [résultats de validation](docs/VALIDATION.md) pour distinguer les tests réellement exécutés des tests à réaliser.

## Architecture

- `include/modbus.h` : CRC16, validation stricte des réponses 03/06, décodage signé, facteurs NH3 et silences RTU.
- `include/control.h` : moteur C++ indépendant du réseau, validité/péremption, préchauffage, hystérésis, arbitrage et temporisations.
- `src/main.cpp` : maître Modbus unique, transactions sérialisées, I²C PCF8574, acquisition, NVS, tâche de contrôle séparée du serveur HTTP et journal en mémoire.
- `include/settings.h` : validation de la configuration et sélection du profil selon espèce/âge.
- `web/` : interface locale sans CDN, commandes authentifiées et protégées par jeton CSRF.
- `simulator/` : serveur Node.js sans dépendance, modèle de référence et capteurs/trames simulés. Il ne peut se connecter à une carte réelle.
- `tests/` : tests JavaScript, HTTP et C++ natifs. `scripts/parity.mjs` compare 120 étapes entre arbitres C++ et JavaScript pour éviter une divergence du simulateur.
- `.github/workflows/validation.yml` : tests, comparaison et compilation ESP32 ; **aucune étape de téléversement**.

### Matériel et documentation

Le [brochage A6v3 officiel](https://www.kincony.com/forum/showthread.php?tid=7622) et la [configuration officielle des sorties](https://www.kincony.com/forum/showthread.php?tid=7623) donnent RX18/TX17, SDA12/SCL11, PCF8574 0x24 et relais P0…P5 actifs LOW. Voir [matériel, sources, direction RS485 et câblage à confirmer](docs/MATERIEL.md). Aucun GPIO ni protocole d’une autre révision n’est repris.

La direction autonome RS485 est **inférée** de la démo officielle A6v3, sans schéma disponible. Elle est isolée dans `include/hardware.h` et doit être confirmée sur la carte. La carte demande **12 V DC** ; TS85-B **9–40 V DC**, NH3 **10–30 V DC**. Les plages capteurs ne définissent pas la tension de la carte.

Les notices locales fournies sont identifiées par leur SHA256 dans la documentation ; elles ont été lues sans être republiées. Voir [tableau complet des registres et incohérences](docs/REGISTRES.md). Le TS85-B est lu par une seule requête 03 de trois registres. Le NH3 seul est lu au registre 2 ; la variante trois-en-un lit les registres 0–2 ensemble.

## Lancer les tests et le simulateur

Node.js 20 ou plus récent suffit, sans `npm install` :

```sh
npm test
npm run check
npm run simulate
```

Ouvrir `http://127.0.0.1:8080`. Utilisateur **admin**, mot de passe aléatoire affiché par le simulateur à chaque lancement ; on peut le fournir avec la variable d’environnement `SIM_PASSWORD`. Aucun secret n’est inscrit dans le dépôt. Le serveur écoute uniquement en boucle locale. `PORT` change le port. Fermer le processus avec Ctrl+C.

Le simulateur conserve les réglages dans `simulator/state.local.json`, ignoré par Git. Au redémarrage, il revient en observation et oublie les commandes manuelles et l’attestation de préchauffage. Supprimer ce fichier local pour retrouver l’exemple initial.

Le banc de simulation permet défauts timeout/CRC/exception/péremption, changement des mesures, avance du temps, perte réseau et redémarrage. Les confirmations dans le simulateur portent sur des appareils virtuels. Pour essayer l’automatique virtuel, confirmer les champs matériels/bus/modèle, le profil avicole et les affectations/politiques voulues dans le JSON ; attester le préchauffage simulé puis choisir Automatique. Les valeurs d’exemple servent aux tests, **pas à valider des consignes avicoles**.

### C++ natif

Avec CMake et un compilateur C++17 :

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
node scripts/parity.mjs build/control_cli
```

Sous Windows, adapter le dernier chemin au générateur, par exemple `build/Debug/control_cli.exe`. `native/main.cpp` est un pilote de scénarios logiciels ; il ne commande aucun matériel.

## Compilation et téléversement

Installer PlatformIO Core dans un environnement Python approprié :

```sh
python -m pip install platformio==6.1.18
pio run -e a6v3
```

La compilation cible uniquement ESP32-S3, avec flash 16 Mo, mémoire QIO/OPI N16R8 et partitionnement fourni. `scripts/embed.py` incorpore la page web et la configuration sans ressource distante. Les dépendances sont fixées dans `platformio.ini`.

**Commande réservée à une décision explicite sur un banc vérifié**, jamais lancée par les tests :

```sh
pio run -e a6v3 -t upload --upload-port COM_A_CONFIRMER
pio device monitor --port COM_A_CONFIRMER --baud 115200
```

Vérifier modèle A6v3, capacité flash, variant USB et port série avant tout téléversement. Le firmware KCS n’est pas une dépendance : ce firmware autonome le remplace.

### Authentification et accès local

Au premier démarrage, le mot de passe aléatoire du point d’accès **Serre-A6v3** est affiché sur l’USB série. Il est conservé en NVS. Ouvrir `http://192.168.4.1`. Tant qu’aucun compte n’a été provisionné, l’API renvoie 503 et aucune commande HTTP n’est autorisée.

Provisionner par USB série, avec des secrets choisis hors du dépôt, sans espaces :

```text
AUTH utilisateur mot_de_passe_HTTP_au_moins_16_caracteres mot_de_passe_AP_au_moins_12_caracteres
```

L’exemple indique les **noms des champs**, pas des identifiants utilisables. Le navigateur demandera ce compte (HTTP Digest). Le simulateur utilise HTTP Basic sur la boucle locale. Les POST exigent le jeton délivré par `/api/config`. Aucun secret Wi-Fi ou HTTP n’est renvoyé par l’API ni ajouté aux journaux. Un effacement complet de NVS impose un nouveau provisionnement. L’interface HTTP doit rester sur un réseau local maîtrisé ; elle ne fournit pas TLS.

## Mise en service RS485

Voir [procédure capteur par capteur](docs/MISE_EN_SERVICE.md). Configuration proposée après vérification :

| Appareil | Adresse cible | Vitesse | Format |
|---|---:|---:|---|
| TS85-B | 1 | 9600 | 8N1 |
| NH3 | 2 | 9600 | 8N1 |

Les deux capteurs ont l’adresse 1 en usine, avec des vitesses différentes. N’effectuer les écritures que sur **un capteur isolé**. Les commandes explicites du diagnostic mettent la lecture normale en pause, écrivent uniquement adresse/vitesse puis permettent la relecture. Aucun changement de configuration du capteur n’est déclenché au démarrage. Le NH3 demande un redémarrage électrique ; l’acquittement d’une écriture ne dispense pas de cette étape.

Garde intertransaction **250 ms**, délai de réponse **1000 ms**, cycle **5 s** par défaut. Les deux premiers sont validés à ≥200 ms suivant la notice TS85-B. Le bus attend aussi le silence RTU, rejette les données tardives et n’a qu’une transaction active. Il vérifie CRC/adresse/fonction/longueur/exception. Les écritures ne sont jamais réessayées automatiquement.

## Régulation et six relais

Les profils portent une espèce, une tranche d’âge et des consignes. La configuration choisit explicitement `species` et `ageDays` ; l’âge ne progresse pas automatiquement sans horloge avicole validée. Chaque profil doit être validé. Une modification de configuration force un retour en observation ; l’automatique n’est jamais réarmé au redémarrage.

La ventilation principale combine un cycle minimal configurable et une demande renforcée selon température, humidité, CO2 ou NH3. La complémentaire suit les dépassements avec hystérésis. Le chauffage suit un seuil bas. R4 peut devenir humidification ou refroidissement suivant l’équipement confirmé ; ces fonctions ont des règles distinctes. L’éclairage est commandé manuellement dans cette version. L’alarme suit dépassements ou défauts selon sa politique. Aucun seuil n’est présenté comme une recommandation zootechnique validée.

Exemple d’affectation **provisoire**, sans sortie confirmée :

| Relais | Rôle exemple | Démarrage exemple | Défaut mesure exemple |
|---|---|---|---|
| R1 | Ventilation principale | ON | ON |
| R2 | Ventilation complémentaire | OFF | ON |
| R3 | Chauffage | OFF | OFF |
| R4 | Humidification, à confirmer | OFF | OFF |
| R5 | Éclairage | OFF | OFF |
| R6 | Alarme | ON | ON |

Ces demandes de repli ne prennent effet que si **affectation et politique sont confirmées**, et si le matériel est confirmé pour écrire les sorties. Le tableau ne représente pas le câblage réel et ne définit pas une stratégie sûre pour une installation inconnue.

Arbitrage : demandes gaz/ventilation prioritaires ; chauffage incompatible avec ventilation/refroidissement ; humidification incompatible avec ventilation, chauffage ou refroidissement. Ce choix conservateur est configurable par l’affectation des rôles, mais les interverrouillages du moteur ne sont pas désactivables par le JSON. Les temporisations minOn/minOff concernent les transitions normales. Défauts, conflits prioritaires, passage en observation et fin d’impulsion peuvent arrêter sans attendre minOn ; la politique de défaut peut demander immédiatement la ventilation ON sans attendre minOff. Il faut valider ces exceptions avec les charges.

Politiques par sortie : `startup`, `sensorFault` (timeout, CRC, exception, données périmées, hors plage, préchauffage ou modèle inconnu), `observation` : **off/on/hold**. `network=continue_local` conserve la régulation locale ; une perte Wi-Fi n’arrête pas le contrôle. HOLD ne conserve pas une commande à travers un redémarrage : l’état initial logiciel est OFF. Les sorties non confirmées restent exclues de toute demande ON, même manuelle.

Le moteur ne possède **aucun retour d’état de charge**. `command` est une intention ; `writeAcknowledged` indique seulement l’acquittement de l’écriture I²C ; `physicalState=null`. Un défaut I²C provoque une alarme sans prétendre arrêter ou maintenir une charge. Prévoir une stratégie matérielle indépendante si une ventilation doit survivre à une coupure électrique, un blocage ou un défaut de carte.

## Mesures, préchauffage et historique

Validité = donnée reçue, communication OK, dans la plage surveillée et âge inférieur à `staleMs`. Les valeurs anciennes restent visibles mais non valides et sont exclues des courbes/CSV. Le préchauffage NH3 doit être démarré explicitement à sa mise sous tension, ou sa fin attestée si le capteur est déjà stabilisé et resté alimenté. Après redémarrage, aucune continuité d’alimentation n’est supposée. La carte ne détecte pas une coupure de l’alimentation externe du capteur ; l’opérateur doit redémarrer le décompte.

Les mesures, alarmes et changements de commande sont journalisés avec un temps monotone depuis le démarrage. Le firmware garde 360 échantillons et 128 événements en RAM ; le simulateur garde 512 événements. Les journaux ne survivent pas au redémarrage ; les réglages seuls sont persistants. Export CSV depuis l’interface. Pas d’horodatage civil garanti, pas de dépendance NTP/Internet.

## Validation physique et limites

Suivre [les essais physiques et les critères d’acceptation](docs/VALIDATION_PHYSIQUE.md) avant exploitation : capteurs séparés, bus commun, préchauffage, comparaison des mesures, relais sans charges et impulsions limitées, défauts sur banc. Les impulsions sont traitées par la tâche locale, limitées à 100–2000 ms et non cumulables ; mesurer leur durée effective sur banc. Aucun logiciel ne remplace une coupure matérielle de banc.

Restent à confirmer : modèle/plage NH3 sur l’étiquette, affectation réelle R1…R6, consignes par espèce/âge, politiques de repli, transceiver/direction, isolation et référence de masse, terminaison, alimentation, protections et contacts réels. Les résultats de tests logiciels ne démontrent aucune de ces propriétés physiques.

La notice NH3 déconseille son usage pour la sécurité des personnes et interdit les essais au briquet ou au gaz pur. Vérifier uniquement avec un **gaz étalon adapté, dans la plage du capteur, selon la procédure fabricant**, sans essai improvisé.