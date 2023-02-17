# What is this folder about ?
### 17/02/2023  11:00 AM  Arthur
I am trying to share audio data between two tasks using a FreeRTOS xQueue in an ESP32 project. I have an I2S microphone that captures audio data and then sends it to a queue. Meanwhile, I want save the audio data in another task that reads the data from the queue and saves it into the sd card. The problem is that when I try to send the data to the queue in the task that captures the audio, I get a Guru Meditation Error: Core 0 panic'ed (StoreProhibited). The code runs without error if I remove the xQueue related code. I suspect that I am not correctly passing the pointer to the data, but I'm not sure how to solve it.

info about xQueue: https://www.freertos.org/a00118.html