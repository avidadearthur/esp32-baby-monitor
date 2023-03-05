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
|speaker(L)|DAC2|GPIO26|

### Code Initialize
* for reciever:
* in config.c, set macro RECV to 1

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
Packet count: 0
Packet count: 1
Packet count: 2
Packet count: 3
Packet count: 4
Packet count: 5
Packet count: 6
Packet count: 7
Packet count: 8
Packet count: 9
Packet count: 10
Packet count: 11
Packet count: 12
Packet count: 13
Packet count: 14
Packet count: 15
Packet count: 16
Packet count: 17
Packet count: 18
Packet count: 19
Packet count: 20
Packet count: 21
Packet count: 22
Packet count: 23
Packet count: 24
Packet count: 25
Packet count: 26
Packet count: 27
Packet count: 28
Packet count: 29
Packet count: 30
Packet count: 31
Packet count: 32
Packet count: 33
Packet count: 34
Packet count: 35
Packet count: 36
Packet count: 37
Packet count: 38
Packet count: 39
Packet count: 40
Packet count: 41
Packet count: 42
Packet count: 43
Packet count: 44
Packet count: 45
Packet count: 46
Packet count: 47
Packet count: 48
Packet count: 49
Packet count: 50
Packet count: 51
Packet count: 52
Packet count: 53
Packet count: 54
Packet count: 55
Packet count: 56
Packet count: 57
Packet count: 58
Packet count: 59
Packet count: 60
Packet count: 61
Packet count: 62
Packet count: 63
Packet count: 64
Packet count: 65
Packet count: 66
Packet count: 67
Packet count: 68
Packet count: 69
Packet count: 70
Packet count: 71
Packet count: 72
Packet count: 73
Packet count: 74
Packet count: 75
Packet count: 76
Packet count: 77
Packet count: 78
Packet count: 79
Packet count: 80
Packet count: 81
Packet count: 82
Packet count: 83
Packet count: 84
Packet count: 85
Packet count: 86
Packet count: 87
Packet count: 88
Packet count: 89
Packet count: 90
Packet count: 91
Packet count: 92
Packet count: 93
Packet count: 94
Packet count: 95
Packet count: 96
Packet count: 97
Packet count: 98
Packet count: 99
Packet count: 100
Packet count: 101
Packet count: 102
Packet count: 103
Packet count: 104
Packet count: 105
Packet count: 106
Packet count: 107
Packet count: 108
Packet count: 109
Packet count: 110
Packet count: 111
Packet count: 112
Packet count: 113
Packet count: 114
Packet count: 115
Packet count: 116
Packet count: 117
Packet count: 118
Packet count: 119
Packet count: 120
Packet count: 121
Packet count: 122
Packet count: 123
Packet count: 124
Packet count: 125
Packet count: 126
Packet count: 127
Time taken to receive 128 frames: 1 second
```

Here is the example of ESPNOW sending device console output.

```
Init transport!
initializing i2s mic
Packets sent in last second: 361 
Packets lost in last second: 221 
Packets sent in last second: 351 
Packets lost in last second: 222 
Packets sent in last second: 359 
Packets lost in last second: 223 
Packets sent in last second: 361 
Packets lost in last second: 220 
Packets sent in last second: 360 
Packets lost in last second: 219 
Packets sent in last second: 359 
Packets lost in last second: 217 
Packets sent in last second: 362 
Packets lost in last second: 220 
Packets sent in last second: 361 
Packets lost in last second: 221 
Packets sent in last second: 356 
Packets lost in last second: 217 
Packets sent in last second: 362 
Packets lost in last second: 220 
Packets sent in last second: 357 
Packets lost in last second: 224 
Packets sent in last second: 361 
```

## Troubleshooting

If ESPNOW data can not be received from another device, maybe the two devices are not
on the same channel or the primary key and local key are different.

In real application, if the receiving device is in station mode only and it connects to an AP,
modem sleep should be disabled. Otherwise, it may fail to revceive ESPNOW data from other devices.
