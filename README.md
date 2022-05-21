# Psylotron---Mellotron-clone
Arduino and wav trigger based Mellotron clone based on the Psylotron by Far Beyond Perception Instruments

http://www.djsoulmachine.com/farbeyondperception/index.html

A clone of the classic Mellotron keyboard

Arduino Nano/Uno based Mellotron sample playback device using a robertsonics wav trigger, although I'm sure it will also work on the later Tsunami wav trigger device for more polyphony and ability to play mono and stereo samples.

This was built into an old Elektronika EM-25 Soviet keyboard chassis with an M-Audio KeyRig 49 Keybed added on 3D printed spacers.
A 3D panel was printed to hold the controls and LCD display.

Components used:-

Arduino Uno or Nano.

Suitable opto coupler for MIDI input (4n35 in this case).

Adafruit (Robertsonics) wav trigger sample playback board.

i2c 1602 LCD display or OLED with an i2c interface attached.

Two toggle switches.

4 momentary push buttons (led surrounds optional).

1 latching push button for power (led surround).

3 10k lin rotary pots 

Optional for low pass filter or tone control.

Suitable stereo or mono depending on your tastes tone control circuit for high/low boost or low pass filter.

Optional for keyboard version.

3 push buttons with LED surrounds.

Suitable 4 octave MIDI DIN keyboard and chassis and associated 3D printed parts for intergration

SAMPLE LAYOUT

Mellotron samples must be around in groups starting at the lowest note 0043_sample1_name.wav to 0077_sample1_name.wav, 0143_sample2_name.wav to 0177_sample2_name.wav etc for the bank A and then 2043_sample1_name.wav to 2077_sample1_name.wav, 2143_sample2_name.wav to 2177_sample2_name.wav etc for the Bank B sounds. Bank A and Bank B sounds are seperated by 2000 sample slots.

Each MIDI note 43 to 77 is mapped to a sample slot on the wav trigger and adding 100 to the slot changes the sample played back, thus changing the instrument. The Wav Trigger has 4096 slots available with the latest 1.34 firmware. This effectively can give you 40 unique onboard sounds to mix and split across the keyboard. Its up to you to put the samples in the slots you want them and change the menus to suit.

Samples must be stereo, 16 bit, 44.1 khz and contain no metadata.
