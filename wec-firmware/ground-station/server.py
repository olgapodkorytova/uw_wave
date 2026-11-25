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