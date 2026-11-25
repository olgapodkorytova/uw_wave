"""SQLite storage: received messages + the downlink counter."""
import json
import os
import sqlite3
from contextlib import closing
from datetime import datetime, timezone

DB_PATH = os.environ.get("WEC_DB", "wec.sqlite3")


def _connect():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init():
    with closing(_connect()) as c, c:
        c.execute("""CREATE TABLE IF NOT EXISTS messages (
            id          INTEGER PRIMARY KEY,
            counter     INTEGER UNIQUE NOT NULL,  -- dedupe: gateway retries + same packet via both links
            type        TEXT,
            received_at TEXT,
            via         TEXT,     -- 'iridium' or 'starlink'
            imei        TEXT,
            momsn       INTEGER,
            fields      TEXT,     -- decoded JSON
            meta        TEXT,     -- gateway extras (transmit_time, Iridium position estimate)
            raw_hex     TEXT)""")
        c.execute("CREATE TABLE IF NOT EXISTS state (key TEXT PRIMARY KEY, value INTEGER)")


def save_message(pkt: dict, raw: bytes, via: str, imei=None, momsn=None, meta=None) -> bool:
    """Returns True if new, False if duplicate."""
    with closing(_connect()) as c, c:
        cur = c.execute(
            "INSERT OR IGNORE INTO messages (counter, type, received_at, via, imei, momsn, fields, meta, raw_hex)"
            " VALUES (?,?,?,?,?,?,?,?,?)",
            (pkt["counter"], pkt["type"], datetime.now(timezone.utc).isoformat(), via, imei,
             momsn, json.dumps(pkt), json.dumps(meta or {}), raw.hex()),
        )
        return cur.rowcount == 1


def next_mt_counter() -> int:
    """Downlink counter. Must only ever increase: the float rejects anything <= the last one it accepted."""
    with closing(_connect()) as c, c:
        c.execute("INSERT OR IGNORE INTO state VALUES ('mt_counter', 0)")
        c.execute("UPDATE state SET value = value + 1 WHERE key = 'mt_counter'")
        return c.execute("SELECT value FROM state WHERE key = 'mt_counter'").fetchone()[0]


def latest(n: int = 50):
    with closing(_connect()) as c:
        return c.execute("SELECT * FROM messages ORDER BY id DESC LIMIT ?", (n,)).fetchall()
