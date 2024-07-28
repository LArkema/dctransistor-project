#!/usr/bin/python3

#from _socket import _RetAddress
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import csv
import numbers

class Server(BaseHTTPRequestHandler):

    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        #self.send_header('Content-Length', )
        self.end_headers()
        
    def do_HEAD(self):
        self._set_headers()
        
    # GET sends back a Hello world message   //ITT,DATE_TIME,TRACKLINE,TRIP_DIRECTION,DEST_STATION,DESCRIPTION,x,y
    def do_GET(self):
        self._set_headers()
        increment=int(self.path.split('/')[1])
        geometry_outputs = list(csv.DictReader(open('red_line_geometry_tracker.csv', 'r')))
        self.wfile.write(json.dumps(
            {
                "fields": [
                    {
                    "name": "ITT",
                    "type": "esriFieldTypeString",
                    "alias": "ITT",
                    "length": 3
                    },
                    {
                    "name": "DATE_TIME",
                    "type": "esriFieldTypeString",
                    "alias": "DATE_TIME",
                    "length": 30
                    },
                    {
                    "name": "CARNO",
                    "type": "esriFieldTypeInteger",
                    "alias": "CARNO"
                    },
                    {
                    "name": "TRACKLINE",
                    "type": "esriFieldTypeString",
                    "alias": "TRACKLINE",
                    "length": 20
                    },
                    {
                    "name": "TRACKNAME",
                    "type": "esriFieldTypeString",
                    "alias": "TRACKNAME",
                    "length": 2
                    },
                    {
                    "name": "DESTINATIONID",
                    "type": "esriFieldTypeString",
                    "alias": "DESTINATIONID",
                    "length": 2
                    },
                    {
                    "name": "DEST_STATION",
                    "type": "esriFieldTypeString",
                    "alias": "DEST_STATION",
                    "length": 30
                    },
                    {
                    "name": "DESTSTATIONCODE",
                    "type": "esriFieldTypeString",
                    "alias": "DESTSTATIONCODE",
                    "length": 3
                    },
                    {
                    "name": "DESCRIPTION",
                    "type": "esriFieldTypeString",
                    "alias": "DESCRIPTION",
                    "length": 120
                    },
                    {
                    "name": "DIRECTION",
                    "type": "esriFieldTypeDouble",
                    "alias": "DIRECTION"
                    },
                    {
                    "name": "TRIP_DIRECTION",
                    "type": "esriFieldTypeString",
                    "alias": "TRIP_DIRECTION",
                    "length": 1
                    },
                    {
                    "name": "ESRI_OID",
                    "type": "esriFieldTypeOID",
                    "alias": "ESRI_OID"
                    }
                ],
                'features': 
                    [{
                        "attributes": {
                            "ITT":geometry_outputs[increment]['ITT'],
                            "DATE_TIME":geometry_outputs[increment]['DATE_TIME'],
                            "CARNO":6,
                            "TRACKLINE":geometry_outputs[increment]['TRACKLINE'],
                            "TRACKNAME":"D2",
                            "DESTINATIONID":"16",
                            "DEST_STATION":geometry_outputs[increment]['DEST_STATION'],
                            "DESTSTATIONCODE":"J03",
                            "DESCRIPTION":geometry_outputs[increment]['DESCRIPTION'],
                            "DIRECTION":298.0,
                            "TRIP_DIRECTION":geometry_outputs[increment]['TRIP_DIRECTION'],
                            "ESRI_OID":133
                        },
                        "geometry":{
                            "x":float(geometry_outputs[increment]['x']),
                            "y":float(geometry_outputs[increment]['y'])
                        }
                    }]
                }, separators=(',',':')).encode('utf-8') )

        
        
def run(server_class=HTTPServer, handler_class=Server, port=8008):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    
    print('Starting httpd on port %d...' % port)
    httpd.serve_forever()
    
if __name__ == "__main__":
    from sys import argv
    
    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()