# Get WMATA certificate fingerprint in format WiFiClientSecure library expects
echo | openssl s_client -showcerts -servername api.wmata.com -connect api.wmata.com:443 2>/dev/null | openssl x509 -inform pem -fingerprint -sha1 | head -n 1 | cut -d '=' -f 2 | sed "s/:/ /g"


# Get list of stations for development reference
curl -X GET "https://api.wmata.com/Rail.svc/json/jStations" -H "api_key: $WMATA_KEY" --data-ascii "{body}" > station_list_json.txt

# Use jq to just get station-code pairings
cat station_list_json.txt | jq '.Stations[] | "\(.Code) \(.Name)"'

# Get lists of stations and associated track IDs
curl -X GET "https://api.wmata.com/TrainPositions/StandardRoutes?contentType=json" -H "api_key: $WMATA_KEY" --data-ascii "{body}" | jq > misc_files/StandRoutes.json

for i in {0..11}; do cat misc_files/StandRoutes.json | jq '.StandardRoutes['$i'] | [{LineCode, TrackNum} + (.TrackCircuits[])]' | jq '.[] | select(.StationCode != null) | {LineCode, TrackNum, StationCode, CircuitId} | join(" ")' > misc_files/track_out_$i.txt; done

for i in {0..11}; do fname=$(head -n 1 misc_files/track_out_$i.txt | cut -d '"' -f 2 | cut -d ' ' -f 1-2 | sed 's/ /_/'); fullname=$(echo -n "$fname"; echo -n "_station_circuits.txt"); cp misc_files/track_out_$i.txt misc_files/$fullname; done

#get comma separated list of circuitIDs associated w/ stations, in reverse order for Dirction 2 cirucits
cat RD_2_station_circuits.txt | tail -n 10 | cut -d ' ' -f 4 | cut -d '"' -f 1 | sort -nr | sed -z 's/\n/, /g'

#get comma separated list of circuit IDs for a given line and direction (in comments in config.h)
cat StandRoutes.json | jq -r '.StandardRoutes[] | select((.LineCode == "GR") and (.TrackNum == 2)) | .TrackCircuits[].CircuitId' | tr '\n' ', ' | sed 's/,/, /g' | less
