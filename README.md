[![Compile and Release Binary](https://github.com/LArkema/dctransistor-project/actions/workflows/compile-sketches.yml/badge.svg)](https://github.com/LArkema/dctransistor-project/actions/workflows/compile-sketches.yml)

Project to make a PCB version of the WMATA metro map that tracks train positions using LEDs. Repository currently contains source code for DCTransistor boards and project code.

For more information on DCTransistor Boards, visit the [website](https://dctransistor.com)

Source code running on the boards is currently under the `DCTransistor` folder. 
Project files used to design and manufacture the boards themselves are in the `board_files` folder.
Business logic used to process board orders on the backend is in the `business_logic` folder.
Code initially loaded onto the board before users perform first-time setup is in the `Setup` folder.

The software bill of materials for the project is in `bom.json`. The hardware bill of materials is in `board_files/DCTransistor_BOM.xlsx` 

All other folders contain miscellaneous, generally outdated artifacts from throughout the development process and are provided for reference only.

## Licensing
This project is provided under a dual-license structure. "Software" files used to generate the code that is loaded onto a chip, as well as miscellaneous project files, are provided under an MIT license. Files used to create the physical PCB boards (i.e. SVG and KiCAD files) are provided under a [Creative Commons BY-NC-SA-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) license (i.e. feel free to make derivative boards for personal use, but do not sell them). Additional details are in [LICENSE.md](LICENSE.md). If you are interested in purchasing a commercial license (limited or exclusive) for the contents of this repository and/or other dctransistor material, contact licensing [at] dctransistor [dot] com.
