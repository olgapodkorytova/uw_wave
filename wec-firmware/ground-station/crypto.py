'''
The most important rule across both programs: 
packet.cpp and decode.py must define exactly 
the same byte layout. Put a version byte at the 
start of every packet so you can change the format 
later without breaking old data.
'''
"""AES-128-GCM, byte-for-byte compatible with firmware/src/crypto.cpp.

nonce (12 bytes) = [direction][7 zero bytes][counter, 4 bytes big-endian]
"""
import os

from cryptography.exceptions import InvalidTag
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

DIR_MO = 0x01  # float -> ground
DIR_MT = 0x02  # ground -> float

_KEY = bytes.fromhex(os.environ["WEC_AES_KEY"])  # same 16 bytes as AES_KEY in secrets.h
if len(_KEY) != 16:
    raise SystemExit("WEC_AES_KEY must be 32 hex characters (16 bytes)")


def _nonce(direction: int, counter: int) -> bytes:
    return bytes([direction]) + bytes(7) + counter.to_bytes(4, "big")


def encrypt(direction: int, counter: int, header: bytes, body: bytes) -> bytes:
    """Returns ciphertext || 16-byte tag (same order the firmware uses)."""
    return AESGCM(_KEY).encrypt(_nonce(direction, counter), body, header)


def decrypt(direction: int, counter: int, header: bytes, ct_and_tag: bytes) -> bytes:
    try:
        return AESGCM(_KEY).decrypt(_nonce(direction, counter), ct_and_tag, header)
    except InvalidTag:
        raise ValueError("authentication failed (wrong key, corrupted, or forged)") from None
