Navio
=====

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/emlid/Navio?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Collection of drivers and examples for Navio - autopilot shield for Raspberry Pi.

## Repository structure

### C++

#### Examples

Basic examples showing how to work with Navio's onboard devices using C++.

* AccelGyroMag 
* ADC
* AHRS
* Barometer
* FRAM
* GPS
* LED
* Multithread
* PPM decoder
* Servo

#### Navio

C++ drivers for Navio's onboard devices and peripheral interfaces.

* MPU9250 SPI
* U-blox SPI
* MS5611 I2C
* PCA9685 I2C
* ADS1115 I2C
* MB85R I2C
* I2C driver
* SPI driver

### Python

Basic examples showing how to work with Navio's onboard devices using Python.

* AccelGyroMag 
* ADC
* Barometer
* GPS
* LED
* Servo

### Utilities 

Applications and utilities for Navio.

* 3D IMU visualizer
* U-blox SPI to PTY bridge utility
* U-blox SPI to TCP bridge utility 

## Raspberry Pi 5

This tree has been ported to run on a Raspberry Pi 5. GPIO access no longer uses
`/dev/mem` mmap, sysfs GPIO, or pigpio (pigpio does not support the Pi 5's RP1 I/O
controller at all); it now goes through the Linux libgpiod character-device API —
the C++ drivers use the libgpiod v1 API, and the Python drivers use the `gpiod` v2
package from PyPI. The PPM decoder follows the same change: it used to use pigpio's
alert-callback mechanism and now waits on libgpiod falling-edge events, timed with
kernel event timestamps instead of pigpio's tick counter.

### Prerequisites

```
sudo apt install -y build-essential libgpiod-dev gpiod i2c-tools python3-venv
```

SPI and I2C must be enabled. On Ubuntu for Raspberry Pi they're on by default; on
Raspberry Pi OS you may need to set `dtparam=spi=on` and `dtparam=i2c_arm=on` in
`/boot/firmware/config.txt` and reboot.

### Python setup

```
cd Python
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### Running the examples

Build the C++ examples with `cd C++/Examples && make` (or `make` inside a single
example's directory). Every example needs root to open the gpiochip, spidev, and
i2c device nodes:

```
sudo ./C++/Examples/<Example>/<binary>
sudo venv/bin/python Python/<Example>.py
```

### Notes and gotchas

* On the Pi 5's RP1 SPI controller, Python's `spidev` resets `max_speed_hz` to its
  125 MHz default on every `.open()` call, which silently corrupts SPI reads. Any
  code that opens spidev directly must set `bus.max_speed_hz` explicitly after
  opening (the drivers in this repo already do this, at 1 MHz).
* The `FRAM` example ships two variants, one per FRAM chip: `FRAM/Navio` targets
  the original Navio's FRAM chip and `FRAM/Navio+` targets the Navio+'s
  MB85RC256. Running the wrong variant for your board is expected to fail —
  build and run the one that matches your hardware. Build each from its own
  directory (`cd C++/Examples/FRAM/Navio+ && make`); the top-level
  `C++/Examples` `make` does not descend into either variant.
