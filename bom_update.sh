#!/bin/bash

# Simple script to update the bill of materials when a dependency is updated, based on local dependency version

## Update ArduinoJson library version, replacing version in BOM with version Arduino library.properties file.
OldVer=$(cat bom.json | jq '.components[] | select(.name == "ArduinoJson") | .version' | cut -d'"' -f 2)
NewVer=$(find ~/ -path "*ArduinoJson*" -name "library.properties" -exec grep "version" {} \; | cut -d '=' -f 2)

sed -i "s/$OldVer|$NewVer/g" bom.json

## Update LED Backpack library version
OldVer=$(cat bom.json | jq '.components[] | select(.name == "Adafruit LED Backpack") | .version' | cut -d'"' -f 2)
NewVer=$(find ~/ -path "*Adafruit_LED_Backpack_Library*" -name "library.properties" -exec grep "version" {} \; | cut -d '=' -f 2)

sed -i "s|$OldVer|$NewVer|g" bom.json

## Update NeoPixel library version
OldVer=$(cat bom.json | jq '.components[] | select(.name == "Adafruit NeoPixel") | .version' | cut -d'"' -f 2)
NewVer=$(find ~/ -path "*Adafruit_LED_Backpack_Library*" -name "library.properties" -exec grep "version" {} \; | cut -d '=' -f 2)

sed -i "s/$OldVer/$NewVer/" bom.json

## Update ESP8266 core version, replacing version in BOM with version in package path name
OldVer=$(cat bom.json | jq '.components[] | select(.name == "ESP8266 Arduino Core") | .version' | cut -d'"' -f 2)
NewVer=$(find ~/ -path "*esp8266*" -name "package.json" | grep -E "esp8266\/[0-9]+.[0-9]+.[0-9]*" -o | cut -d '/' -f 2 | cut -d ' ' -f 1) #Version is in path name after "esp8266/" and is a series of 2-3 numbers separated by digits (e.g. 3.0.2)

sed -i "s/$OldVer/$NewVer/" bom.json

echo "Print to become successful"