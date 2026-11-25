//AES-GCM encrypt/decrypt + nonce counter in flash
#pragma once
// AES-128-GCM: encrypts AND authenticates. ground/crypto.py must build the same nonce.
//   nonce (12 bytes) = [direction][7 zero bytes][counter, 4 bytes big-endian]
// The direction byte keeps float->ground and ground->float nonces apart even though they share a key.
#include <stdint.h>
#include <stddef.h>

constexpr uint8_t DIR_MO = 0x01;  // float -> ground
constexpr uint8_t DIR_MT = 0x02;  // ground -> float

bool     cryptoBegin();                // opens the NVS namespace holding the counters
uint32_t nextTxCounter();              // increments and saves to flash BEFORE returning (survives power loss)
bool     acceptRxCounter(uint32_t c);  // false if c <= last accepted MT counter (replay)

bool gcmEncrypt(uint8_t dir, uint32_t counter, const uint8_t* aad, size_t aadLen,
                const uint8_t* pt, size_t len, uint8_t* ct, uint8_t* tag);
bool gcmDecrypt(uint8_t dir, uint32_t counter, const uint8_t* aad, size_t aadLen,
                const uint8_t* ct, size_t len, const uint8_t* tag, uint8_t* pt);
