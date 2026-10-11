#pragma once
// Exclusivement KC868-A6v3 : voir docs/MATERIEL.md et ses sources officielles.
namespace hardware {
constexpr int rs485Rx = 18, rs485Tx = 17;
constexpr int i2cSda = 12, i2cScl = 11;
constexpr unsigned char relayAddress = 0x24;
constexpr unsigned char relayBits[6] = {0,1,2,3,4,5};
constexpr bool relayActiveLow = true;
// Démo A6v3 officielle : RX/TX uniquement ; aucune broche DE/RE documentée.
// Direction autonome déduite de la démo et du MS2548, à confirmer sur PCB/banc.
constexpr int directionPin = -1;
constexpr bool directionActiveHigh = true;
}
