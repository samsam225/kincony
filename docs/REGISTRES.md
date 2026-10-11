# Registres des capteurs

Les adresses ci-dessous sont les adresses **de trame à base zéro**, en hexadécimal. Aucun registre de protocole KinCony n’est utilisé : les six relais sont pilotés par l’expandeur I²C de l’A6v3.

| Capteur | Adresse registre | Fonction | Type et unité |
|---|---|---|---|
| TS85-B | 0x0000 | 03 | int16 en complément à deux /10 °C |
| TS85-B | 0x0001 | 03 | uint16 /10 %RH |
| TS85-B | 0x0002 | 03 | uint16 ppm CO2 |
| TS85-B | 0x4000 | 03 / 06 | Adresse appareil ; cible 1 |
| TS85-B | 0x4001 | 03 / 06 | Code vitesse ; 3 = 9600 |
| NH3 seul | 0x0002 (également 0x0000) | 03 | uint16 ; facteur selon plage |
| NH3 trois-en-un | 0x0000 | 03 | uint16 /10 %RH |
| NH3 trois-en-un | 0x0001 | 03 | int16 /10 °C |
| NH3 trois-en-un | 0x0002 | 03 | uint16 ; facteur selon plage |
| NH3 | 0x07D0 | 03 / 06 | Adresse appareil ; cible 2 |
| NH3 | 0x07D1 | 03 / 06 | Code vitesse ; 2 = 9600 |

TS85-B : adresse usine 1, 9600 bauds, 8N1. Trois registres lus en **une transaction**, `01 03 00 00 00 03 05 CB` à l’adresse 1. Registres de configuration TS : codes 0…7 = 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200.

NH3 : adresse usine 1, 4800 bauds, 8N1. En variante seule, le firmware lit 0x0002 ; en trois-en-un il lit 0x0000…0x0002 ensemble et utilise le NH3 pour le contrôle. Les température/humidité auxiliaires sont décodées par la bibliothèque, mais ne remplacent pas les mesures TS85-B de la régulation.

| Plage NH3 | Facteur NH3 | Préchauffage minimum |
|---|---:|---:|
| 0–50 ppm haute précision | /10 | 5 min |
| 0–100 ppm haute précision | ×1 | 5 min |
| 0–500 ppm | ×1 | 48 h |
| 0–1000 ppm haute précision | ×1 | 5 min |

NH3 : codes vitesse 0…7 = 2400, 4800, 9600, 19200, 38400, 57600, 115200, 1200. La mise en service ne touche jamais aux registres d’étalonnage 0x0032, 0x0035 ou 0x0038.

Ordre sur le fil : octet fort d’un registre avant son octet faible ; CRC16 Modbus octet faible avant octet fort. Réponses validées sur CRC, adresse, fonction, nombre d’octets, longueur totale et code d’exception ; echo exact pour les écritures 06. Une exception n’est pas une mesure.

## Incohérences constatées

- TS85-B : le texte indique `0x0111 = −273`, mais cette valeur vaut **+273**, donc **+27,3 °C**. `0xFF9B = −101`, donc −10,1 °C.
- TS85-B : humidité 0–100 % et précision ±5 % dans la présentation, 0–99 % et ±2 % dans le tableau. La plage 0–100 est retenue comme contrôle logiciel provisoire, sans garantir une précision non confirmée.
- TS85-B : la notice utilise une requête à 0xFF dite « broadcast ». Ce mécanisme propriétaire n’est pas utilisé ; les commandes prennent une adresse connue entre 1 et 247. Modbus réserve 0 au broadcast sans réponse.
- NH3 : la réponse `01 03 02 00 64` a pour CRC calculé **B9 AF** ; la notice imprime **9B AF**. Le programme vérifie le calcul et refuse l’exemple imprimé erroné.
- NH3 : le tableau de relecture 0x07D0/0x07D1 nomme les champs « vitesse puis adresse », tandis que le tableau des registres les définit « adresse puis vitesse ». Le code suit le tableau des registres ; vérifier sur l’appareil par deux lectures séparées si l’ordre réel contredit la relecture groupée.

Vérifications numériques incluses : TS 0x0111 → 27,3 ; 0x0237 → 56,7 ; 0x01A7 → 423 ; 0xFF9B → −10,1. NH3 0x0064 → 10 ou 100 ppm selon plage ; trois-en-un 0x0167 → 35,9 %RH et 0xFFB5 → −7,5 °C.