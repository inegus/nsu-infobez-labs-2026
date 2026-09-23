#!/bin/bash

URL="http://localhost:5000"

echo "=== (/api/ping) ==="
echo "Payload: 127.0.0.1; whoami; head -n 3 /etc/os-release"
curl -s -X POST "$URL/api/ping" \
     -H "Content-Type: application/json" \
     -d '{"host": "127.0.0.1; whoami; head -n 3 /etc/os-release"}'
echo -e "\n"

echo "=== (/api/ping_safe) invalid ==="
echo "Payload: 127.0.0.1; whoami; head -n 3 /etc/os-release"
curl -s -X POST "$URL/api/ping_safe" \
     -H "Content-Type: application/json" \
     -d '{"host": "127.0.0.1; whoami; head -n 3 /etc/os-release"}'
echo -e "\n"


echo "=== (/api/ping_safe) valid==="
echo "Payload: 127.0.0.1"
curl -s -X POST "$URL/api/ping_safe" \
     -H "Content-Type: application/json" \
     -d '{"host": "127.0.0.1"}'
echo ""