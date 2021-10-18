#!/bin/bash

# Code (mostly) from Andrea Grandi
# https://www.andreagrandi.it/2020/12/16/how-to-safely-store-arduino-secrets/

include .env

OUTFILE = "arduino_secrets.h"

arduino_secrets:
	@echo "#define SECRET_SSID \"$(SSID_NAME)\"" >> $(OUTFILE)
	@echo "#define SECRET_PASS \"$(WIFI_PASSWORD)\"" >> $(OUTFILE)
	@echo "#define SECRET_WMATA_API_KEY \"$(WMATA_KEY)\"" >> $(OUTFILE)

