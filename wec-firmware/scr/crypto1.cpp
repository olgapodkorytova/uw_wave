#include "crypto.h"
#include <Preferences.h>
#include <mbedtls/gcm.h>
#include "packet.h"
#include "secrets.h"

static Preferences nvs;

bool cryptoBegin() { return nvs.begin("crypto", false); }

uint32_t nextTxCounter() {
  uint32_t c = nvs.getUInt("tx", 0) + 1;
  nvs.putUInt("tx", c);  // save first: a reused nonce breaks GCM completely
  return c;
}

bool acceptRxCounter(uint32_t c) {
  if (c <= nvs.getUInt("rx", 0)) return false;
  nvs.putUInt("rx", c);
  return true;
}

static void makeNonce(uint8_t dir, uint32_t counter, uint8_t iv[12]) {
  for (int i = 0; i < 12; i++) iv[i] = 0;
  iv[0]  = dir;
  iv[8]  = counter >> 24;
  iv[9]  = counter >> 16;
  iv[10] = counter >> 8;
  iv[11] = counter;
}

static bool gcm(bool encrypt, uint8_t dir, uint32_t counter, const uint8_t* aad, size_t aadLen,
                const uint8_t* in, size_t len, uint8_t* out, uint8_t* tag) {
  uint8_t iv[12];
  makeNonce(dir, counter, iv);

  mbedtls_gcm_context ctx;
  mbedtls_gcm_init(&ctx);
  int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, AES_KEY, 128);
  if (rc == 0) {
    rc = encrypt
        ? mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, len, iv, sizeof iv,
                                    aad, aadLen, in, out, TAG_LEN, tag)
        : mbedtls_gcm_auth_decrypt(&ctx, len, iv, sizeof iv, aad, aadLen,
                                   tag, TAG_LEN, in, out);
  }
  mbedtls_gcm_free(&ctx);
  return rc == 0;
}

bool gcmEncrypt(uint8_t dir, uint32_t counter, const uint8_t* aad, size_t aadLen,
                const uint8_t* pt, size_t len, uint8_t* ct, uint8_t* tag) {
  return gcm(true, dir, counter, aad, aadLen, pt, len, ct, tag);
}

bool gcmDecrypt(uint8_t dir, uint32_t counter, const uint8_t* aad, size_t aadLen,
                const uint8_t* ct, size_t len, const uint8_t* tag, uint8_t* pt) {
  return gcm(false, dir, counter, aad, aadLen, ct, len, pt, const_cast<uint8_t*>(tag));
}
