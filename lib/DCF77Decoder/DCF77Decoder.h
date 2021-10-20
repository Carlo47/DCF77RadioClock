/**
 * Header       DCF77Decoder.h
 * Author       2021-08-13 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Declaration of the class DCF77Decoder
 * 
 * Constructor
 * arguments    int dcf77InputPin      	input pin which gets pulses from receiver
 *              int dcf77IndicatorPin  	output pin which is toggled every second 
 *              tm  &dcf77Time         	a reference to a struct tm which gehts 
 *                                      updated every second by the decoder 
 */

#include <Arduino.h>
#include <time.h>
#ifndef _DCF77Decoder_H_
#define _DCF77Decoder_H_

#define EDGE_RISING  HIGH
#define EDGE_FALLING LOW
#define P0          100      // Pulse width of 100 ms means bit = 0
#define P1          200      // Pulse width of 200 ms means bit = 1
#define JITTER      35       // Uncertainty of measured pulse width
#define MIN_SYNCGAP 1800     // Minimal synchronization gap at sec 59 
#define MAX_SYNCGAP 1900     // Maximal synchronization gap at sec 59
#define DCF77TIMEFORMAT "%3s 20%02d-%02d-%02d %02d:%02d:%02d %4s DCF77"

/*
  DCF77 numbering conventions 
  weekday 1 is Monday
  weekday 7 is Sunday
  month   1 is january
  month  12 is december 
  year   00..99 (no century)


  Numbering convention used for struct tm
  struct tm
  {
    int	tm_sec;   // 0..59
    int	tm_min;   // 0..59
    int	tm_hour;  // 0..23 hours since midnight
    int	tm_mday;  // 1..31 day of month
    int	tm_mon;   // 0..11 January=0 .. December=11            DCF77 Jan=1 .. Dez=12
    int	tm_year;  // years since 1900                          DCF77 Year without century
    int	tm_wday;  // 0..6  weekday, Sunday=0 .. Saturday=6     DCF77 Mo=1 ... So=7
    int	tm_yday;  // 0..365 day in the year, January 1 = 0
    int	tm_isdst; //  0: Standard time, 
                  // >0: Daylight saving time, 
                  // <0: information not available 
  }
*/

class DCF77Decoder
{
  public:
    DCF77Decoder(int dcf77InputPin, int dcf77IndicatorPin, tm &dcf77Time);
    void loop();
    void handleInterrupt();
    void printDateTime();
    void setVerbose(bool verbose);
    bool isReady();

  private:
    bool collectBits();
  	int  getValueFromBits(int firstbit, int nbrBits);
    void decodeBits();
	  bool parityOK();
    volatile int  _inputPin;
	  volatile int  _edgeMode = -1;    // value set by interrupt handler, 0 falling, 1 rising
	  volatile bool _newEdge = false;  // value set by interrupt handler
	  uint32_t   _startPulse = 0;      // is also end of pause
	  uint32_t   _endPulse = 0;        // is also start of pause
	  int        _indicatorPin;
	  int        _widthPulse = 0;
	  int        _widthPause = 0;
	  int 	     _bcdValue[8] = {1, 2, 4, 8, 10, 20, 40, 80};
	  bool       _synchronized = false;
	  bool       _verbose = true;
	  int        _seconds = 0;
	  int        _z12 = 0; // 0 = no information available, 1 = MESZ, 2 = MEZ
  	//                           0        10        20        30        40        50        60
    //                          "0....:....:....:....:....:....:....:....:....:....:....:....:"
	  char       _dcf77Bits[62] = "0--Meteo-Data--RazZA|mmmmmmmPhhhhhhPddddddwwwMMMMMyyyyyyyyP_";
  	char       _dcf77TimeString[40];
	  const char *_weekDay[8]  = { "--", "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So" };
	  const char *_timeZone[3] = { "---", "MESZ", "MEZ" };    
  	tm         &_dcf77Time;
};
#endif
