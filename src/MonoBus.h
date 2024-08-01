/// @file MonoBus.h
/// @author Moritz Wirger (contact@wirmo.de)
/// @brief Mono Bus 5 and 10 protocol library
#pragma once

#include <Stream.h>
#include <stdint.h>

namespace Mono
{

    // clang-format off
    constexpr uint8_t lut[] = {
          1,   2,   3,   4,   5,   6,   7,   9,  10,  11,  12,  13,  14,  15,  17,  18,  19,  20,  21,  22,  23,  25,  26,  27,  28,  29,  30,  31,   0,   0,   0,   0,
         33,  34,  35,  36,  37,  38,  39,  41,  42,  43,  44,  45,  46,  47,  49,  50,  51,  52,  53,  54,  55,  57,  58,  59,  60,  61,  62,  63,   0,   0,   0,   0,
         65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  86,  87,  89,  90,  91,  92,  93,  94,  95,   0,   0,   0,   0,
         97,  98,  99, 100, 101, 102, 103, 105, 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127,   0,   0,   0,   0,
        129, 130, 131, 132, 133, 134, 135, 137, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159,   0,   0,   0,   0,
        161, 162, 163, 164, 165, 166, 167, 169, 170, 171, 172, 173, 174, 175, 177, 178, 179, 180, 181, 182, 183, 185, 186, 187, 188, 189, 190, 191,   0,   0,   0,   0,
        193, 194, 195, 196, 197, 198, 199, 201, 202, 203, 204, 205, 206, 207, 209, 210, 211, 212, 213, 214, 215, 217, 218, 219, 220, 221, 222, 223,   0,   0,   0,   0,
        225, 226, 227, 228, 229, 230, 231, 233, 234, 235, 236, 237, 238, 239, 241, 242, 243, 244, 245, 246, 247, 249, 250, 251, 252, 253, 254, 255,   0,   0,   0,   0,
    };
    // clang-format on

    constexpr uint8_t calculateColumn(const uint8_t address, const uint8_t column) { return lut[address + column]; }

    constexpr auto test = calculateColumn(199, 0);

    class Bus
    {
    public:
        constexpr static const uint8_t START_BYTE = 0x7E;
        constexpr static const uint8_t STOP_BYTE = 0x7E;
        constexpr static const uint8_t PIXEL_CMD = 0xA0;

    public:
        Bus(Stream& stream) : stream(stream) {};
        virtual void setPixelColumn(const uint8_t address, const uint8_t column, const uint8_t pixels[4]) = 0;

    protected:
        Stream& stream;
        uint8_t buffer[16];
    };

    class DotMatrix
    {
    public:
        DotMatrix(const uint8_t rows, const uint8_t columns, const uint8_t address, const uint8_t index)
            : rows(rows), columns(columns), address(address), index(index) {};

        void write(Bus& bus, const uint8_t panelAddress, const uint8_t column, const uint8_t pixels[4])
        {
            if (column >= columns)
            {
                return;
            }

            const uint8_t col = calculateColumn(address, column);
            bus.setPixelColumn(panelAddress, col, pixels);
        }

    private:
        uint8_t rows;
        uint8_t columns;
        uint8_t address;
        uint8_t index;
    };

    class Matrix28x28 : public DotMatrix
    {
    public:
        Matrix28x28(const uint8_t index) : DotMatrix(28, 28, index * 32 + 1, index) {};
    };

    class Matrix28x21 : public DotMatrix
    {
    public:
        Matrix28x21(const uint8_t index) : DotMatrix(28, 21, index * 32 + 1 + 7, index) {};
    };

    class Panel
    {
    public:
        Panel(const uint8_t address) : address(address) {};

    protected:
        uint8_t address;
    };

    // column
    // |   p7   |   p6   |   p5   |   p4   |   p3   |   p2   |   p1   |   p0   |
    // |--------|--------|--------|--------|--------|--------|--------|--------|
    // |225-255 |193-223 |161-191 |129-159 |97-127  |65-95   |33-63   |1-31    |
    // |E1-FF   |C1-DF   |A1-BF   |81-9F   |61-7F   |41-5F   |21-3F   |01-1F   |
    // if p6 is 21 columns index starts at C8 (C9) instead of C1
    class Panel28x189 : public Panel
    {
    public:
        Panel28x189(const uint8_t address) : Panel(address) {};

