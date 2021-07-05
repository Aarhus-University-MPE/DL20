/*
    Revised drill logger software, original code by
      Emil Behrens Hansen, Jens Johan Lage Jørgensen,
      Mads Bjerregaard Jacobsen and Mathias Egholm Pedersen

    Modified by
      Mads Rosenhoej Jeppesen - Aarhus 2021
      mrj@mpe.au.dk

    Version update:
      Fixed sensor data communication (data reception was getting scrambled)
      Added manual override (Button press during startup), halts system
*/

/*------------------------------------------------------
                     Defines
  -------------------------------------------------------*/

#define Modem_serial Serial
#define ODM3_serial Serial1
#define IDS4000_1_SERIAL Serial2
#define IDS4000_2_SERIAL Serial3

/*------------------------------------------------------
                     Inkludere biblioteker
  -------------------------------------------------------*/

#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SPI.h>
#include <SD.h>
#include <stdlib.h>

/*------------------------------------------------------
                     Opsætning I2C adresse
  -------------------------------------------------------*/

Adafruit_ADS1115 ads(0x48); //800sample/s max 100kHz max speed for arduino

/*------------------------------------------------------
                     Prototyper
  -------------------------------------------------------*/

void aflaes_transducer_1();
void aflaes_transducer_2();
void aflaes_temperatur();
void process_data (const char * data);
void processIncomingByte (const byte inByte);
void initTimer3();
void aflaes_tryknap();
void til_steng();
void gem_samlet_streng();

/*------------------------------------------------------
                     Opretter variabler
  -------------------------------------------------------*/

//Opretter struct til at samle data. De oprettes en
// tilknyttede string til de datetyper der ikke i
// forvenjen er en string.

struct state {

  unsigned int sending;
  String sending_string;

  //ISD4000_1
  String ISD4000_1;

  //ISD4000_2
  String ISD4000_2;

  //ODM3
  String ODM;

  //Transdoucer 1
  int transdoucer_1;
  String transdoucer_1_string;

  //Transdoucer 2
  int transdoucer_2;
  String transdoucer_2_string;

  //Temperatur
  unsigned long temperatur_spaending;
  String temperatur_spaending_string;

  //Trykknap
  int trykknap;
  String trykknap_string;

  //Samlet streng
  String SamletDataPakke;
} state;

//Trykknap - Aflæsnings pin
const int Aflaesning_knap = 6;

//Impact Subsea - Maksimalt antal karaktere der kan
// læses fra UART bufferen
const unsigned int MAX_INPUT = 512;

//Opsætning af timer 0
// Opretter variable til timer antal overflows
volatile unsigned int antal_overflows = 0;
// Variabel til indikation af data klar
int data_klar = 0;
// Variabel til timing af data sending til overfalde
int data_til_overflade = 0;

//SD kort
const int chipSelect = 53;
String filename;

// Time between each logged data
const unsigned int sampleDt = 500;
unsigned long prevSample;
const unsigned int surfaceFrequency = 1; // every xth sample also sent to surface
bool dataReady[3];



/*------------------------------------------------------
                     Setup
  -------------------------------------------------------*/

void setup(void)
{
  //Kommunikation
  //Opretter serial kommunikation 8N1 600 til
  //Computer/transformer
  Modem_serial.begin(600);
  //Serial.begin(115200);
  data_til_overflade = surfaceFrequency;

  //Trykknap
  pinMode(Aflaesning_knap, INPUT);

  delay(10);
  if (!digitalRead(Aflaesning_knap)) {
    delay(1000);
    if (!digitalRead(Aflaesning_knap)) {
      // Set SPI pins as INPUT
      pinMode(50, INPUT);
      pinMode(51, INPUT);
      pinMode(52, INPUT);
      pinMode(53, INPUT);
      while (1);
    }
  }

  //Opretter serial kommunikation 8N1 9600 til ODM3
  ODM3_serial.begin(9600);
  //Opretter serial kommunikation 8N1 9600 til
  // ISD4000 1
  IDS4000_1_SERIAL.begin(9600);
  //Opretter serial kommunikation 8N1 9600 til
  // ISD4000 2
  IDS4000_2_SERIAL.begin(9600);

  //Temperatur - Initialisere ADS1015
  ads.begin();


  //Klargoer SD kort
  //Initialisere SD kort
  SD.begin(chipSelect);
  //Opretter fil, hvis der ikke er en  forvejen
  unsigned int fileNameExtension = 0;
  while (SD.exists("Data_" + (String)fileNameExtension + ".txt")) fileNameExtension++;
  filename = "Data_" + (String)fileNameExtension + ".txt";

  File datafil = SD.open(filename, FILE_WRITE);

  //Lukker fil
  datafil.close();
}

/*------------------------------------------------------
                     Funktioner
  -------------------------------------------------------*/
//Funktion til at gemme data på SD kort
void gem_samlet_streng()
{
  //Åbner fil
  File datafil = SD.open(filename, FILE_WRITE);
  //Skriver til fil
  datafil.println(state.SamletDataPakke);
  //Lukker fil
  datafil.close();
}

