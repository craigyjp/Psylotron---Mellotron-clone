
#pragma GCC optimize ("O3")
#include <AltSoftSerial.h>
#include <MIDI.h>
#include <wavTrigger.h>
#include <avr/pgmspace.h>

#define EI_NOTINT0
#define EI_NOTINT1
#include <EnableInterrupt.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

//AltSoftSerial altSerial;
//MIDI_CREATE_INSTANCE(AltSoftSerial, altSerial, MIDI);
MIDI_CREATE_DEFAULT_INSTANCE();

wavTrigger wTrig;

// NE PAS OUBLIER DE METTRE BANK MAX A JOUR !!

struct constante {

  const int bankMax;      // 1100
  const char volumeMin;   // -90dB
  const char volumeMax;   // -12dB
  const char gainMax;     // -9dB
  const byte antiRebond;  // 250ms

} cst = {1900, -90, -12, -9, 250};

struct globalVar {

  uint32_t last_interrupt_time;
  int crossfade;
  float coef;             // Calculé dans le Setup, initialisé à 0
  float coef2;            // Calculé dans le Setup, initialisé à 0
  float coefGain;         // Calculé dans le Setup, initialisé à 0
  boolean bkAchange;
  boolean bkBchange;

} var = {0, 0, 0, 0, 0, 0, 0};

struct globalPin {

  // Analog
  const byte master;    //   ==> Analog 0
  const byte release;   //   ==> Analog 1
  const byte crossfade; //   ==> Analog 2

  // Digital
  const byte bankUpA;   //   ==> Digital 2  (Hardware INT0)
  const byte bankUpB;   //   ==> Digital 3  (Hardware INT1)
  const byte bankDnA;   //   ==> Digital 4
  const byte bankDnB;   //   ==> Digital 5

  const byte split;     //   ==> Digital 6
  const byte half;      //   ==> Digital 7

} pin = {0, 1, 2, 2, 3, 4, 5, 6, 7};

struct switchState {

  boolean half;
  boolean split;

} state = {0, 0};   // a degager

struct channel {
  // track = X X XX : channel / bank / note number
  volatile int bank;
  char volume;
  char volumeSplit;
  int tracks[12];

};

channel channelA;
channel channelB;

static void bankUpA() {
  uint32_t interrupt_time = millis();

  if (interrupt_time - var.last_interrupt_time > cst.antiRebond) {
    channelA.bank = (channelA.bank + 100) % (cst.bankMax + 100);
    var.bkAchange = 1;
  }
  var.last_interrupt_time = interrupt_time;
}

static void bankDnA() {
  uint32_t interrupt_time = millis();

  if (interrupt_time - var.last_interrupt_time > cst.antiRebond) {
    channelA.bank = ((cst.bankMax + 100) + (channelA.bank - 100)) % (cst.bankMax + 100);
    var.bkAchange = 1;
  }
  var.last_interrupt_time = interrupt_time;
}

static void bankUpB() {
  uint32_t interrupt_time = millis();

  if (interrupt_time - var.last_interrupt_time > cst.antiRebond) {
    channelB.bank = (channelB.bank + 100) % (cst.bankMax + 100);
    var.bkBchange = 1;
  }
  var.last_interrupt_time = interrupt_time;
}

static void bankDnB() {
  uint32_t interrupt_time = millis();

  if (interrupt_time - var.last_interrupt_time > cst.antiRebond) {
    channelB.bank = ((cst.bankMax + 100) + (channelB.bank - 100)) % (cst.bankMax + 100);
    var.bkBchange = 1;
  }
  var.last_interrupt_time = interrupt_time;
}

static int track(int bank, byte pitch)
{
  int track = bank + pitch;
  return track;
}

