# MonoBus
Library and information about Mono Bus 5 and 10. Currently only Mono Bus 5 is 90% reversed and Mono Bus 10 not at all.

## Mono Bus 5
### Pinout
|Pin |Name |Color   |Note|
|----|-----|--------|----|
| 1. | DOT | white  |22V 300mA
| 2. | GND | brown  |Common Ground
| 3. | DAT | green  |24V single wire UART
| 4. | 8V  | yellow |8V logic power

### Addressing
| Panel          |   p7   |   p6   |   p5   |   p4   |   p3   |   p2   |   p1   |   p0   |
|----------------|--------|--------|--------|--------|--------|--------|--------|--------|
|Address base 10 |225-255 |193-223 |161-191 |129-159 |97-127  |65-95   |33-63   |1-31    |
|Address hex     |E1-FF   |C1-DF   |A1-BF   |81-9F   |61-7F   |41-5F   |21-3F   |01-1F   |

### Protocol
Basic command structure:
| Start  | Data | Checksum | Stop   |
|--------|------|----------|--------|
| `0x7E` | ...  | 1 byte   | `0x7E`´|

Basic reply structure:
| Start  | Data | Stop   |
|--------|------|--------|
| `0x7E` | ...  | `0x7E` |

- There is one start and one stop byte both being `0x7E`.
- Each request (except replies) have a checksum at the end right before the stop byte.
- `0x7D` is encoded as `0x7D5D`, `0x7E` is encoded as `0x7D5E` except for the start and stop bytes.
The encoding happens after the message was built and the checksum generated.
- There are different commands (not all known)
    - Commands use the upper 4 bits to encode the command, the lower 4 bits are the address of the client. For example `0xA1` addresses client address `0x01` with command `0xA0`.
    - `0x8X` is some sort of 'get status' command
    - `0x9X` is some sort of 'configuration' command sent before multiple `0xAX` commands. Without this the client won't flip any dots.
    - `0xAX` is the command for sending a column of pixels
        - The command is followed by the address of the column. This address leaves out all numbers where all lower 4 bits are `0x0` or `0x8`. For that we implemented a lookup table. (`lut`)
        - After that there are 6 or 8 bytes of pixel data depending on the row size of the matrix. Pixels are encoded 2 bits each where `0b11` means flip the dot to yellow and `0b10` means flip the dot to black. The MSB bits of the second byte are always left blank, meaning for 2 bytes of data only 7 pixels are included. You can take a llok at the `setPixelColumn` command to see how it works.

#### Get Status
| Start  | Command | Checksum | Stop   |
|--------|---------|----------|--------|
| 1 byte | 1 byte  | 1 byte   | 1 byte |
| `0x7E` | `0x8X`  | `0xXX`   | `0x7E` |

#### Config
| Start  | Command | Config           | Checksum | Stop   |
|--------|---------|------------------|----------|--------|
| 1 byte | 1 byte  | 6 bytes          | 1 byte   | 1 byte |
| `0x7E` | `0x9X`  | `0x011F05370502` | `0xXX`   | `0x7E` |

#### Pixel Row
19 rows:
| Start  | Command | Column        | Dot 1-7   | Dot 8-14  | Dot 15-19 | Checksum | Stop   |
|--------|---------|---------------|-----------|-----------|-----------|----------|--------|
| 1 byte | 1 byte  | 1 byte        | 2 byte            | 2 byte            | 2 byte            | 1 byte | 1 byte |
| `0x7E` | `0xAX`  | `0x01`-`0xFF` | `0x0000`-`0x3FFF` | `0x0000`-`0x3FFF` | `0x0000`-`0x03FF` | `0xXX` | `0x7E` |

28 rows:
| Start  | Command | Column        | Dot 1-7   | Dot 8-14  | Dot 15-21 | Dot 22-28 | Checksum | Stop   |
|--------|---------|---------------|-----------|-----------|-----------|-----------|----------|--------|
| 1 byte | 1 byte  | 1 byte        | 2 byte            | 2 byte            | 2 byte            | 2 byte            | 1 byte | 1 byte |
| `0x7E` | `0xAX`  | `0x01`-`0xFF` | `0x0000`-`0x3FFF` | `0x0000`-`0x3FFF` | `0x0000`-`0x3FFF` | `0x0000`-`0x3FFF` | `0xXX` | `0x7E` |

# Special Thanks
Special thanks to [@atc1441](https://github.com/atc1441) for reversing most of the protocol :rocket: