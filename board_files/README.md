This repository holds the files used to design and manufacture the DCTransistor boards themselves.

[DCTransistor_updated.svg](DCTransistor_updated.svg) is an InkScape project file containing the PCB artwork (i.e. map design). The map is the inkscape file used on the most recent (Bidirectional) version of the boards.

The original Inkscape file used to create the first production DCTransistor board run is still available at [DCTransistor_orig.svg](DCTransistor_orig.svg)

`DCTransistor_*_BOM.xlsx` files are bills of materials needed to specify the board parts when ordering from JLCPCB.

The remaining files in `DCTransistor_*_Kicad_Modules.zip` are modified Kicad modules with modified PCB footprints from the default parts' Kicad libraries. Specifically, the footprints were modified to remove nearly all material that would normally be printed on a PCB and shrunk the silkscreen used to identify the orientation / polarity of the LEDs so that they would only be printed under the placed LEDs. 

The best way to get started modifying the projectin KiCad is to download all the Kicad files for a project (everything except the .svg) into a single folder, with the Kicad_Modules zip extracted as a sub-folder, and load the folder as a new KiCad project. The Kicad_Modules extracted sub-folder will need to be added to Kicad as a new source of footprints to use with the project.

Turning DCTransistor.svg into a KiCad modules requires using the [svg2shenzhen module](https://github.com/badgeek/svg2shenzhen). This [DEFCON talk from TwinkleTwinkie](https://www.youtube.com/watch?v=Sbkvza8cKQE) was my starting point learning how to make PCB art and turn it into a functioning board.

All contents of this folder are [licensed](LICENSE.md) under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License. 