        void write(Bus& bus, const uint8_t column, const uint8_t pixels[4])
        {
            if (column < 21)
            {
                bus.setPixelColumn(address, calculateColumn(6 * 32 + 7, column), pixels);
                return;
            }

            const uint8_t columnOffset = column - 21;
            const uint8_t matrix = 5 - (columnOffset / 28);
            const uint8_t matrixColumn = columnOffset % 28;
            bus.setPixelColumn(address, calculateColumn(matrix * 32, matrixColumn), pixels);
        }
    };

    class Panel28x28 : public Panel
    {
    public:
        Panel28x28(const uint8_t address) : Panel(address) {};

        void write(Bus& bus, const uint8_t column, const uint8_t pixels[4])
        {
            if (column >= 28)
            {
                return;
            }
            bus.setPixelColumn(address, calculateColumn(0, column), pixels);
        }
    };

    class Bus5 : public Bus
    {
    public:
        Bus5(Stream& stream) : Bus(stream) {};

        void getStatus(const uint8_t address)
        {
            buffer[0] = START_BYTE;
            buffer[1] = 0x80 | (address & 0xF);
            buffer[2] = 0x00;
            buffer[3] = STOP_BYTE;

            buffer[2] = checksum(buffer, 4);
            // dump(buffer, 4);
            stream.write(buffer, 4);
        }

        void setPixelColumn(const uint8_t address, const uint8_t column, const uint8_t pixels[4]) override
        {
            uint8_t len = 13;

            buffer[0] = START_BYTE;
            buffer[1] = 0xA0 | (address & 0x0F);
            buffer[2] = column;
            buffer[11] = 0x00;
            buffer[12] = STOP_BYTE;

            for (uint8_t i = 0; i < 4; ++i)
            {
                const uint8_t part = pixels[i];
                uint16_t encoded = 0; // This will hold the 16-bit encoded result for each byte
                for (uint8_t bitIndex = 0; bitIndex < 8; ++bitIndex)
                {
                    const bool bit = (part >> bitIndex) & 0x1;

                    if (bit)
                    {
                        encoded |= (0b11 << (bitIndex * 2));
                    }
                    else
                    {
                        encoded |= (0b10 << (bitIndex * 2));
                    }
                }

                buffer[3 + i * 2] = encoded & 0xFF;
                buffer[4 + i * 2] = (encoded >> 8) & 0xFF;
            }

            buffer[11] = checksum(buffer, len);

            // skip first two and last byte
            for (uint8_t i = 2; i < len - 1; ++i)
            {
                const uint8_t value = buffer[i];
                if (value == 0x7D)
                {
                    memmove(&buffer[i + 2], &buffer[i + 1], len - i - 1);
                    buffer[i + 1] = 0x5D;
                    ++i; // skip added byte
                    ++len; // adjust telegram length
                }
                else if (value == 0x7E)
                {
                    memmove(&buffer[i + 2], &buffer[i + 1], len - i - 1);
                    buffer[i] = 0x7D;
                    buffer[i + 1] = 0x5E;
                    ++i; // skip added byte
                    ++len; // adjust telegram length
                }
            }

            // dump(buffer, len);
            stream.write(buffer, len);
        }

        // void setPixelColumn(const uint8_t address, const uint8_t row, const uint8_t pixels[6])
        // {
        //     uint8_t rowi = row + row / 8;
        //     uint8_t buffer[11] = {
        //         START_BYTE,
        //         0xA0 | (address & 0x0F),
        //         row,
        //         pixels[0],
        //         pixels[1],
        //         pixels[2],
        //         pixels[3],
        //         pixels[4],
        //         pixels[5],
        //         0x00,
        //         STOP_BYTE,
        //     };
        //     buffer[sizeof(buffer) - 2] = checksum(buffer, sizeof(buffer));
        //     stream.write(buffer, sizeof(buffer));
        // }

    private:
        uint8_t checksum(const uint8_t* buffer, const uint8_t len)
        {
            uint8_t crc = 0xFF;
            for (uint8_t i = 0; i < len; ++i)
            {
                crc = crc ^ buffer[i];
            }
            return crc;
        }

        void dump(const uint8_t* buffer, const uint8_t len)
        {
            Serial.print("0x");
            for (uint8_t i = 0; i < len; ++i)
            {
                Serial.printf("%02X", buffer[i]);
            }
            Serial.println();
        }
    };

} // namespace Mono