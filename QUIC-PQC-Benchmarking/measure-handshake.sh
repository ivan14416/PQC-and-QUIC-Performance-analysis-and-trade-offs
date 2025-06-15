#!/usr/bin/env bash
#
# measure‑handshake.sh
# Run OpenSSL s_client 1000 times and log the handshake RTT (ms).

OUTFILE=
OPENSSL_BIN=
SERVER=
COUNT=
KEY_EXCHANGE=	# optional

# start with a clean output file
: > "$OUTFILE"

for ((i = 1; i <= COUNT; i++)); do
  # Build command in array
  CMD=(
    "$OPENSSL_BIN" s_client
    -quic -alpn ossltest
    -connect "$SERVER"
    -quiet -handshaketime
  )

  # Append -groups only if GROUPS is set
  [[ -n "$KEY_EXCHANGE" ]] && CMD+=(-groups "$KEY_EXCHANGE")

  # Run command and extract RTT
  "${CMD[@]}" 2>&1 1>/dev/null | \
    awk '/Handshake‑RTT:/ {print $(NF-1)}' >> "$OUTFILE"

  printf "run %4d/%d\r" "$i" "$COUNT"
done

echo -e "\nDone. Results written to $OUTFILE"
