/*
  syslog makros
*/

#ifndef Logger_h
#define Logger_h

#include <WiFiUdp.h>
#include <Syslog.h>

#ifdef LOGGER
#define ERR(fmt, ...)     if( WiFi.status() == WL_CONNECTED ) logger._syslog.logf(LOG_ERR,     fmt, ## __VA_ARGS__)
#define WARNING(fmt, ...) if( WiFi.status() == WL_CONNECTED ) logger._syslog.logf(LOG_WARNING, fmt, ## __VA_ARGS__)
#define NOTICE(fmt, ...)  if( WiFi.status() == WL_CONNECTED ) logger._syslog.logf(LOG_NOTICE,  fmt, ## __VA_ARGS__)
#define INFO(fmt, ...)    if( WiFi.status() == WL_CONNECTED ) logger._syslog.logf(LOG_INFO,    fmt, ## __VA_ARGS__)
#define TRACE(fmt, ...)   if( WiFi.status() == WL_CONNECTED ) logger._syslog.logf(LOG_DEBUG,   fmt, ## __VA_ARGS__)
#else
#define ERR(fmt, ...)     if( 0 ) {}
#define WARNING(fmt, ...) if( 0 ) {}
#define NOTICE(fmt, ...)  if( 0 ) {}
#define INFO(fmt, ...)    if( 0 ) {}
#define TRACE(fmt, ...)   if( 0 ) {}
#endif

class Logger {
  public:
    Logger( const char *server, const char *app, int16_t prio ) : _udp(), _syslog(_udp, SYSLOG_PROTO_IETF) {
      _syslog.server(server, 514);
      _syslog.appName(app);
      _syslog.defaultPriority(LOG_KERN);
      _syslog.logMask(LOG_UPTO(prio));
    };

  private:
    Logger();
    Logger( const Logger & );
    
    WiFiUDP _udp;
  
  public:
    Syslog _syslog;
};

extern Logger logger;

#endif
