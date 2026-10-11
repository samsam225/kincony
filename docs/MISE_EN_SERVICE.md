# Mise en service des capteurs RS485

Aucune écriture au démarrage. Les boutons de diagnostic ou les requêtes POST authentifiées sont les seules commandes d’adresse/vitesse. Les écritures portent sur une liste fermée : TS85-B 0x4000/0x4001, NH3 0x07D0/0x07D1. Les registres d’étalonnage ne sont jamais accessibles à cette procédure.

## Préparer le banc

1. Vérifier carte **A6v3** et alimentation 12 V, bornes, bus, transceiver et direction selon MATERIEL.md.
2. Déconnecter les charges des six relais. Conserver l’état d’observation et les affectations non confirmées.
3. Vérifier modèle complet NH3 sur son étiquette, plage et variante seule/trois-en-un. Renseigner `model`, `rangePpm`, `combined` ; confirmer `modelConfirmed` seulement après cette vérification.
4. Alimenter le capteur selon sa notice ; un seul capteur sur le bus, un seul maître.
5. Provisionner l’authentification, confirmer `hardwareConfirmed` après vérification du PCB/bus sur banc. Garder `busCommissioned=false` pendant les opérations. Aucun relais non confirmé ne peut être demandé ON.
6. Cocher « seul le capteur choisi est connecté ». Les commandes sont refusées si une sortie est commandée ON ou le mode n’est pas Observation.

## TS85-B seul

1. Connecter seulement le TS85-B, usine **adresse 1 / 9600 / 8N1**.
2. Diagnostic : choisir TS85-B, renseigner adresse et vitesse actuelles, « Lire les paramètres actuels ». Une requête 03 lit 0x4000…0x4001 ; confirmer adresse 1 et code vitesse 3.
3. Si nécessaire, « Écrire l’adresse cible » → fonction 06, 0x4000=1. Vérifier l’echo exact. Pas de répétition automatique après timeout.
4. Si nécessaire, « Écrire 9600 bauds » → fonction 06, 0x4001=3, avec les paramètres **encore actifs** de l’appareil. Un appareil déjà à la cible n’a pas besoin d’écriture.
5. Relire via « Vérifier la cible » : adresse 1, 9600 ; valeurs 1 et 3 attendues. Si la vitesse a changé immédiatement, adapter les paramètres actuels avant toute autre écriture. Suivre le redémarrage éventuellement prescrit par le fournisseur de votre variante.
6. Lire les trois mesures par `01 03 00 00 00 03 05 CB`, avec l’A6v3 ou le logiciel constructeur utilisé seul comme maître. Comparer aux instruments de référence/affichage disponibles.
7. Déconnecter le TS85-B avant de connecter le NH3.

## NH3 seul

1. Connecter seulement le NH3, usine **adresse 1 / 4800 / 8N1**.
2. Lire 0x07D0 et 0x07D1 : adresse 1, code vitesse 1. Vérifier l’ordre des registres si la réponse diffère ; la légende de l’exemple de notice est inversée.
3. Commande explicite adresse : fonction 06, 0x07D0=2, avec l’adresse/vitesse actuellement actives. Exemple usine : `01 06 07 D0 00 02 08 86`.
4. Commande explicite vitesse : fonction 06, 0x07D1=2. Exemple à l’ancienne adresse 1 : `01 06 07 D1 00 02 59 46`. Si l’adresse a pris effet immédiatement, utiliser la nouvelle adresse 2. Ne pas deviner ni envoyer à plusieurs adresses sur un bus partagé.
5. **Couper puis rétablir l’alimentation NH3 selon la notice.** Mettre à jour le début du préchauffage à cette remise sous tension ; aucune continuité d’alimentation n’est supposée.
6. Cocher « redémarrage électrique réalisé » puis « Vérifier la cible » à **adresse 2 / 9600 / 8N1**. Lire 0x07D0…0x07D1 et attendre valeurs 2 et 2. Vérification false ou timeout interdit de considérer la mise en service comme terminée.
7. Lire NH3 par fonction 03 registre 2 et comparer avec un affichage/instrument de référence. Appliquer facteur /10 pour 50 ppm, ×1 sinon. Une réponse pendant le préchauffage reste non valide pour le contrôle.

Après une écriture sans echo, ne pas répéter aveuglément : elle peut déjà avoir été prise en compte. Avec ce seul capteur connecté, vérifier les paramètres attendus, appliquer le redémarrage prescrit et rechercher la réponse dans le logiciel constructeur. La procédure n’utilise aucun broadcast.

Le simulateur offre le bouton « Simuler le redémarrage » : il applique les paramètres écrits aux appareils **virtuels**, sans opération électrique réelle. Faire les deux écritures à l’ancienne adresse/vitesse avant cette opération, puis vérifier la cible avec l’attestation correspondante.

## Bus commun

1. Couper les alimentations avant modification du câblage, reconnecter les deux avec A/A, B/B et la référence de potentiel conforme aux appareils.
2. Vérifier terminaison uniquement aux extrémités et absence d’un second maître USB-RS485.
3. Réglages cible : TS85-B 1 / 9600, NH3 2 / 9600, 8N1. Configurer le programme et confirmer `busCommissioned=true` **après vérification physique des deux appareils**.
4. Enregistrer ; retour en observation. Démarrer ou attester le préchauffage NH3 au moment réel de sa mise sous tension.
5. Observer plusieurs cycles : une lecture groupée TS et une lecture NH3, séparées par ≥250 ms. Vérifier les compteurs et l’absence d’exception/CRC/timeout.
6. Valider affectations, politiques et consignes avant toute demande manuelle/automatique. Suivre VALIDATION_PHYSIQUE.md.

## API de diagnostic

`POST /api/commission`, authentifié, jeton `X-CSRF-Token` obligatoire :

```json
{"sensor":"nh","action":"read-current","currentAddress":1,"currentBaud":4800,"isolated":true,"powerRestartConfirmed":false}
```

Actions firmware : `read-current`, `set-address`, `set-baud`, `verify`. Le simulateur ajoute `power-cycle`. Les valeurs écrites sont fixes aux cibles ci-dessus. Le retour 202 du firmware signifie **requête lancée**, pas réussite. Attendre `commissionDone=true` et vérifier statut, trame reçue et `commissionVerified` dans `/api/state`. Une réponse d’exception expose son code. Aucune commande ne remplace une vérification de câblage.