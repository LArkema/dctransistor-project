
# Get list of stations for development reference
curl -X GET "https://api.wmata.com/Rail.svc/json/jStations" -H "api_key: $WMATA_KEY" --data-ascii "{body}" > station_list_json.txt

# Use jq to just get station-code pairings
cat station_list_json.txt | jq '.Stations[] | "\(.Code) \(.Name)"'

