# PN5180-Library

Arduino Uno / Arduino ESP-32 library for PN5180-NFC Module from NXP Semiconductors

![PN5180-NFC module](./doc/PN5180-NFC.png)
![PN5180 Schematics](./doc/FritzingLayout.jpg)

Release Notes:

Version 1.8.1 - 19.08.2021

	* Added changes from Nettermann90

Version 1.7 - 12.07.2021

	* Migrated branch from Dirk Carstensen for ISO14443 tags to the library.
	* See https://github.com/tueddy/PN5180-Library/tree/ISO14443

Version 1.6 - 13.03.2021

	* Added PN5180::writeEEPROM

Version 1.5 - 29.01.2020

	* Fixed offset in readSingleBlock. Was off by 1.

Version 1.4 - 13.11.2019

	* ICODE SLIX2 specific commands, see https://www.nxp.com/docs/en/data-sheet/SL2S2602.pdf
	* Example usage, currently outcommented

Version 1.3 - 21.05.2019

	* Initialized Reset pin with HIGH
	* Made readBuffer static
	* Typo fixes
	* Data type corrections for length parameters

Version 1.2 - 28.01.2019

	* Cleared Option bit in PN5180ISO15693::readSingleBlock and ::writeSingleBlock

Version 1.1 - 26.10.2018

	* Cleanup, bug fixing, refactoring
	* Automatic check for Arduino vs. ESP-32 platform via compiler switches
	* Added open pull requests
	* Working on documentation

Version 1.0.x - 21.09.2018

	* Initial versions
