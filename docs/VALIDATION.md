# Validation logicielle et état du matériel

11 octobre 2026.

## Résultats locaux exécutés

- `node --test tests/core.test.mjs` : **19 tests réussis** (CRC, décodages des notices, plages NH3, défauts, préchauffage, hystérésis, temporisations, ventilation, chauffage/humidification, redémarrage, perte réseau, impulsions et profils d’âge).
- `node --test tests/http.test.mjs` : **1 test d’intégration HTTP réussi**, avec authentification, refus CSRF, page/interface, blocage automatique, mise en service d’un NH3 virtuel isolé, redémarrage virtuel, relecture, refus d’étalonnage et commandes interdites.
- `node --check web/app.js` et `node --check simulator/server.mjs` : réussis.
- Notices locales lues ; tableau des registres et incohérences documentés. Sources officielles A6v3 consultées et citées.

## Compilation et vérifications C++

Les tests C++ et la comparaison de 120 étapes C++/JavaScript sont fournis. La CI `.github/workflows/validation.yml` les exécute puis compile le firmware PlatformIO A6v3. **La présence de ce workflow ne prouve pas sa réussite** : consulter le résultat du commit dans GitHub Actions avant utilisation du binaire.

L’environnement local initial ne disposait ni de compilateur C++ natif ni de PlatformIO dans le PATH ; les téléchargements directs étaient restreints. Aucun résultat de compilation locale n’est revendiqué ici. Les résultats observés de CI, s’ils deviennent disponibles, doivent être reportés avec leur commit.

## Non exécutés

- Aucun téléversement de firmware.
- Aucun actionnement de relais ni essai électrique.
- Aucune lecture d’un TS85-B/NH3 physique ni test au gaz étalon.
- Aucune validation métrologique, avicole ou du câblage réel.

Voir VALIDATION_PHYSIQUE.md pour la procédure locale et MATERIEL.md pour les éléments à confirmer.