//Funktion til at konvertere forskellige datatyper
// til strenge
void til_steng()
{
  state.sending_string = String(state.sending);
  state.transdoucer_1_string = String(state.transdoucer_1);
  state.transdoucer_2_string = String(state.transdoucer_2);
  state.temperatur_spaending_string = String(state.temperatur_spaending);
  state.trykknap_string = String(state.trykknap);
}

//Funktion til at samle state til en streng
void samle_streng()
{
  state.SamletDataPakke = "";
  state.SamletDataPakke += state.sending_string;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.transdoucer_1_string;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.transdoucer_2_string;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.temperatur_spaending_string;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.trykknap_string;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.ISD4000_1;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.ISD4000_2;
  state.SamletDataPakke += "\t";
  state.SamletDataPakke += state.ODM;

}

//Funktion til at sende data til overfladen
void send_til_overflade()
{
  Modem_serial.println(state.SamletDataPakke);
  Modem_serial.println();
  //Serial.println(state.SamletDataPakke);
  //Serial.println();
}

// Funktion til at gemme steng fra sensor
void process_data (const char * data, const int id)
{
  dataReady[id] = true;
  if (id == 0)
  {
    state.ODM = data;
  }
  if (id == 1)
  {
    state.ISD4000_1 = data;
  }
  if (id == 2)
  {
    state.ISD4000_2 = data;
  }
}

//Funktion til procering af byte fra UART
void processIncomingByte (const byte inByte, const int nummer)
{
  //Intialisere variable
  static char input_line [3][MAX_INPUT];
  static unsigned int input_pos[3];
  static bool validData[3];
  const int id = nummer;

  //Switch afgør byte værdi
  switch (inByte)
  {
    case '$':
      input_pos[id] = 0;
      validData[id] = true;
      break;
    //Case hvor byteværdien er \n og dermed
    // slutningen på en string
    case '\n':
      // terminating null byte
      input_line [id][input_pos[id]] = 0;
      // Dataarbejde udføres
      if (validData[id]) process_data (input_line[id], id);
      validData[id] = false;
      // Bufferen resættes
      input_pos[id] = 0;
      break;

    case '\r':
      break;

    case  '*':
      if (input_pos[id] < (MAX_INPUT - 1))
        input_line [id][input_pos[id]++] = ' ';
      break;

    //Default case - benyttes til nye værdier
    // indtil slutningen af string
    default:
      if (input_pos[id] < (MAX_INPUT - 1))
        input_line [id][input_pos[id]++] = inByte;
      break;
  }
  //delay(2);
}

// Funktion til at aflæse spænding transdoucer 1
void aflaes_transducer_1()
{
  state.transdoucer_1 = ads.readADC_SingleEnded(1);
  delay(1);
}

// Funktion til at aflæse spænding transdoucer 2
void aflaes_transducer_2()
{
  state.transdoucer_2 = ads.readADC_SingleEnded(2);
  delay(1);
}

//Funktion til initialisering af timer3
void initTimer3()
{
  // Timer0: Normal mode, PS = 1 (= "Ingen prescaling")
  TCCR3A = 0b00000000;
  TCCR3B = 0b00000001;
  // Enable Timer0 overflow interrupt
  TIMSK3 |= 0b00000001;
}

//Funktion til at aflæse trykknap
void aflaes_tryknap()
{
  state.trykknap = !digitalRead(Aflaesning_knap);
  delay(1);
}

//Funktion til at aflæse temperatur spænding
void aflaes_temperatur()
{
  state.temperatur_spaending = ads.readADC_SingleEnded(0);
  delay(1);
}

/*------------------------------------------------------
                     Hovedeløkke
  -------------------------------------------------------*/
void loop(void)
{
  //Læser fra ODM3
  while (ODM3_serial.available () > 1) {
    processIncomingByte (ODM3_serial.read(), 0);
  }
  //Læser fra ISD4000 1
  while (IDS4000_1_SERIAL.available () > 1) {
    processIncomingByte (IDS4000_1_SERIAL.read(), 1);
  }
  //Læser fra ISD4000 2
  while (IDS4000_2_SERIAL.available () > 1) {
    processIncomingByte (IDS4000_2_SERIAL.read(), 2);
  }


  if (millis() - prevSample > sampleDt && dataReady[0] && dataReady[1] && dataReady[2]) {
    prevSample = millis();

    aflaes_temperatur();
    aflaes_tryknap();
    aflaes_transducer_1();
    aflaes_transducer_2();
    til_steng();
    samle_streng();
    state.sending++;
    gem_samlet_streng();

    dataReady[0] = false;
    dataReady[1] = false;
    dataReady[2] = false;

    data_til_overflade++;
    if (data_til_overflade >= surfaceFrequency)
    {
      send_til_overflade();
      data_til_overflade = 0;
    }
    else {
    }
  }
  else {
    delay(10);
  }
}
