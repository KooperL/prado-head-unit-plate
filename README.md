# prado-head-unit-plate

A collection of modifications and accessories to my car's head unit.
Scope has blown past just the head unit, so moving this to [another repo](https://github.com/KooperL/rado-mods).

## Compass
- [rg1602a](https://www.jaycar.com.au/dot-matrix-white-on-blue-lcd-16x2-character/p/QP5521)
- [ESP-WROOM-32](https://www.amazon.com.au/ELEGOO-ESP-WROOM-32-Development-Bluetooth-Microcontroller/dp/B0D8T53CQ5)

Draw compass headings to 16x2 lcd screen. Almost completely useless unless you're in an apocolypse but looks really cool.

<table style="width:100%; text-align:center;">
  <tr>
    <td style="width:50%;">
      <img src="https://raw.githubusercontent.com/KooperL/prado-head-unit-plate/main/images/radoaccel-radocompass.gif" 
           alt="Demo 1" 
           style="width:100%; height:auto;">
    </td>
    <td style="width:50%;">
      <img src="https://raw.githubusercontent.com/KooperL/prado-head-unit-plate/main/images/radocompass.gif" 
           alt="Demo 2" 
           style="width:100%; height:auto;">
    </td>
  </tr>
</table>

## Accelerometer 
- [LSM303AGR](https://www.adafruit.com/product/4413)
- [Lilygo T-display S3 amoled](https://lilygo.cc/products/t-display-s3-amoled)

Visualise accelerometer data by displacing a fixed grid of randomly spread dots

![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radoaccel.gif)

## GPS speedometer
- [grove gps](https://core-electronics.com.au/grove-gps-seeed-studio.html)
- [lilygo t4 v1.3](https://lilygo.cc/products/t4?variant=42405660393653)

Speed from GPS reading changes the speed of a moving perspective grid graphic. A nice looking DIY speedometer.

![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radogps.gif)

## rado wifi packet sniffer
- [lilygo t4 v1.3](https://lilygo.cc/products/t4?variant=42405660393653)

Listens to wifi radio waves and draws activity to screen. See more at [this repo](https://github.com/KooperL/PacketMonitor32).

![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radopacketsniffer.gif)

## UHF radio scanner
- [cc1101 8 pin module](https://www.amazon.com.au/ECSiNG-Wireless-Transceiver-Compatible-Arduinos/dp/B0DS1V77BC)
- [lilygo t4 v1.3](https://lilygo.cc/products/t4?variant=42405660393653)

Find channel activity on UHF/CB sub ghz radio. Useful for finding empty channels, or for finding busy channels.
**NOTE:** In the `ELECHOUSE_CC1101_SRC_DRV.cpp` library file, rename `bool spi` variable to `bool spi__c` to avoid clashes in `TFT_eSPI`

![Demo](https://github.com/KooperL/prado-head-unit-plate/blob/main/images/radouhf.png)
