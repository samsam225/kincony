# Validation physique avant exploitation

Ces étapes doivent être exécutées localement par une personne disposant du matériel. Elles ne sont pas des tests réalisés depuis le cloud. Consigner date, référence PCB/module, version du firmware, capteurs et plages, câblage, protections, paramètres et résultats.

## Capteurs séparés puis bus commun

1. Carte et charges hors exploitation : vérifier alimentation 12 V A6v3, TS85-B 9–40 V et NH3 10–30 V, polarités/repères, A/B, masse/isolation, terminaison.
2. Reprendre MISE_EN_SERVICE.md avec un seul appareil à chaque fois ; relire adresse/vitesse et vérifier les CRC des trames réelles. Observer les silences RTU avec un analyseur si disponible. Vérifier la direction réelle du transceiver.
3. TS85-B seul : lire 0/1/2 groupés, comparer température/humidité/CO2 à un instrument ou affichage de référence. Le décodage logiciel d’une température négative est testé avec −10,1 °C ; ne pas confondre ce test de trame avec un étalonnage climatique physique.
4. NH3 seul : vérifier plage, facteur, variante et **préchauffage effectif**. Pendant le décompte, état non valide et aucun usage de la mesure pour une consigne validée. Redémarrer le décompte après coupure de son alimentation.
5. Vérifier NH3 avec un **gaz étalon adapté dans la plage, suivant la procédure fabricant**. Ni briquet, ni gaz pur. La notice déconseille l’usage pour la sécurité des personnes ; ce système n’est pas un dispositif de protection des personnes.
6. Après la relecture des cibles, brancher les deux sur le bus : TS1/NH2, 9600, 8N1, un seul maître. Vérifier plusieurs cycles, plages, âge et validité. Comparer séparément les mesures de chaque capteur.

Critères : pas de CRC/exception/timeout inexpliqué, décodage cohérent avec les références et la plage réelle, préchauffage et péremption visibles, paramètres relus conformes. L’absence d’erreur numérique ne valide pas la précision métrologique.

## Relais individuellement, sans charges d’abord

1. Identifier réellement R1…R6, COM/NO/NC et la polarité des bobines. Confirmer que LOW sur P0…P5 pilote bien les six relais dans cet ordre.
2. **Déconnecter les charges**, contrôler les protections et garder une coupure matérielle de banc accessible. Mode Observation, politiques d’observation OFF et toutes les sorties au repos.
3. Confirmer uniquement l’affectation et la politique de la sortie à tester, après identification. Ne pas confirmer une sortie inconnue. Une confirmation générique de matériel n’autorise pas une affectation non confirmée.
4. Tester R1 puis R2…R6, un à la fois, par impulsion de 100 à 2000 ms (500 ms conseillé pour le test logiciel, pas une consigne d’exploitation). Vérifier contact au multimètre et retour OFF à la fin de la durée ; mesurer la durée réelle et sa tolérance sous charge serveur web.
5. Vérifier refus d’impulsions simultanées, >2000 ms, sans attestation de banc ou hors observation. Redémarrer la carte et noter l’état réel durant reset et après initialisation.
6. Reconnecter chaque équipement suivant son schéma, limites de contacts et protections ; confirmer la fonction réelle par un retour/instrument local. Sans retour matériel raccordé, l’interface continuera d’afficher « état physique inconnu ».

Si la ventilation doit rester alimentée durant un reset, coupure ou défaut de carte, définir et tester une solution matérielle indépendante (contact, alimentation/secours, supervision selon installation). Les politiques logicielles ne peuvent pas agir avant l’initialisation ni pendant une coupure électrique.

## Défauts sur banc avant exploitation

- Retirer uniquement la communication d’un capteur : timeout, invalidation immédiate, ventilation/alarme/chauffage selon politiques confirmées ; mesures anciennes visibles non valides, interruption dans courbe et CSV.
- Injecter CRC erroné, longueur/adresse/fonction incorrectes et exception avec un simulateur de capteur maître unique : réponse rejetée, code indiqué, aucune mesure validée.
- Retarder ou supprimer des réponses pour vérifier la péremption à `staleMs`, la sérialisation et le traitement des octets tardifs.
- Couper puis rétablir alimentation NH3 : opérateur redémarre le préchauffage ; vérifier les délais exacts de la variante.
- Provoquer demandes chauffage/ventilation/humidification simultanées par trames simulées sur banc : vérifier priorité de ventilation et arrêt des fonctions incompatibles, même durant minOn.
- Vérifier seuils, hystérésis et minOn/minOff sur plusieurs cycles ; mesurer la ventilation minimale effective et les exceptions des politiques de défaut.
- Couper le réseau Wi-Fi et l’accès Internet : contrôle local et acquisition continuent ; point d’accès de secours reste disponible. Couper la connexion HTTP : affichage signale état matériel inconnu.
- Redémarrer l’A6v3 : automatique non réarmé, manuel oublié, démarrage par sortie appliqué seulement aux affectations/politiques confirmées, préchauffage à attester.
- Tester défaut I²C sur banc maîtrisé : l’acquittement passe false et l’état de charge reste inconnu. Valider la stratégie indépendante de secours ; ne pas interpréter une commande OFF comme un arrêt physique confirmé.

## Réception

Valider par écrit : espèces/âges, consignes, hystérésis, ventilation minimale, conflits, temporisations, affectation réelle des six relais et politiques de démarrage/défaut/observation/perte réseau. Confirmer chaque équipement et politique dans la configuration, puis réarmer volontairement l’automatique. Sans cette réception, rester en observation.