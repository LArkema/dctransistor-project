#!/usr/bin/python3

import csv
import math

track_lines = ['Blue', 'Orange', 'Silver', 'Red', 'Green', 'Yellow']

all_stations = {}
# consolidated_stations = []

# Map reference values from https://gisservices.wmata.com/gisservices/rest/services/Public/TRAIN_LOC_WMS_PUB/MapServer
'''
Initial Extent:

    XMin: -8595428.328358412 // -8595429 w/ floor
    YMin: 4693700.296275031 // 4693701 w/ max
    XMax: -8531000.902033508 // -8531001 w/ floor
    YMax: 4740635.136458088 // 4740636 w/ cieling
    Spatial Reference: 102100  (3857)
'''

X_MIN_VAL = -8595429
Y_MIN_VAL = 4693701


with open('ordered_station_geometries.csv', 'r') as csvfile:
    station_csv = list(csv.DictReader(csvfile))

    for line in track_lines:
        line_stations = [station for station in station_csv if station['TRACKLINE'] == line]

        sorted_stations = sorted(line_stations, key=lambda d: int(d['STATION_POS']))

        consolidated_stations = []

        cur_x_min = 999999999
        cur_x_max = 0
        cur_y_min = 999999999
        cur_y_max = 0
        cur_station = 0
        cur_description = ''

        for station in sorted_stations:

            if cur_station != int(station['STATION_POS']):
                if cur_station != 0:
                    consolidated_stations.append({'TRACKLINE':line, 'STATION': cur_description, 'STATION_POS':cur_station, 'MIN_X':cur_x_min, 'MAX_X':cur_x_max, 'MIN_Y':cur_y_min, 'MAX_Y':cur_y_max})
                cur_station+=1
                cur_description = station['DESCRIPTION']
                cur_x_min = 999999999
                cur_x_max = 0
                cur_y_min = 999999999
                cur_y_max = 0


            x = math.floor(float(station['x'])) - X_MIN_VAL
            y = math.ceil(float(station['y'])) - Y_MIN_VAL

            if x < cur_x_min:
                cur_x_min = x
            
            if x > cur_x_max:
                cur_x_max = x

            if y < cur_y_min:
                cur_y_min = y
            
            if y > cur_y_max:
                cur_y_max = y
            

        # Append last station info
        consolidated_stations.append({'TRACKLINE':line, 'STATION': cur_description, 'STATION_POS':cur_station, 'MIN_X':cur_x_min, 'MAX_X':cur_x_max, 'MIN_Y':cur_y_min, 'MAX_Y':cur_y_max})

        # for station in consolidated_stations[:3]:
        #     print(station)

        all_stations[line] = consolidated_stations

    rd_stations = all_stations['Red']

    print()
    for station in rd_stations:
        print( '{min_x},{min_y}'.format(min_x=station['MIN_X'], min_y=station['MIN_Y']), end=',')
    
    print()
        
    # for line, stations in all_stations.items():
        
    #     station_iter = iter(stations)
    #     cur_station = next(station_iter)

    #     while True:
    #         next_station = next(station_iter)
    #         if cur_station['MAX_X'] > next_station['MIN_X'] or cur_station['MAX_Y'] > next_station['MAX_Y']:
    #             print(cur_station)
    #             print(next_station)
    #             print()

    #         cur_station = next_station

        #while next(station_iter)

        #station_iter.

        
        # if station['MIN_X'] == station['MAX_X'] or station['MIN_Y'] == station['MAX_Y']:
        #     print(station)


