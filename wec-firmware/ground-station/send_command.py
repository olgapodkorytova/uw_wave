"""Queue a command for the float through Ground Control.

Examples:
    python send_command.py ping
    python send_command.py set-interval 1800
    python send_command.py starlink 10
    python send_command.py reboot --dry-run

The message waits at the gateway until the float's next session (or the ring alert, if enabled).
The float's ACK shows up later in the server log / web page as a message of type "ack".
"""
import argparse
import os

import requests

import storage
from decode import CMD_IDS, encode_command

MT_URL = "https://rockblock.rock7.com/rockblock/MT"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("command", choices=CMD_IDS)
    p.add_argument("arg", type=int, nargs="?", default=0, help="seconds for set-interval, minutes for starlink")
    p.add_argument("--flush", action="store_true", help="delete older commands still waiting at the gateway")
    p.add_argument("--dry-run", action="store_true", help="build and print the packet but don't send it")
    a = p.parse_args()

    storage.init()
    counter = storage.next_mt_counter()  # always increases, even on dry runs; gaps are fine
    pkt = encode_command(counter, a.command, a.arg)
    print(f"command #{counter}: {a.command} {a.arg} -> {len(pkt)} bytes: {pkt.hex()}")
    if a.dry_run:
        return

    params = {
        "imei": os.environ["WEC_IMEI"],
        "username": os.environ["ROCKBLOCK_USER"],
        "password": os.environ["ROCKBLOCK_PASS"],
        "data": pkt.hex(),
    }
    if a.flush:
        params["flush"] = "yes"
    r = requests.post(MT_URL, params=params, timeout=30)
    print("gateway:", r.text.strip())  # "OK,<id>" or "FAILED,<code>,<reason>"


if __name__ == "__main__":
    main()
