
#ifndef _spark_h
#define _spark_h

#include <Arduino.h>
#include <cstdlib>


// A spark defines a target color to blend to from black for range values between 0 and limit
// and on to white from limit to uint16_max
class baseSpark {
public:
  typedef struct { uint8_t r, g, b; } color_t;

  baseSpark( uint16_t limit = 0xf000, color_t color = {0xff, 0xff, 0xff} );
  virtual ~baseSpark();

  // return the color blend mapped to the given partial interval value (between 0 and uint16_max)
  virtual bool get( uint16_t part, color_t &color ) const;

  virtual void reset();
  virtual void setLimit( uint16_t limit );

protected:
  color_t  _color;

private:
  uint16_t _limit;
};


// A spark with a random rainbow color (i.e one of rgb colors is max, one random and one 0)
class randomSpark : public baseSpark {
public:
  randomSpark( uint16_t limit = 0xf000 );
  void reset();
};


// A spark with a random color out of a given array of colors (theme)
class themedSpark : public baseSpark {
public:
  themedSpark( uint16_t limit = 0xf000 );
  void reset();

  static void setTheme( color_t colors[], uint16_t numColors );

private:
  static color_t *_colors;
  static uint16_t _numColors;
};


// Maps time intervals to range values of a spark
class timedSpark {
public:
  typedef baseSpark::color_t color_t;

  timedSpark( baseSpark &spark, uint16_t ms, uint16_t intervals = 1 );

  // Resets spark, if intervals are over.
  // Returns color for current time in interval or false if sparks get() fails
  bool get( color_t &color );

private:
  baseSpark &_spark;
  uint32_t _started;
  uint16_t _ms;
  uint16_t _intervals;
};


#endif