static char volume(int potard, boolean cross)
{
  float volume0;

  if (cross) {
    volume0 = sqrt(var.coef * potard) + cst.volumeMin ;   // potard = crossfade ou (1023-crossfade)
  }
  else {
    volume0 = sqrt(var.coef2 * potard) + cst.volumeMin ; // potard = volumePot
  }

  char volume = volume0;
  return volume ;
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  int trackA = track(channelA.bank, pitch);
  int trackB = track(channelB.bank, pitch) + 2000;

  if (state.split == LOW) {

    wTrig.trackGain(trackA, channelA.volume);       // sinon -70 pour attack
    wTrig.trackPlayPoly(trackA);

    wTrig.trackGain(trackB, channelB.volume);       // sinon -70 pour attack
    wTrig.trackPlayPoly(trackB);
  }
  else {

    trackA = trackA + 12;
    trackB = trackB - 12;

    if (pitch < 60) {

      wTrig.trackGain(trackA, channelA.volumeSplit);   // sinon -70 pour attack
      wTrig.trackPlayPoly(trackA);

    }

    else {

      wTrig.trackGain(trackB, cst.volumeMax);   // sinon -70 pour attack
      wTrig.trackPlayPoly(trackB);

    }
  }

  byte note = (pitch - 36) % 12;
  channelA.tracks[note] = trackA;
  channelB.tracks[note] = trackB;

}
void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  int releaseTime = analogRead(pin.release) * 3;
  int trackA = track(channelA.bank, pitch);
  int trackB = track(channelB.bank, pitch) + 2000;

  if (state.split == LOW) {

    if (releaseTime > 200) {
      wTrig.trackFade(trackA, cst.volumeMin, releaseTime, 1);
      wTrig.trackFade(trackB, cst.volumeMin, releaseTime, 1);
    }
    else {
      wTrig.trackStop(trackA);
      wTrig.trackStop(trackB);
    }
  }

  else {
    trackA = trackA + 12;
    trackB = trackB - 12;

    if (releaseTime > 200) {
      wTrig.trackFade(trackA, cst.volumeMin, releaseTime, 1);
      wTrig.trackFade(trackB, cst.volumeMin, releaseTime, 1);
    }
    else {

      wTrig.trackStop(trackA);
      wTrig.trackStop(trackB);
    }
  }

  byte note = (pitch - 36) % 12;
  channelA.tracks[note] = 0;
  channelB.tracks[note] = 0;

}

static void crossfader(int crossPot) {

  register byte note = 11;

  if (state.split == LOW) {

    if (crossPot < 512) {

      channelA.volume = cst.volumeMax;
      channelB.volume = volume(crossPot, 1);

    }
    else {

      channelB.volume = cst.volumeMax;
      channelA.volume = volume(1023 - crossPot, 1);

    }

    do {
      if (channelA.tracks[note]) {

        wTrig.trackGain(channelA.tracks[note], channelA.volume);
        wTrig.trackGain(channelB.tracks[note], channelB.volume);
      }
    } while (--note);
  }

  else {
    channelA.volumeSplit = volume(var.crossfade, 0) ;

    do {
      if (channelA.tracks[note]) {

        wTrig.trackGain(channelA.tracks[note], channelA.volumeSplit);
      }
    } while (--note);

  }
}

static void masterVolume(int volumePot)
{
  char masterVolume = sqrt(var.coefGain * volumePot) + cst.volumeMin ;
  wTrig.masterGain(masterVolume);
}

void line1() {

  lcd.setCursor(0, 0);
  switch (channelA.bank)
  {
    case 0:
      lcd.print(F("A00: Strings    "));
      break;

    case 100:
      lcd.print(F("A01: Flutes     "));
      break;

    case 200:
      lcd.print(F("A02: Choir      "));
      break;

    case 300:
      lcd.print(F("A03: Strings    "));
      break;

    case 400:
      lcd.print(F("A04: Vibes      "));
      break;

    case 500:
      lcd.print(F("A05: Organ      "));
      break;

    case 600:
      lcd.print(F("A06: Bassoon    "));
      break;

    case 700:
      lcd.print(F("A07: Birotron   "));
      break;

    case 800:
      lcd.print(F("A08: Fairlight  "));
      break;

    case 900:
      lcd.print(F("A09: Bass       "));
      break;

    case 1000:
      lcd.print(F("A10: Strings 2  "));
      break;

    case 1100:
      lcd.print(F("A11: Flutes     "));
      break;

    case 1200:
      lcd.print(F("A12: Choir      "));
      break;

    case 1300:
      lcd.print(F("A13: Strings    "));
      break;

    case 1400:
      lcd.print(F("A14: Vibes      "));
      break;

    case 1500:
      lcd.print(F("A15: Church Orga"));
      break;

    case 1600:
      lcd.print(F("A16: Bassoon    "));
      break;

    case 1700:
      lcd.print(F("A17: Birotron   "));
      break;

    case 1800:
      lcd.print(F("A18: Fairlight  "));
      break;

    case 1900:
      lcd.print(F("A19: Bass       "));
      break;

  }
}

