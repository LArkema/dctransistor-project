This repository holds the files used to design and manufacture the DCTransistor boards themselves.

`DCTransistor.svg` is an InkScape project file containing the PCB artwork (i.e. map design).
`DCTransistor_BOM.xlsx` is a bill of materials needed to specify the board parts when ordering from JLCPCB.
The remaining files are KiCad files defining the schematic interaction among the different board parts, the physical layout of the PCB board, and the design of each of the components used on the board.

Turning DCTransistor.svg into a KiCad modules requires using the [svg2shenzhen module](https://github.com/badgeek/svg2shenzhen). This [DEFCON talk from TwinkleTwinkie](https://www.youtube.com/watch?v=Sbkvza8cKQE) was my starting point learning how to make PCB art and turn it into a functioning board.

All contents of this folder are [licensed](LICENSE.md) under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License. 