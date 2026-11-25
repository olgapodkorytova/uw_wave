'''
What the server does, step by step:

Receive Ground Control's POST.
Check the IMEI.
Deduplicate on MOMSN. The gateway can retry a delivery, so the same message may arrive more than once.
Decode the hex.
Decrypt.
Unpack.
Store the result.
Reply 200 OK quickly, so the gateway doesn't keep retrying.
'''
"""Ground station web server.

POST /iridium-webhook   Ground Control delivers every Iridium message here
POST /starlink-upload   the float uploads directly here when Starlink is on
GET  /                  quick table of the latest messages
"""
import hmac
import html
import json
import logging
import os

from flask import Flask, abort, request

import storage
from decode import decode_packet

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("wec")

EXPECTED_IMEI = os.environ.get("WEC_IMEI")         # optional: reject other modems on the account
UPLOAD_TOKEN = os.environ["WEC_UPLOAD_TOKEN"]      # same as UPLOAD_TOKEN in secrets.h

app = Flask(__name__)
storage.init()


def handle(raw: bytes, via: str, imei=None, momsn=None, meta=None):
    try:
        pkt = decode_packet(raw)
    except ValueError as e:
        log.warning("rejected packet via %s (momsn=%s): %s", via, momsn, e)
        return
    is_new = storage.save_message(pkt, raw, via, imei, momsn, meta)
    log.info("%s via %s: %s", "NEW" if is_new else "duplicate", via, pkt)


@app.post("/iridium-webhook")
def iridium_webhook():
    # Accept both form-encoded and JSON delivery formats.
    data = request.get_json(silent=True) or request.form
    imei = str(data.get("imei", ""))
    if EXPECTED_IMEI and imei != EXPECTED_IMEI:
        abort(403)
    try:
        raw = bytes.fromhex(data.get("data", ""))
    except ValueError:
        return "bad hex", 200  # still 200: retrying will never fix it
    meta = {k: data.get(k) for k in ("transmit_time", "iridium_latitude", "iridium_longitude", "iridium_cep")}
    handle(raw, "iridium", imei, data.get("momsn"), meta)
    return "OK", 200  # answer fast, or the gateway keeps redelivering


@app.post("/starlink-upload")
def starlink_upload():
    if not hmac.compare_digest(request.headers.get("X-Upload-Token", ""), UPLOAD_TOKEN):
        abort(403)
    handle(request.get_data(), "starlink")
    return "OK", 200


@app.get("/")
def index():
    rows = "".join(
        f"<tr><td>{r['received_at'][:19]}</td><td>{r['via']}</td><td>{r['counter']}</td>"
        f"<td>{r['type']}</td><td><code>{html.escape(json.dumps(json.loads(r['fields'])))}</code></td></tr>"
        for r in storage.latest()
    )
    return ("<h2>WEC float — latest messages</h2><table border=1 cellpadding=4>"
            "<tr><th>received (UTC)</th><th>via</th><th>#</th><th>type</th><th>data</th></tr>"
            f"{rows}</table>")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 8000)))
