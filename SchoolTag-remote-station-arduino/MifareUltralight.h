#ifndef MifareUltralight_h
#define MifareUltralight_h

#include <MFRC522.h>
#include "NfcTag.h"
#include "Ndef.h"

class MifareUltralight
{
    public:
        MifareUltralight(MFRC522& nfcShield);
        ~MifareUltralight();
        NfcTag read();
        boolean write(NdefMessage& ndefMessage, byte *uid, unsigned int uidLength);
        boolean clean();
    private:
        MFRC522* nfc;
        unsigned int tagCapacity;
        unsigned int messageLength;
        unsigned int bufferSize;
        unsigned int ndefStartIndex;
        boolean isUnformatted();
        void readCapabilityContainer();
        void findNdefMessage();
        void calculateBufferSize();
};

#endif
