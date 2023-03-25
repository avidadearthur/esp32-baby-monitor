| Supported Targets | ESP32 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- |

# ESPNOW Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This example shows how to use ESPNOW of wifi. Example does the following steps:

* Start WiFi.
* Initialize ESPNOW.
* Register ESPNOW sending or receiving callback function.
* Add ESPNOW peer information.
* Send and receive ESPNOW data.

This example need at least two ESP devices:

* In order to get the MAC address of the other device, Device1 firstly send broadcast ESPNOW data with 'state' set as 0.
* When Device2 receiving broadcast ESPNOW data from Device1 with 'state' as 0, adds Device1 into the peer list.
  Then start sending broadcast ESPNOW data with 'state' set as 1.
* When Device1 receiving broadcast ESPNOW data with 'state' as 1, compares the local magic number with that in the data.
  If the local one is bigger than that one, stop sending broadcast ESPNOW data and starts sending unicast ESPNOW data to Device2.
* If Device2 receives unicast ESPNOW data, also stop sending broadcast ESPNOW data.

In practice, if the MAC address of the other device is known, it's not required to send/receive broadcast ESPNOW data first,
just add the device into the peer list and send/receive unicast ESPNOW data.

There are a lot of "extras" on top of ESPNOW data, such as type, state, sequence number, CRC and magic in this example. These "extras" are
not required to use ESPNOW. They are only used to make this example to run correctly. However, it is recommended that users add some "extras"
to make ESPNOW data more safe and more reliable.

## How to use example
### Hardware Required

* A development board with ESP32 SoC (e.g., ESP32-DevKitC, ESP-WROVER-KIT, etc.)
* A USB cable for power supply and programming
* A microphone (with amplifier) and one or two speaker(s) for testing.

The following is the hardware connection:

|hardware|module|GPIO|
|:---:|:---:|:---:|
|Microphone|ADC1_CH0|GPIO36|
|speaker(R)|DAC1|GPIO25|

### Code Initialize
* for reciever:
* in config.h, set macro RECV to 1

## Pending actions
* improve the synchronization between network packet collectoin adn filling task and dac i2s_write tasks
* integrate my FFT program

### Configure the project

```
. $HOME/esp/esp-idf/export.sh
idf.py set-target esp32
idf.py menuconfig
```

* Set WiFi mode (station or SoftAP) under Example Configuration Options.
* Set ESPNOW primary master key under Example Configuration Options.
  This parameter must be set to the same value for sending and recving devices.
* Set ESPNOW local master key under Example Configuration Options.
  This parameter must be set to the same value for sending and recving devices.
* Set Channel under Example Configuration Options.
  The sending device and the recving device must be on the same channel.
* Set Send count and Send delay under Example Configuration Options.
* Set Send len under Example Configuration Options.
* Set Enable Long Range Options.
  When this parameter is enabled, the ESP32 device will send data at the PHY rate of 512Kbps or 256Kbps
  then the data can be transmitted over long range between two ESP32 devices.
* Set WIFI AMPDM TX and RX under Compoenent Configuration to false


### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

Here is the example of ESPNOW receiving device console output.

```
initializing i2s spk
Received 3264 packets in 10 seconds
Received 3392 packets in 10 seconds
Received 3264 packets in 10 seconds
Received 3328 packets in 10 seconds
Received 3200 packets in 10 seconds
Received 3328 packets in 10 seconds
Received 3264 packets in 10 seconds
```

Here is the example of ESPNOW sending device console output.

```
Init transport!
initializing i2s mic
Bytes available in stream buffer: 16384 
peak 1 at frequency 430.000000 Hz with amplitude 2.408174 
peak 2 at frequency 1892.000000 Hz with amplitude 2.283813 
peak 1 at frequency 1505.000000 Hz with amplitude 2.708347 
peak 2 at frequency 301.000000 Hz with amplitude 2.529614 
peak 1 at frequency 645.000000 Hz with amplitude 3.665064 
peak 2 at frequency 387.000000 Hz with amplitude 2.986069 
cry detected at f0 387.000000 Hz with amplitude 3.617141 and f2 1376.000000 with amplitude 2.395091
cry detected at f0 387.000000 Hz with amplitude 2.040233 and f2 1376.000000 with amplitude 1.812771
cry detected at f0 387.000000 Hz with amplitude 2.650546 and f2 1376.000000 with amplitude 2.321801
cry detected at f0 387.000000 Hz with amplitude 3.742941 and f2 1376.000000 with amplitude 2.673561
cry detected at f0 387.000000 Hz with amplitude 3.999676 and f2 1376.000000 with amplitude 2.550040
cry detected at f0 387.000000 Hz with amplitude 2.740722 and f2 1376.000000 with amplitude 1.979644
```

## Troubleshooting

If ESPNOW data can not be received from another device, maybe the two devices are not
on the same channel or the primary key and local key are different.

In real application, if the receiving device is in station mode only and it connects to an AP,
modem sleep should be disabled. Otherwise, it may fail to revceive ESPNOW data from other devices.
