# prado-head-unit-plate

## rado compass
[rg1602a](https://www.jaycar.com.au/dot-matrix-white-on-blue-lcd-16x2-character/p/QP5521)
[ESP-WROOM-32](https://www.amazon.com.au/ELEGOO-ESP-WROOM-32-Development-Bluetooth-Microcontroller/dp/B0D8T53CQ5)
![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radocompass.gif)

## rado gps
[grove gps](https://core-electronics.com.au/grove-gps-seeed-studio.html)
[lilygo t4 v1.3](https://lilygo.cc/products/t4?variant=42405660393653)
![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radogps.gif)

## rado uhf
[cc1101 8 pin module](https://www.amazon.com.au/ECSiNG-Wireless-Transceiver-Compatible-Arduinos/dp/B0DS1V77BC)
[lilygo t4 v1.3](https://lilygo.cc/products/t4?variant=42405660393653)

![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radouhf.png)

| CC1101 Pin | T4 Pin | Notes                    |
| ---------- | ------ | ------------------------ |
| **SCK (5)**    | 19     | SPI Clock                |
| **MISO (7)**   | 34     | Master In Slave Out      |
| **MOSI (6)**   | 26     | Master Out Slave In      |
| **CS (4)**     | 33     | Chip Select (active low) |

**NOTE:** In the `ELECHOUSE_CC1101_SRC_DRV.cpp` library file, rename `bool spi` variable to `bool spi__c` to avoid clashes in `TFT_eSPI`

## rado accel
[LSM303AGR](https://www.adafruit.com/product/4413)
[Lilygo T-display S3 amoled](https://lilygo.cc/products/t-display-s3-amoled)