void line2() {

  lcd.setCursor(0, 1);
  switch (channelB.bank)
  {
    case 0:
      lcd.print(F("B00: Strings    "));
      break;

    case 100:
      lcd.print(F("B01: Flutes     "));
      break;

    case 200:
      lcd.print(F("B02: Choir      "));
      break;

    case 300:
      lcd.print(F("B03: Strings    "));
      break;

    case 400:
      lcd.print(F("B04: Vibes      "));
      break;

    case 500:
      lcd.print(F("B05: Organ      "));
      break;

    case 600:
      lcd.print(F("B06: Bassoon    "));
      break;

    case 700:
      lcd.print(F("B07: Birotron   "));
      break;

    case 800:
      lcd.print(F("B08: Fairlight  "));
      break;

    case 900:
      lcd.print(F("0B9: Bass       "));
      break;

    case 1000:
      lcd.print(F("B10: Strings    "));
      break;

    case 1100:
      lcd.print(F("B11: Flutes     "));
      break;

    case 1200:
      lcd.print(F("B12: Choir      "));
      break;

    case 1300:
      lcd.print(F("B13: Strings    "));
      break;

    case 1400:
      lcd.print(F("B14: Vibes      "));
      break;

    case 1500:
      lcd.print(F("B15: Church Orga"));
      break;

    case 1600:
      lcd.print(F("B16: Bassoon    "));
      break;

    case 1700:
      lcd.print(F("B17: Birotron   "));
      break;

    case 1800:
      lcd.print(F("B18: Fairlight  "));
      break;

    case 1900:
      lcd.print(F("B19: Bass       "));
      break;
  }

}

void setup()
{
  // Pin Mode
  pinMode(pin.bankUpA, INPUT_PULLUP);
  pinMode(pin.bankDnA, INPUT_PULLUP);
  pinMode(pin.bankUpB, INPUT_PULLUP);
  pinMode(pin.bankDnB, INPUT_PULLUP);

  pinMode(pin.half, INPUT_PULLUP);
  pinMode(pin.split, INPUT_PULLUP);

  // Interruptions
  attachInterrupt(0,  bankUpA, FALLING);
  attachInterrupt(1,  bankUpB, FALLING);
  enableInterrupt(4,  bankDnA, FALLING);
  enableInterrupt(5,  bankDnB, FALLING);

  // MIDI Init
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.turnThruOff ();



  // Wav Trigger Init
  wTrig.start();
  //wTrig.setAmpPwr(0);
  wTrig.masterGain(cst.gainMax);


  // Banks
  channelA.bank = 0;         // 0-> 9 ==> 000 -> 900
  channelB.bank = 200;       // Bank Choir

  // Volumes
  channelA.volume = cst.volumeMax;      // -9
  channelB.volume = cst.volumeMax;      // -9
  channelA.volumeSplit = cst.volumeMax; // -9
  var.coef = (((float) cst.volumeMin - cst.volumeMax) * (cst.volumeMin - cst.volumeMax)) / 512;
  var.coef2 = (((float) cst.volumeMin - cst.volumeMax) * (cst.volumeMin - cst.volumeMax)) / 1024;
  var.coefGain = (((float) cst.volumeMin - cst.gainMax) * (cst.volumeMin - cst.gainMax)) / 1024;

  // initialize the LCD
  lcd.init();

  // Turn on the blacklight and print a message.
  //lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(F("   FAR BEYOND   "));

  lcd.setCursor(0, 1);
  lcd.print(F("   PERCEPTION   "));
  delay(1000);

  lcd.setCursor(0, 1);
  lcd.print(F("                "));

  lcd.setCursor(0, 0);
  lcd.print(F("   PSYL()TRON"));
  delay(250);

  lcd.setCursor(0, 0);
  lcd.print(F("   PSY(  )RON"));
  delay(250);

  lcd.setCursor(0, 0);
  lcd.print(F("   PS(    )ON"));
  delay(250);

  lcd.setCursor(0, 0);
  lcd.print(F("   P(      )N"));
  delay(250);

  lcd.setCursor(0, 0);
  lcd.print(F("   (        )"));
  delay(250);

  line1();
  line2();
}

void loop()
{

  MIDI.read();

  if (var.bkAchange) {
    wTrig.stopAllTracks();
    line1();
    var.bkAchange = 0;
  }

  if (var.bkBchange) {
    wTrig.stopAllTracks();
    line2();
    var.bkBchange = 0;
  }

  MIDI.read();

  // Pin Reading
  state.half = digitalRead(pin.half);
  state.split = digitalRead(pin.split);


  // Master Volume
  int volumePot = analogRead(pin.master);
  static int lastVolumePot = 0;
  if (abs(volumePot - lastVolumePot) > 8) {
    masterVolume(volumePot);
    lastVolumePot = volumePot;
  }


  // Cross Fade / SPLIT
  var.crossfade = analogRead(pin.crossfade) ;
  static int lastCrossFade = 0;
  if (abs(var.crossfade - lastCrossFade) > 8) {
    crossfader(var.crossfade) ;
    lastCrossFade = var.crossfade;
  }

  MIDI.read();

  static int melloPitch = 0;

  //Half Speed
  if (state.half == HIGH && melloPitch != -32768) {

    melloPitch = melloPitch - 128;
    wTrig.samplerateOffset(melloPitch);

  }

  //Normal Speed
  if (state.half == LOW && melloPitch != 0) {

    melloPitch = melloPitch + 128;
    wTrig.samplerateOffset(melloPitch);
  }
}
