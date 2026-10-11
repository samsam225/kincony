# Validation logicielle et état du matériel

11 octobre 2026. Aucun appareil n’est branché aux six relais pour l’instant, selon l’utilisateur. Toutes les affectations/politiques restent non confirmées dans l’exemple.

## Résultats locaux exécutés

- `npm test` : **22 tests réussis**. Vingt tests de logique, un test d’intégration HTTP et un test DOM de l’interface française.
- Décodages exacts des notices : températures positives/négatives, humidité, CO2, facteurs NH3 et variante trois-en-un ; CRC erroné de la notice NH3 rejeté.
- Validité, péremption, préchauffage explicite, hystérésis, temporisations, ventilation minimale/renforcée, conflits prioritaires, modes, politiques de défaut, redémarrage et maintien local sans réseau.
- HTTP : authentification et CSRF, refus de commandes non confirmées, configuration, NH3 virtuel isolé, écritures adresse/vitesse, redémarrage virtuel et relecture. Essais de mesures valides puis timeout, CRC, exception et données périmées avec compteurs vérifiés.
- DOM : quatre mesures, six relais non confirmés désactivés, statut NH3 non valide pendant préchauffage, requêtes CSRF, nom utilisateur rendu comme texte et sélection explicite du profil d’âge.
- `npm run check` : syntaxe JavaScript de l’interface et du serveur réussie.
- Notices locales lues ; sources A6v3 et incohérences documentées.

## Compilation et vérifications C++ observées en CI

Le [workflow du commit f4d1d7a](https://github.com/samsam225/kincony/actions/runs/38178253227) est **réussi** : 22 tests Node, compilation et 65 assertions C++ réussies, comparaison de **120 étapes identiques** entre arbitres C++/JavaScript, puis compilation **PlatformIO A6v3 ESP32-S3**.

Bilan observé de cette compilation : RAM 63460 / 327680 octets (19,4 %), firmware 844057 / 3145728 octets de partition application (26,8 %). Le binaire et l’ELF ont été produits en artefact ; **aucun téléversement** n’est prévu dans le workflow.

Ce succès concerne les derniers fichiers de code : test d’interface, essais renforcés des défauts, sélection explicite du profil d’âge et blocage des réglages pendant une impulsion inclus. Le présent compte rendu est une mise à jour documentaire seule ; le commit de code validé est f4d1d7ab7bded98271f60080bee3cc26e2c21df4.

L’environnement local ne disposait ni de compilateur C++ natif ni de PlatformIO dans le PATH. Les compilations ci-dessus ont été réalisées sur le runner GitHub, pas sur la carte. Le rendu graphique mobile n’a pas été validé : Chrome headless local a rencontré un problème de GPU/sandbox ; seul le test DOM est revendiqué.

## Non exécutés

- Aucun téléversement de firmware.
- Aucun actionnement de relais ni essai électrique.
- Aucune lecture d’un TS85-B/NH3 physique ni test au gaz étalon.
- Aucune validation métrologique, avicole ou du câblage réel.

Voir VALIDATION_PHYSIQUE.md pour la procédure locale et MATERIEL.md pour les éléments à confirmer.