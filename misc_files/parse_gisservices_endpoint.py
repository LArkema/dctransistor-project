#!/usr/bin/python3

import requests as re 
import csv
import time

LOC_URL = "https://gisservices.wmata.com/gisservices/rest/services/Public/TRAIN_LOC_WMS_PUB/MapServer/0/query?f=json&where=TRACKLINE%3C%3E%20%27Non-revenue%27%20and%20TRACKLINE%20is%20not%20null&returnGeometry=true&spatialRel=esriSpatialRelIntersects&outFields=*"

#json_locations = locations.json()['features']

with open('station_geometries.csv', 'w', newline='') as csvfile:
    fieldnames = ['ITT', 'TRACKLINE', 'TRIP_DIRECTION', 'DESCRIPTION', 'x', 'y']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore')
    writer.writeheader()

    for i in range (0, 120):
        locations = re.get(LOC_URL).json()['features']
        for location in locations:
            attributes:dict
            attributes = location['attributes']

            if attributes['DESCRIPTION'] != None:
                attributes.update({'x':location['geometry']['x']})
                attributes.update({'y':location['geometry']['y']})
                writer.writerow(attributes)
        
        i+= 1
        time.sleep(10)

            


# special_train = re.get("https://gis.wmata.com/proxy/proxy.ashx?https://gispro.wmata.com/RpmSpecialTrains/api/SpcialTrain").json()['DataTable']['diffgr:diffgram']['DocumentElement']['CurrentConsists']

# for train in special_train:
#     if train['LinkTti'] == '271':
#         print(train)

#print(json_locations[0]['attributes'])
#print(locations.text['features'][0]['attributes'])