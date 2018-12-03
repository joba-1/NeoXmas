#include <spark.h>

#include <stdlib.h>


baseSpark::baseSpark( uint16_t limit, color_t color ) : _color(color), _limit(limit) {
}

baseSpark::~baseSpark() {
}

bool baseSpark::get( uint16_t part, color_t &color ) const {
  uint32_t r, g, b;

  // mirror animation
  // TODO move to timedSpark
  if( part > 0xffff / 2 )
    part = (0xffff - part) * 2;
  else
    part *= 2;

  if( part < _limit ) { // fade in to random color (r, g, b go to max 0xffff)
    r = ((uint32_t)part * _color.r * 0xff) / _limit;
    g = ((uint32_t)part * _color.g * 0xff) / _limit;
    b = ((uint32_t)part * _color.b * 0xff) / _limit;
  }
  else { // blend over to white (=3*0xffff)
    r = (((uint32_t)part - _limit) * (0xff - _color.r) * 0xff) / (0xffff - _limit) + (uint32_t)_color.r * 0xff;
    g = (((uint32_t)part - _limit) * (0xff - _color.g) * 0xff) / (0xffff - _limit) + (uint32_t)_color.g * 0xff;
    b = (((uint32_t)part - _limit) * (0xff - _color.b) * 0xff) / (0xffff - _limit) + (uint32_t)_color.b * 0xff;
  }
  // squared increase (slow for low values, fast for high values)
  // Color change looks better / more even
  color.r = (uint8_t)((r*r)>>24);
  color.g = (uint8_t)((g*g)>>24);
  color.b = (uint8_t)((b*b)>>24);

  /*
  if( millis() < 12000 )
  Serial.printf("%04x/%04x * (%02x/%02x/%02x) = (%02lx/%02lx/%02lx)\n", part, _limit,
   _color.r, _color.g, _color.b, color.r, color.g, color.b);
  //  _color.r, _color.g, _color.b, r, g, b);
  */
  return true;
}

void baseSpark::reset() {
}

void baseSpark::setLimit( uint16_t limit ) {
  _limit = limit;
}


randomSpark::randomSpark( uint16_t limit ) : baseSpark(limit) {
  reset();
}

void randomSpark::reset() {
  switch( map(rand() % 0xffff, 0, 0xffff, 0, 2) ) {
    case 0:
      _color.r = 0xff;
      _color.g = map(rand() % 0xffff, 0, 0xffff, 0, 0xff);
      _color.b = 0;
      break;
    case 1:
      _color.r = 0;
      _color.g = 0xff;
      _color.b = map(rand() % 0xffff, 0, 0xffff, 0, 0xff);
      break;
    case 2:
      _color.r = map(rand() % 0xffff, 0, 0xffff, 0, 0xff);
      _color.g = 0;
      _color.b = 0xff;
      break;
  }
}


const themedSpark::color_t *themedSpark::_colors = 0;
uint16_t themedSpark::_numColors = 0;

themedSpark::themedSpark( uint16_t limit ) : baseSpark(limit) {
  reset();
}

void themedSpark::reset() {
  if( _numColors && _colors ) {
    _color = _colors[map(rand() % 0xffff, 0, 0xffff, 0, _numColors-1)];
  }
  else {
    _color.r = _color.g = _color.b = 0xff;
  }
}

void themedSpark::setTheme( const color_t colors[], uint16_t numColors ) {
  _colors = colors;
  _numColors = numColors;
}


timedSpark::timedSpark() : _pSpark(0), _msMin(0), _ms(0), _intervals(0), _started(0) {
}

timedSpark::timedSpark( baseSpark *pSpark, uint16_t ms, uint16_t intervals ) {
  setSpark(pSpark, ms, intervals);
}

bool timedSpark::get( color_t &color ) {
  if( !_pSpark || !_ms ) {
    color.r = color.g = color.b = 0xff;
    return true;
  }

  uint32_t now = millis();
  if( _intervals && (((now - _started) / _ms) >= _intervals) ) {
    _ms = _msMin + rand() % _msMin;
    _started = now;
    _pSpark->reset();
  }

  //Serial.printf("value %u, ms %u, map %06lx\n", (now - _started) % _ms, _ms, map((now - _started) % _ms, 0, _ms, 0, 0xffff));

  return _pSpark->get(map((now - _started) % _ms, 0, _ms, 0, 0xffff), color);
}

void timedSpark::setSpark( baseSpark *pSpark, uint16_t ms, uint16_t intervals ) {
  _pSpark = pSpark;
  _msMin = ms;
  _ms = _msMin + rand() % _msMin;
  _intervals = intervals;
  _started = millis();
}
