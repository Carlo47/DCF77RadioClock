/**
 * Class        DCF77Decoder.cpp
 * Author       2021-08-13 Charles Geiser (https://www.dodeka.ch)
 *
 * Purpose      Decodes the time telegram received from the time signal 
 *              transmitter DCF77 located at Mainflingen near Frankfurt (Germany)    
 * 
 * Board        Arduino Uno R3
 * 
 * Remarks      Uses a change interrupt on an input pin
 * 
 * References   https://oar.ptb.de/files/download/56d6a9c0ab9f3f76468b45a7 
 */

#include <DCF77Decoder.h>

DCF77Decoder::DCF77Decoder(int dcf77InputPin, int dcf77IndicatorPin, tm &dcf77Time) : 
  _inputPin(dcf77InputPin), _indicatorPin(dcf77IndicatorPin), _dcf77Time(dcf77Time)
{
	pinMode(_inputPin, INPUT);
	pinMode(_indicatorPin, OUTPUT);
}

/**
 * Called when an interrupt signals a newly detected edge.
 * Evaluates the measured pulse width and fills the dcf77Bits
 * with 0 and 1 accordingly.
 * A longer gap between 2 pulses is interpreted as the beginning of 
 * a new minute and the counting of seconds restarts with 0.
 */
bool DCF77Decoder::collectBits()
{
  if (_newEdge) 
  {
    _newEdge = false;

    if (_edgeMode == EDGE_RISING) 
    { // Pulse begins and pause ends
      _startPulse = millis();
      _widthPause = _startPulse - _endPulse;

      if (_widthPause > (MIN_SYNCGAP - JITTER) && _widthPause < (MAX_SYNCGAP + JITTER)) 
      {
        _synchronized = true;
        _seconds = 0;
        _dcf77Time.tm_sec = 0;
        return (true);
      }
    }

    if (_edgeMode == EDGE_FALLING) 
    { // Pulse ends and pause begins
      _endPulse = millis();
      _widthPulse = _endPulse - _startPulse;
      
      if (_synchronized) 
      { // Clock is synchronized
        if (_widthPulse > (P0 - JITTER) && _widthPulse < (P0 + JITTER)) 
        {
          _dcf77Bits[_seconds] = '0';
          if (_verbose) Serial.print(0);
        }
        if (_widthPulse > (P1 - JITTER) && _widthPulse < (P1 + JITTER)) 
        {
          _dcf77Bits[_seconds] = '1';
          if (_verbose) Serial.print(1);
        }
		    digitalWrite(_indicatorPin, !digitalRead(_indicatorPin));
        _seconds++;
        _dcf77Time.tm_sec++;
      }
      else
      {
        // Clock is synchronizing, seconds still unknown
        if (_verbose) Serial.print("*");
        //if (_verbose) Serial.print(_dcf77Bits[58]);
      }
    }
  }
  return false;	
}

/**
 * Calculate value from bcd coded bits starting 
 * at firstBit and composed of nbrBits
 */
int DCF77Decoder::getValueFromBits(int firstBit, int nbrBits)
{
  int value = 0;
  for (int i = firstBit; i < firstBit + nbrBits; i++)
  {
    value += ((int)_dcf77Bits[i] - (int)'0') * _bcdValue[i - firstBit];
  }
  return value;
}

/**
 *  Decode the whole time telegram
 */
void DCF77Decoder::decodeBits()
{
  int value = getValueFromBits(17, 2); // standard or dayligt saving time
  if (value == 2) 
  {
    _z12 = value;
    _dcf77Time.tm_isdst = 0;  // standard time (MEZ)
  }
  else if (value == 1) 
  {
    _z12 = 1;
    _dcf77Time.tm_isdst = 1;  // daylight saving (MESZ))
  }
  else
  {
    _z12 = 0;
    _dcf77Time.tm_isdst = -1; // no information available
  };
  _dcf77Time.tm_sec  = 0; // Seconds are always 0
  _dcf77Time.tm_min  = getValueFromBits(21, 7);
  _dcf77Time.tm_hour = getValueFromBits(29, 6);
  _dcf77Time.tm_mday = getValueFromBits(36, 6);
  _dcf77Time.tm_wday = getValueFromBits(42, 3);
  _dcf77Time.tm_mon  = getValueFromBits(45, 5) - 1;
  _dcf77Time.tm_year = getValueFromBits(50, 8) + 100;
  snprintf(_dcf77TimeString, sizeof(_dcf77TimeString), DCF77TIMEFORMAT, 
        _weekDay[_dcf77Time.tm_wday], 
        _dcf77Time.tm_year - 100, 
        _dcf77Time.tm_mon + 1, 
        _dcf77Time.tm_mday, 
        _dcf77Time.tm_hour, 
        _dcf77Time.tm_min, 
        _dcf77Time.tm_sec, 
        _timeZone[_z12]);    // time zone flags: 2 = MESZ, 1 = MEZ, 0 = no information available
}

/**
 *  Checks parity to verify validity of received DCF77 information
 */
bool DCF77Decoder::parityOK() 
{
  int sum = 0;

  for (int i = 21; i < 29; i++) {  // parity minutes
    sum += ((int)_dcf77Bits[i] - (int)'0');
  }

  for (int i = 29; i < 36; i++) {  // parity hours
    sum += ((int)_dcf77Bits[i] - (int)'0');
  }

  for (int i = 36; i < 59; i++) {  // parity date
    sum += ((int)_dcf77Bits[i] - (int)'0');
  }
  return ((sum % 2) == 0) ? true : false;
}

/**
 *  Interrupt handler detects rising or falling edge of received pulse
 */
void DCF77Decoder::handleInterrupt()
{
	_edgeMode = digitalRead(_inputPin);
	_newEdge = true;
}

/**
 * Print decoded time string formatted
 * with DCF77TIMEFORMAT
 */
void DCF77Decoder::printDateTime()
{
  Serial.println(_dcf77TimeString);  
}

/**
 * Set this flag to print arriving
 * time telegram from receiver
 */
void DCF77Decoder::setVerbose(bool verbose)
{
  _verbose = verbose;
}

/**
 * The time telegram has been received completely if the
 * last character in the initial string is no longer 'P'.
 */
bool DCF77Decoder::isReady()
{
  return (_dcf77Bits[58] == 'P') ? false : true;
}

void DCF77Decoder::loop()
{
  if (collectBits() == true)
  {
    if (parityOK())
    {
      decodeBits();
      if (_verbose) printDateTime();
    } 
    else 
    {
      Serial.println(" Parity check failed, continue collecting time info..."); 

      Serial.println("012345678901234567890123456789012345678901234567890123456789 ");     
      Serial.println("0--Meteo-Data--RazZA|mmmmmmmPhhhhhhPddddddwwwMMMMMyyyyyyyyP_ ");
                     
    }
  }
}