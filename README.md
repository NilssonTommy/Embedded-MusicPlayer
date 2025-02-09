# Embedded-MusicPlayer
OverviewThis project is a real-time embedded music player implemented in C using the TinyTimber framework. The system runs on an MD407 development board with an STM32 microcontroller, allowing users to control playback, adjust tempo and pitch, and manage volume levels via CAN-bus communication and serial input. It is designed for real-time execution with interrupt-driven input handling.


🎼  Features Play Brother John melody (p to play, s to stop)
🔇 Mute and volume control (m to mute, i to increase, d to decrease, max volume: 20, min: 1)
🎚 Adjust tempo (set a value between 60-240 BPM using t)
🎵 Change pitch (values between -5 and 5 using k)
🎛 Switch between Conductor and Musician mode (TAB key)
⏳ Dynamic tempo switching (USER button interactions for real-time tempo adjustment)
