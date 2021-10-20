/**
 * Program      dcf77RadioClock.cpp
 * Author       2021-08-13 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Decodes the DCF77 timesignal input to an Arduino Uno on digital input pin 2.
 *              Each rising or falling edge of the timesignal triggers an interrupt.
 *              DCF77 receiver is from Conrad (641138 - 62) for CHF 16.95
 *              The time information is stored in the structure tm so that we can use 
 *              the function strftime() for formatted output.  
 *              The interrupt and decoding is handled in the class DCF77Decoder.
 *              A CLI menu allows to show the arriving bits of the time telegram
 *              or to print date and time from the struct tm. 
 * 
 * Board        Arduino Uno R3
 * 
 * Wiring          .------------------.  white                .----------------.
 *                 |               5V |-----------------------| 5V             |
 *                 |    DCF77         |  green                |        ARDUINO |
 *                 |              out |--->-------------------| GPIO2          |
 *                 |  Receiver        |  brown                |          UNO   |
 *                 |              GND |-----------------------| GND            |
 *                 `------------------´                       `----------------´ 
 * 
 * 
 * Remarks      The program is an adaption of an older version from 1999 written for the
 *              gameport of a Windows PC
 *              The program evaluates only the information about Standard or daylight saving 
 *              time, minutes, hours, calendar day, weekday, month and year. 
 *              Any leap seconds are neither displayed nor evaluated.
 *              The numeric values are BCD coded. The year is transferred without century.
 *              In the 59th second, the second pulse is dropped, which announces the 
 *              beginning of the full minute.
 * 
 *              MEZ        : 01 (standard time)
 *              MESZ       : 10 (daylight saving time)
 *              weekday    : 1..7 (monday..sunday)
 *              calenderday: 1..31
 *              month      : 1..12 (january..december)
 *              year       : 0..99
 *              
 *                                                  MEZ 39      9      5     6  3    16
 *                                 0--Meteo-Data--RazZA|mmmmmmmPhhhhhhPddddddwwwMMMMMyyyyyyyyP_                                       
 * Example      The DCF77 sequence 01001101001001000010110011100100100010100001111000011010001_
 *               is Sa 2016-03-05 09:39:00 MEZ
 * 
 * References   https://en.wikipedia.org/wiki/DCF77
 *              https://oar.ptb.de/files/download/56d6a9c0ab9f3f76468b45a7
 *              https://www.ptb.de/cms/en/ptb/fachabteilungen/abt4/fb-44/ag-442/dissemination-of-legal-time/dcf77/dcf77-time-code.html
 *              http://www.c-max-time.com/tech/dcf77.php
 *              and many others
 */
#include <Arduino.h>
#include <DCF77Decoder.h>
char buf[128];

#define CLEAR_LINE Serial.print("\r                                                                                                                        \r")

bool      timeFromStruct_tm  = false;
uint32_t  msEvery            = 5000;
const int PIN_DCF77INPUT     = 2;
const int PIN_DCF77INDICATOR = LED_BUILTIN;
tm        dcf77Time;

// Forward declaration of menu actions
void showTelegram();
void showDateTime();
void setPrintInterval();
void showMenu();

typedef struct { const char key; const char *txt; void (&action)(); } MenuItem;
MenuItem menu[] = 
{
  { 's', "[s] Show received time telegram",                  showTelegram },
  { 't', "[t] Show time from struct tm every interval sec" , showDateTime },
  { 'i', "[i] Set print interval [sec]",                     setPrintInterval },
  { 'S', "[S] Show menu",                                    showMenu },
};
constexpr int nbrMenuItems = sizeof(menu) / sizeof(menu[0]);

DCF77Decoder myDCF77(PIN_DCF77INPUT, PIN_DCF77INDICATOR, dcf77Time);

/**
 * Returns true, as soon as msWait milliseconds have passed.
 * Supply a reference to an uint32_t variable to hold 
 * the previous milliseconds.
 */
bool waitIsOver(uint32_t &msPrevious, uint32_t msWait)
{
  return (millis() - msPrevious >= msWait) ? (msPrevious = millis(), true) : false;
}

/**
 * Set the flag to print time telegram
 */
void showTelegram()
{
  myDCF77.setVerbose(true);
  timeFromStruct_tm = false;
}

/**
 * Set the flag to print time from struct tm
 */
void showDateTime()
{
  myDCF77.setVerbose(false);
  timeFromStruct_tm = true;
}

/**
 * Set the time interval to print
 * time from struct tm
 */
void setPrintInterval()
{
  int32_t value = 0;

  delay(2000);
  while (Serial.available())
  {
    value = Serial.parseInt();
  }
  msEvery = (value < 1) ? 1000 : value * 1000;
  Serial.print("Interval set to "); Serial.print(msEvery/1000); Serial.print(" sec");
  delay(1000);
  CLEAR_LINE;
}

void showMenu()
{
  // title is packed into a raw string
  Serial.print(
  R"TITLE(
-----------------
DCF77 Radio Clock
-----------------
)TITLE");

  for (int i = 0; i < nbrMenuItems; i++)
  {
  Serial.println(menu[i].txt);
  }
  Serial.print("\nPress a key: ");
}

/**
 * Get the keystroke from the operator and 
 * perform the corresponding action
 */
void doMenu()
{
  char key = Serial.read();
  CLEAR_LINE;
  for (int i = 0; i < nbrMenuItems; i++)
  {
  if (key == menu[i].key)
    {
    menu[i].action();
    break;
    }
  }
}

// Interrupt Service Routine
// Wrapper for interrupt handler hidden in class DCF77Decoder
void isr()
{
  myDCF77.handleInterrupt();  
} 

void initDCF77Decoder()
{
  myDCF77.setVerbose(true);  // Print time telegram
  attachInterrupt(digitalPinToInterrupt(PIN_DCF77INPUT), isr, CHANGE);
}

void setup()
{
  Serial.begin(115200);
  initDCF77Decoder();
  showMenu();
}

void loop()
{
  static uint32_t msPrevious = millis();

  myDCF77.loop(); // keep decoding signal received from DCF77

  // Print time in customized format when timeFromStruct_tm is 
  // set to true, which implies that setVerbose(false) is called.
  // The decoder fills in the time info into the tm structure 
  // you supplied in the constructor
  if (waitIsOver(msPrevious, msEvery) && myDCF77.isReady() && timeFromStruct_tm)
  {
    strftime(buf, sizeof(buf), "%a %F %T", &dcf77Time);
    Serial.println(buf);
  }
  if (Serial.available()) doMenu();
}
