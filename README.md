# School Tag Remote Station

Remote stations are mounted on posts around town along the Safe Route to School.  
When students see a station they hold their NFC School Tag up to the station to 
make a record of the visit.  The station transfers a record of the station visit
into the tag.

When the student arrives at school, the student tags in at the School Station which
reads all of the visits made and rewards the student based on the rules of the game.

## Requirements 

* [NXP NTAG 213](https://www.nxp.com/docs/en/data-sheet/NTAG213_215_216.pdf) chips are used in School Tags
    * 144 bytes user programmable read/write memory
        * 36 four byte pages
    * [MIFARE Ultralight C](https://www.nxp.com/docs/en/data-sheet/MF0ICU2_SDS.pdf)
* NDEF Records are used to transport data
    * 1st Record: URL of Tag's UID
        * https://schooltag.org/tag/{UID}
        * Android phones will open a browser to view player's scoreboard
    * 2nd Record: Mime Type = `text/vnd.stag`
        * `;` - entry separator
        * `,` - field separator
        * 1st entry identifies version
            * `1;`
        * Subsequent entries are visits
            * `{station-id},{timestamp};`
                * station id is unique within the system
                    * ids should be short to use fewer bytes
                * timestamp is seconds since epoch
                    * opportunity to reduce size should be explored
        * A message with visits to two stations
            * `1;1Ad,109239092;Pz8,109249393`
* Timestamps must be available
    * Tolerant of +/- 1 minute float every 6 months
* Low Power
    * Battery should last 6 months
    * Sleeping during hours outside of game time is acceptable and encouraged

### Non-Requirements

These requirements are common requests, but won't be  implemented:

* School Identifier
    * A single sensor may serve multiple schools
    * The connected school station will identify the game being played

### Future Enhancements
* [Security](https://en.wikipedia.org/wiki/Signature_Record_Type_Definition) to ensure only School Tag can read/write the data
* Make the MFRC522 NDEF library portable
* Make a portable read/write library for the data

## Arduino 

The code is written to work with the Arduino dev board
which has limitations that must be overcome to match
the requirements:

1. timestamp
1. unique identifer 
1. low power / sleep 

### Arduino Software IDE

Use this development environment to compile and test the software on the Arduino Uno.

1. [Download](https://www.arduino.cc/en/Main/Software) 
    1. Version 1.8.9 confirmed
1. Install `MFRC522` Library
    1. Tools -> Manage Libraries ... 
    1. Version 1.4.4 confirmed
1. Open `SchoolTag-remote-station-arduino.ino`
1. `Verify`
    1. Warnings and notes will appear when compiling the `MFRC522` library the first time.

### VSCode + Arduino IDE

Using VSCode provides a more advanced editor than the Arduino IDE that supports many files with easy linking to code definition.  The VSCode editor requires the Arduino IDE to be installed. 

1. Setup Arduino IDE as described above
1. [Download](https://code.visualstudio.com/Download)
    1. Version 1.37.1 confirmed
1. [Install](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino) Arduino extension
1. Command Palette -> Arduino: Board Config
    1. Select Board
        1. Arduino/Geniuno Uno confirmed
    1. Select Port `/dev/cu.usbmodem...`
    1. Select Programmer can remain unset
1. Command Palette -> Arduino: Upload
1. Command Palette -> Arduion: Open Serial Monitor
    1. Set Baud to 9600
1. Open .vscode/arduino.json
    1. Add `"output": "../build",` 

When testing the Arduino via the Serial Monitor, the Arduino IDE splash screen will appear.

In the Compiler output you should see similar output:

```
Starting] Verify sketch - SchoolTag-remote-station-arduino.ino
Please see the build logs in Output path: /Users/aaronroller/dev/projects/school-tag/school-tag-remote-station-arduino/build
Loading configuration...
Initializing packages...
Preparing boards...
Verifying...
Sketch uses 10672 bytes (33%) of program storage space. Maximum is 32256 bytes.
Global variables use 293 bytes (14%) of dynamic memory, leaving 1755 bytes for local variables. Maximum is 2048 bytes.
Uploading...
[Done] Uploaded the sketch: SchoolTag-remote-station-arduino.ino
```
In the Serial Monitor, you should see similar:
```
[Starting] Opening the serial port - /dev/cu.usbmodem141101
[Info] Opened the serial port - /dev/cu.usbmodem141101
Sketch has been started!
```

## See Also

[Particle Remote Station](https://bitbucket.org/school-tag/school-tag-remote-station-particle/)
[Particle School Station](https://bitbucket.org/school-tag/school-tag-station-particle/)

# Contributions

* [NDEF library](https://github.com/don/NDEF) 
* [MFRC522 NFC Library](https://github.com/miguelbalboa/rfid)
* [Vikas Singh](https://bitbucket.org/%7B1a38eee1-24c6-425b-806d-e523e1c05cf7%7D/)
* [Aaron Roller](https://bitbucket.org/%7Beb7ba780-9619-455e-8458-12c47db1984e%7D/)