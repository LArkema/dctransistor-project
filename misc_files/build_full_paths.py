#!/usr/bin/python3


import requests as re 
import csv
import time


                       #RD    #SV-TBD    #YL    #GN   #OR    # BL-TBD
train_ids_to_track = ['232', '264', '254', '321', '260', '270' ]

# GET TRKID DATA
LOC_URL = "https://gisservices.wmata.com/gisservices/rest/services/Public/TRAIN_LOC_WMS_PUB/MapServer/2/query?f=json&where=TRACKLINE%3C%3E%20%27Non-revenue%27%20and%20TRACKLINE%20is%20not%20null&returnGeometry=true&spatialRel=esriSpatialRelIntersects&outFields=*"

with open('train_trkid_tracker.csv', 'a', newline='') as csvfile:
    fieldnames = ['ITT', 'DATE_TIME', 'TRACKLINE', 'TRIP_DIRECTION', 'DEST_STATION', 'DESCRIPTION', 'TRKID']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore')
    #writer.writeheader()

    for i in range (0, 1200):
        trains = re.get(LOC_URL).json()['features']

        for train in trains:
            attributes:dict
            attributes = train['attributes']

            if attributes['ITT'] in train_ids_to_track:
                writer.writerow(attributes)
        
        i += 1
        csvfile.flush()
        time.sleep(5)

print('done')

# GET GEOMETRY DATA

# LOC_URL = "https://gisservices.wmata.com/gisservices/rest/services/Public/TRAIN_LOC_WMS_PUB/MapServer/0/query?f=json&where=TRACKLINE%3C%3E%20%27Non-revenue%27%20and%20TRACKLINE%20is%20not%20null&returnGeometry=true&spatialRel=esriSpatialRelIntersects&outFields=*"

# with open('train_geometry_tracker.csv', 'a', newline='') as csvfile:
#     fieldnames = ['ITT', 'DATE_TIME', 'TRACKLINE', 'TRIP_DIRECTION', 'DEST_STATION', 'DESCRIPTION', 'x', 'y']
#     writer = csv.DictWriter(csvfile, fieldnames=fieldnames, extrasaction='ignore')
#     writer.writeheader()

#     for i in range (0, 1200):
#         trains = re.get(LOC_URL).json()['features']

#         for train in trains:
#             attributes:dict
#             attributes = train['attributes']

#             if attributes['ITT'] in train_ids_to_track:
#                 attributes.update({'x':train['geometry']['x']})
#                 attributes.update({'y':train['geometry']['y']})
#                 writer.writerow(attributes)
        
#         i += 1
#         csvfile.flush()
#         time.sleep(5)

# print('done')

# MAP_SERVER_URL = "https://gisservices.wmata.com/gisservices/rest/services/Public/TRAIN_LOC_WMS_PUB/MapServer/1/query?f=json&where=OBJECTID%20is%20not%20null&returnGeometry=true&outFields=*"

# line_definitions = re.get(MAP_SERVER_URL).json()['features']

# for i, segment in enumerate(line_definitions):


#     match_segments = [(j, other_segment) for (j, other_segment) in enumerate(line_definitions) if segment['geometry']['paths'][0][0] == other_segment['geometry']['paths'][0][-1]]

#     segment_2nd_color = ''
#     if isinstance(segment['attributes']['CLR2'], str):
#         segment_2nd_color = segment['attributes']['CLR2']

#     for match_segment in match_segments:

#         match_2nd_color = ''
#         if isinstance(match_segment[1]['attributes']['CLR2'], str):
#             match_2nd_color = match_segment[1]['attributes']['CLR2']

#         print(str(match_segment[0]) +'/'+str(match_segment[1]['attributes']['OBJECTID']) + ':' + match_segment[1]['attributes']['COLOR'] +'/'+ match_2nd_color)
#         print(str(i)+'/'+str(segment['attributes']['OBJECTID']) + ':' + segment['attributes']['COLOR'] +'/'+ segment_2nd_color )
#         print()

# for i, segment in enumerate(line_definitions):

#     segment_2nd_color = ''
#     if isinstance(segment['attributes']['CLR2'], str):
#         segment_2nd_color = segment['attributes']['CLR2']

#     print(str(i)+':'+segment['attributes']['COLOR'] +'/'+ segment_2nd_color)


# segment_0 = 5
# segment_1 = 6

# for geo in line_definitions[segment_0]['geometry']['paths'][0][-20:]:
#     print(geo)

# print ('SEGMENT BREAK')

# for geo in line_definitions[segment_1]['geometry']['paths'][0][:20]:
#     print(geo)



# Green:  5 -> 6 -> 7 -> 24 -> 10 -> 11
# Yellow: 9 -> 23 -> 4 -> 6 -> 7 -> 24