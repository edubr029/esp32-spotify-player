<a href="https://git.eduu.xyz/esp32-spotify-player"><img src="https://us-mirror.eduu.xyz/images/esp32-spotify-player/player.svg" width="160" align="right"></a>

# ESP32 Spotify Player Display

A Spotify player display implementation using ESP32 and an OLED I2C display.

## Overview

This project displays Spotify playback information on an OLED screen connected to an ESP32 microcontroller.

## Requirements

> [!NOTE]
> This project is developed and tested on **Linux-based** systems. Compatibility with other operating systems is not guaranteed.

### Hardware
- ESP32 (or compatible board)
- OLED 128x64 Display (SSD1306 or similar)
- USB cable for programming
- I2C connection cables

### Software
- Arduino IDE with ESP32 support
- [u8g2 library][u8g2] for display control
- Linux-based system
- [playerctl][playerctl]
- Spotify[^1]

[^1]: While Spotify is the default player, others can be used by modifying the `playerctl_to_serial.c` file.

## Installation

1. Install playerctl ([installation guide][playerctl-installation])
2. Clone this repository
3. Compile the serial communication program:

```sh
gcc -o playerctl_to_serial playerctl_to_serial.c -lpthread
```

## Usage

Execute the compiled binary:

```sh
./playerctl_to_serial [--port=SERIAL_PORT] [--baud=BAUD_RATE]
```

Default values:
- Port: `/dev/ttyACM0`
- Baud: `1152000`

Example:
```sh
./playerctl_to_serial -p /dev/ttyUSB0 -b 9600
```

## Configuration

### Display Setup

Configure your OLED display controller in `esp32-spotify-player.ino`:

```cpp
// initialization for the 128x64px OLED display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
```

See [supported controllers][supported-controller] for options.

## Protocol

Serial data format:
```
status;volume%;title;artist;current_position;time_left;full_time;percentage_position;
```

#### Example:
```
1;69%;Open Hearts;The Weeknd;1:21;-2:33;3:54;34;
```

Display Output:

<img src="https://us-mirror.eduu.xyz/images/esp32-spotify-player/example.svg" width="360">

## Troubleshooting

> [!IMPORTANT]
> - Verify serial port connection
> - Check playerctl installation
> - Confirm display I2C connection

Error display:

<img src="https://us-mirror.eduu.xyz/images/esp32-spotify-player/error.svg" width="360">


[u8g2]: https://github.com/olikraus/u8g2
[playerctl]: https://github.com/altdesktop/playerctl
[playerctl-installation]: https://github.com/altdesktop/playerctl/blob/master/README.md#installing
[supported-controller]: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
