#include <spark.h>


baseSpark::baseSpark( uint16_t limit, color_t color ) : _color(color), _limit(limit) {
}

baseSpark::~baseSpark() {
}

bool baseSpark::get( uint16_t part, color_t &color ) const {
  if( part < _limit ) { // fade in to random color
    color.r = map(part, 0, _limit, 0, _color.r);
    color.g = map(part, 0, _limit, 0, _color.g);
    color.b = map(part, 0, _limit, 0, _color.b);
  }
  else { // blend over to white
    color.r = map(part, _limit, 0xffff, _color.r, 0xff);
    color.g = map(part, _limit, 0xffff, _color.g, 0xff);
    color.b = map(part, _limit, 0xffff, _color.b, 0xff);
  }
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
  switch( map(rand(), 0, RAND_MAX, 0, 2) ) {
    case 0:
      _color.r = 0xff;
      _color.g = map(rand(), 0, RAND_MAX, 0, 0xff);
      _color.b = 0;
      break;
    case 1:
      _color.r = 0;
      _color.g = 0xff;
      _color.b = map(rand(), 0, RAND_MAX, 0, 0xff);
      break;
    case 2:
      _color.r = map(rand(), 0, RAND_MAX, 0, 0xff);
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
    _color = _colors[map(rand(), 0, RAND_MAX, 0, _numColors-1)];
  }
  else {
    _color.r = _color.g = _color.b = 0xff;
  }
}

void themedSpark::setTheme( const color_t colors[], uint16_t numColors ) {
  _colors = colors;
  _numColors = numColors;
}


timedSpark::timedSpark() : _pSpark(0), _ms(0), _intervals(0), _started(0) {
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
  if( _intervals && (now - _started) / _ms >= _intervals ) {
    _pSpark->reset();
  }

  return _pSpark->get(map((now - _started) % _ms, 0, _ms, 0, 0xffff), color);
}

void timedSpark::setSpark( baseSpark *pSpark, uint16_t ms, uint16_t intervals ) {
  _pSpark = pSpark;
  _ms = ms;
  _intervals = intervals;
  _started = millis();
}
