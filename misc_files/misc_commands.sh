# Get WMATA certificate fingerprint in format WiFiClientSecure library expects
echo | openssl s_client -showcerts -servername api.wmata.com -connect api.wmata.com:443 2>/dev/null | openssl x509 -inform pem -fingerprint -sha1 | head -n 1 | cut -d '=' -f 2 | sed "s/:/ /g"


# Get list of stations for development reference
curl -X GET "https://api.wmata.com/Rail.svc/json/jStations" -H "api_key: $WMATA_KEY" --data-ascii "{body}" > station_list_json.txt

# Use jq to just get station-code pairings
cat station_list_json.txt | jq '.Stations[] | "\(.Code) \(.Name)"'

