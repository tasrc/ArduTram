#include "arduTram.h"

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetClient.h>

namespace
{
  char parameterDivider = '-';
}

/*
  Konstruktor
*/
arduTram_c::arduTram_c() :
  _direction( DIRECTION_NONE ),
  _lastSequence( 0 ),
  _northTrackOccupied( false ),
  _southTrackOccupied( false ),
  _speedNorth( 100 ),
  _speedSouth( 100 ),
  _state( STATE_SELFTEST )
{
}

/*!
  Initialisierung
*/
void arduTram_c::init()
{
  pinMode( _pinSwitchEastNorth, OUTPUT );
  pinMode( _pinSwitchEastSouth, OUTPUT );
  pinMode( _pinSwitchWestNorth, OUTPUT );
  pinMode( _pinSwitchWestSouth, OUTPUT );

  doStopTrain();

  ldrRead();
  _ldrNorthReferenceValue = _ldrNorthLastValue;
  _ldrSouthReferenceValue = _ldrSouthLastValue;
}

/*!
  Setzt die Pins der Fotowiderstände
*/
void arduTram_c::setPinLdr( int pinNorth, int pinSouth )
{
  _pinLdrNorth = pinNorth;
  _pinLdrSouth = pinSouth;
}

/*!
  Setzt die Pins der östlichen Weiche
*/
void arduTram_c::setPinSwitchEast( int pin1, int pin2 )
{
  _pinSwitchEastNorth = pin1;
  _pinSwitchEastSouth = pin2;
}

/*!
  Setzt die Pins der westlichen Weiche
*/
void arduTram_c::setPinSwitchWest( int pin1, int pin2 )
{
  _pinSwitchWestNorth = pin1;
  _pinSwitchWestSouth = pin2;
}

/*!
  Setzt die Pins der Zug-Stromversorgung
*/
void arduTram_c::setPinTrain( int pin1, int pin2 )
{
  _pinTrainEast = pin1;
  _pinTrainWest = pin2;
}

/*!
  Setzt die Geschwindigkeit des Zug im nördlichen Gleis
*/
void arduTram_c::setSpeedNorth( int speed )
{
  _speedNorth = speed;
}

/*!
  Setzt die Geschwindigkeit des Zug im südlichen Gleis
*/
void arduTram_c::setSpeedSouth( int speed )
{
  _speedSouth = speed;
}

/*
  HTTP-Request bearbeiten
*/
void arduTram_c::httpRequest( EthernetServer &server )
{
  EthernetClient client = server.available();
  if ( client )
  {
    const char *red = "red";
    const char *green = "green";
    const char *black = "black";
    const char *gray = "gray";
    const char *colorLoopTrack = black;
    const char *colorNorthTrack = black;
    const char *colorSouthTrack = black;
    const char *colorArrowNorthWest = gray;
    const char *colorArrowNorthEast = gray;
    const char *colorArrowSouthWest = gray;
    const char *colorArrowSouthEast = gray;

    char request[ 1000 + 1 ];
    int requestID = 0;
    int sequence = 0;

    int avail = client.available();
    if ( avail > 1000 )
    {
      avail = 1000;
    }

    for ( int xx = 0; xx < avail; xx++ )
    {
      request[ xx ] = client.read();
    }
    request[ avail ] = '\0';

    char *strPos = strstr( request, "atReq" );
    if ( strPos )
    {
      strPos += 5;
      char *endPos = strchr( request, parameterDivider );
      if ( endPos )
      {
        *endPos = '\0';
        requestID = atoi( strPos );
        *endPos = parameterDivider;
      }
    }

    strPos = strstr( request, "atSeq" );
    if ( strPos )
    {
      strPos += 5;
      char *endPos = strchr( request, parameterDivider );
      if ( endPos )
      {
        *endPos = '\0';
        sequence = atoi( strPos );
        *endPos = parameterDivider;
      }
    }

    // Request auswerten
    if ( sequence != _lastSequence )
    {
      switch ( requestID )
      {
      case REQUEST_NONE:
        break;

      case REQUEST_TRAIN_NORTH_WEST:
        if ( _state == STATE_IDLE )
        {
          _direction = DIRECTION_NORTH_WEST;
          _state = STATE_BEGIN_ACTION;
        }
        break;

      case REQUEST_TRAIN_NORTH_EAST:
        if ( _state == STATE_IDLE )
        {
          _direction = DIRECTION_NORTH_EAST;
          _state = STATE_BEGIN_ACTION;
        }
        break;

      case REQUEST_TRAIN_SOUTH_WEST:
        if ( _state == STATE_IDLE )
        {
          _direction = DIRECTION_SOUTH_WEST;
          _state = STATE_BEGIN_ACTION;
        }
        break;

      case REQUEST_TRAIN_SOUTH_EAST:
        if ( _state == STATE_IDLE )
        {
          _direction = DIRECTION_SOUTH_EAST;
          _state = STATE_BEGIN_ACTION;
        }
        break;

      case REQUEST_STOP:
        _direction = DIRECTION_NONE;
        if ( _state != STATE_STOPPED )
        {
          _state = STATE_EMERGENCY_STOP;
        }
        break;

      case REQUEST_RESET:
        _direction = DIRECTION_NONE;
        if ( _state == STATE_STOPPED )
        {
          _state = STATE_SELFTEST;
        }
        else
        {
          _state = STATE_EMERGENCY_STOP;
        }
        break;

      default:
        _direction = DIRECTION_NONE;
        _state = STATE_EMERGENCY_STOP;
        break;
      }

      _lastSequence = sequence;
    }

    // Farben setzen
    if ( _state == STATE_IDLE )
    {
      colorArrowNorthWest = green;
      colorArrowNorthEast = green;
      colorArrowSouthWest = green;
      colorArrowSouthEast = green;
    }
    else if ( ( _state >= STATE_BEGIN_ACTION ) && ( _state <= STATE_FINISH_ACTION ) )
    {
      colorLoopTrack = green;
      if ( directionNorth() )
      {
        colorNorthTrack = green;
        if ( _direction == DIRECTION_NORTH_WEST )
        {
          colorArrowNorthWest = green;
        }
        else
        {
          colorArrowNorthEast = green;
        }
      }
      else if ( directionSouth() )
      {
        colorSouthTrack = green;
        if ( _direction == DIRECTION_SOUTH_WEST )
        {
          colorArrowSouthWest = green;
        }
        else
        {
          colorArrowSouthEast = green;
        }
      }
    }

    // HTML Ausgabe erzeugen
    client.println( F( "HTTP/1.1 200 OK" ) );
    client.println( F( "Content-Type: text/html" ) );
    client.println();
//    client.println( F( "<!DOCTYPE html>" ) );
    client.println( F( "<html><head><title>ArduTram</title></head><meta http-equiv=\"refresh\" content=\"5\">"
                       "<body bgcolor='#444444'><center>" ) );
    client.println( F( "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" baseProfile=\"full\" "
                       "width=\"80%\" height=\"80%\" viewBox=\"0 0 1600 1000\">" ) );

    // Rahmen
    client.println( F( "<rect x=\"5\" y=\"5\" width=\"1590\" height=\"990\" rx=\"20\" ry=\"20\" "
                       "fill=\"white\" stroke=\"gray\" stroke-width=\"3\"/>" ) );

    // Kreis
    client.print( F( "<polyline points=\"1400,250 1500,350 1500,600 1300,800 300,800 100,600 100,350 200,250\" "
                       "fill=\"none\" stroke=\"" ) );
    client.print( colorLoopTrack );
    client.println( F( "\" stroke-width=\"10\"/>" ) );

    // Gleis Nord
    client.print( F( "<polyline points=\"200,250 300,150 1300,150 1400,250\" fill=\"none\" stroke=\"" ) );
    client.print( colorNorthTrack );
    client.println( F( "\" stroke-width=\"10\"/>" ) );
    // Gleis Süd
    client.print( F( "<line x1=\"200\" y1=\"250\" x2=\"1400\" y2=\"250\" stroke=\"" ) );
    client.print( colorSouthTrack );
    client.println( F( "\" stroke-width=\"10\"/>" ) );

    // Zug Gleis Nord
    if ( _northTrackOccupied )
    {
      client.print( F( "<rect x=\"700\" y=\"110\" width=\"200\" height=\"80\" fill=\"" ) );
      client.print( colorNorthTrack );
      client.println( F( "\"/>" ) );
    }
    // Zug Gleis Süd
    if ( _southTrackOccupied )
    {
      client.print( F( "<rect x=\"700\" y=\"210\" width=\"200\" height=\"80\" fill=\"" ) );
      client.print( colorSouthTrack );
      client.println( F( "\"/>" ) );
    }

    // Pfeil Nord/West
    if ( _state == STATE_IDLE )
    {
      printLink( client, REQUEST_TRAIN_NORTH_WEST );
    }
    client.print( F( "<polygon points=\"500,100 520,80 520,90 550,90 550,110 520,110 520,120\" fill=\"" ) );
    client.print( colorArrowNorthWest );
    client.println( F( "\"/>" ) );
    if ( _state == STATE_IDLE )
    {
      client.println( F( "</a>" ) );
    }

    // Pfeil Nord/Ost
    if ( _state == STATE_IDLE )
    {
      printLink( client, REQUEST_TRAIN_NORTH_EAST );
    }
    client.print( F( "<polygon points=\"1100,100 1080,80 1080,90 1050,90 1050,110 1080,110 1080,120\" fill=\"" ) );
    client.print( colorArrowNorthEast );
    client.println( F( "\"/>" ) );
    if ( _state == STATE_IDLE )
    {
      client.println( F( "</a>" ) );
    }

    // Pfeil Süd/West
    if ( _state == STATE_IDLE )
    {
      printLink( client, REQUEST_TRAIN_SOUTH_WEST );
    }
    client.println( F( "<polygon points=\"500,300 520,280 520,290 550,290 550,310 520,310 520,320\" fill=\"" ) );
    client.print( colorArrowSouthWest );
    client.println( F( "\"/>" ) );
    if ( _state == STATE_IDLE )
    {
      client.println( F( "</a>" ) );
    }

    // Pfeil Süd/Ost
    if ( _state == STATE_IDLE )
    {
      printLink( client, REQUEST_TRAIN_SOUTH_EAST );
    }
    client.println( F( "<polygon points=\"1100,300 1080,280 1080,290 1050,290 1050,310 1080,310 1080,320\" fill=\"" ) );
    client.print( colorArrowSouthEast );
    client.println( F( "\"/>" ) );
    if ( _state == STATE_IDLE )
    {
      client.println( F( "</a>" ) );
    }

/*
    // Weiche West nach Gleis Nord
    client.println( F( "<circle cx=\"220\" cy=\"200\" r=\"10\" fill=\"red\"/>" ) );
    // Weiche West nach Gleis Süd
    client.println( F( "<circle cx=\"230\" cy=\"270\" r=\"10\" fill=\"green\"/>" ) );
    // Weiche Ost nach Gleis Nord
    client.println( F( "<circle cx=\"1380\" cy=\"200\" r=\"10\" fill=\"red\"/>" ) );
    // Weiche Ost nach Gleis Süd
    client.println( F( "<circle cx=\"1370\" cy=\"270\" r=\"10\" fill=\"green\"/>" ) );
*/

    // Button STOP
    client.println( F( "<rect x=\"50\" y=\"880\" width=\"170\" height=\"80\" rx=\"10\" ry=\"10\" "
                       "fill=\"red\" stroke=\"black\" stroke-width=\"3\"/>" ) );
    printLink( client, REQUEST_STOP );
    client.println( F( "<text x=\"80\" y=\"933\" style=\"font-size:40px; font-weight:bold; fill:white;\">STOP</text></a>" ) );

    // Button RESET
    client.println( F( "<rect x=\"250\" y=\"880\" width=\"170\" height=\"80\" rx=\"10\" ry=\"10\" "
                       "fill=\"lightgray\" stroke=\"black\" stroke-width=\"3\"/>" ) );
    if ( _state == STATE_STOPPED )
    {
      printLink( client, REQUEST_RESET );
    }
    client.print( F( "<text x=\"270\" y=\"933\" style=\"font-size:40px; font-weight:bold; fill:" ) );
    if ( _state == STATE_STOPPED )
    {
      client.print( black );
    }
    else
    {
      client.print( gray );
    }
    client.println( F( ";\">RESET</text>" ) );
    if ( _state == STATE_STOPPED )
    {
      client.println( F( "</a>" ) );
    }

    client.println( F( "</svg>" ) );

    client.print( F( "<p>State: " ) );
    switch ( _state )
    {
    case STATE_SELFTEST:           client.println( F( "Selftest" ) );           break;
    case STATE_IDLE:               client.println( F( "Idle" ) );               break;
    case STATE_BEGIN_ACTION:       client.println( F( "Begin Action" ) );       break;
    case STATE_THROW_SWITCHES:     client.println( F( "Throw Switches" ) );     break;
    case STATE_SWITCHES_THROWN:    client.println( F( "Switches Thrown" ) );    break;
    case STATE_START_TRAIN:        client.println( F( "Start Train" ) );        break;
    case STATE_WAIT_FOR_DEPARTURE: client.println( F( "Wait for Departure" ) ); break;
    case STATE_WAIT_FOR_ARRIVAL:   client.println( F( "Wait for Arrival" ) );   break;
    case STATE_STOP_TRAIN:         client.println( F( "Stop Train" ) );         break;
    case STATE_FINISH_ACTION:      client.println( F( "Finish Action" ) );      break;
    case STATE_EMERGENCY_STOP:     client.println( F( "Emergency Stop" ) );     break;
    case STATE_STOPPED:            client.println( F( "Stopped" ) );            break;
    case STATE_RESET:              client.println( F( "Reset" ) );              break;
    default:
      client.println( F( "Unknown State " ) );
      client.println( _state );
      break;
    }

    client.print( F( "<br>Direction: " ) );
    switch ( _direction )
    {
    case DIRECTION_NONE:       client.println( F( "-" ) );                 break;
    case DIRECTION_NORTH_WEST: client.println( F( "North Track, West" ) ); break;
    case DIRECTION_NORTH_EAST: client.println( F( "North Track, East" ) ); break;
    case DIRECTION_SOUTH_WEST: client.println( F( "South Track, West" ) ); break;
    case DIRECTION_SOUTH_EAST: client.println( F( "South Track, East" ) ); break;
    default:
      client.println( F( "Unknown Direction " ) );
      client.println( _direction );
      break;
    }

    client.print( F( "<br>Last Request: " ) );
    switch ( requestID )
    {
    case REQUEST_NONE:             client.println( F( "-" ) );                 break;
    case REQUEST_TRAIN_NORTH_WEST: client.println( F( "North Track, West" ) ); break;
    case REQUEST_TRAIN_NORTH_EAST: client.println( F( "North Track, East" ) ); break;
    case REQUEST_TRAIN_SOUTH_WEST: client.println( F( "South Track, West" ) ); break;
    case REQUEST_TRAIN_SOUTH_EAST: client.println( F( "South Track, East" ) ); break;
    case REQUEST_STOP:             client.println( F( "Stop" ) );              break;
    case REQUEST_RESET:            client.println( F( "Reset" ) );             break;
    default:
      client.println( F( "Request " ) );
      client.println( requestID );
      break;
    }

    client.print( F( "<br>LDR North: " ) );
    client.print( _ldrNorthReferenceValue );
    client.print( F( " / ") );
    client.println( _ldrNorthLastValue );

    client.print( F( "<br>LDR South: " ) );
    client.print( _ldrSouthReferenceValue );
    client.print( F( " / ") );
    client.println( _ldrSouthLastValue );

    client.println( F( "<p>" ) );
//    client.println( request );
    client.println( F( "</center></body></html>" ) );

    delay( 1 );
    client.stop();
  }
}

/*
  Einen HTML-Link erzeugen
*/
void arduTram_c::printLink( EthernetClient &client, int requestID )
{
  client.print( F( "<a xlink:href=\"atReq" ) );
  client.print( requestID );
  client.print( parameterDivider );
  client.print( F( "atSeq" ) );
  client.print( _lastSequence + 1 );
  client.print( parameterDivider );
  client.print( F( "\">" ) );
}

/*
  Statuswechsel vollziehen
*/
void arduTram_c::stateMachine()
{
  ldrRead();

  switch ( _state )
  {
  case STATE_SELFTEST:           selftest();         break;
  case STATE_IDLE:               idle();             break;
  case STATE_BEGIN_ACTION:       beginAction();      break;
  case STATE_THROW_SWITCHES:     throwSwitches();    break;
  case STATE_SWITCHES_THROWN:    switchesThrown();   break;
  case STATE_START_TRAIN:        startTrain();       break;
  case STATE_WAIT_FOR_DEPARTURE: waitForDeparture(); break;
  case STATE_WAIT_FOR_ARRIVAL:   waitForArrival();   break;
  case STATE_STOP_TRAIN:         stopTrain();        break;
  case STATE_FINISH_ACTION:      finishAction();     break;
  case STATE_STOPPED:            stopped();          break;
  case STATE_RESET:              reset();            break;
  default:                       emergencyStop();    break;
  }
}

/*
  Führt den Selbsttest durch
*/
void arduTram_c::selftest()
{
  if ( trackNorthOccupied() && trackSouthOccupied() )
  {
    _state = STATE_IDLE;
  }
  else
  {
    _state = STATE_EMERGENCY_STOP;
  }
}

/*
  Auf Benutzereingaben warten
*/
void arduTram_c::idle()
{
  // nichts
}

/*
  Startet eine Zugfahrt
*/
void arduTram_c::beginAction()
{
  _state = STATE_THROW_SWITCHES;
}

/*
  Weichen umlegen
*/
void arduTram_c::throwSwitches()
{
  Serial.println( F( "throwSwitches" ) );

  if ( directionNorth() )
  {
    throwSwitch( _pinSwitchEastNorth );
    throwSwitch( _pinSwitchWestNorth );
    _state = STATE_SWITCHES_THROWN;
  }
  else if ( directionSouth() )
  {
    throwSwitch( _pinSwitchEastSouth );
    throwSwitch( _pinSwitchWestSouth );
    _state = STATE_SWITCHES_THROWN;
  }
  else
  {
    _state = STATE_EMERGENCY_STOP;
  }
}

/*
  einzelne Weichen umlegen
*/
void arduTram_c::throwSwitch( int pin )
{
  Serial.print( F( "throwSwitches " ) );
  Serial.println( pin );

  digitalWrite( pin, HIGH );
  delay( 400 );
  digitalWrite( pin, LOW );
  delay( 100 );
}

/*
  Weichen umgelegt
*/
void arduTram_c::switchesThrown()
{
  _state = STATE_START_TRAIN;
}

/*
  Zugabfahrt
*/
void arduTram_c::startTrain()
{
  int speed = 0;
  if ( directionNorth() )
  {
    speed = _speedNorth;
  }
  else if ( directionSouth() )
  {
    speed = _speedSouth;
  }

  if ( _direction == DIRECTION_NORTH_EAST || _direction == DIRECTION_SOUTH_EAST )
  {
    analogWrite( _pinTrainEast, speed );
    analogWrite( _pinTrainWest, 0 );
    _state = STATE_WAIT_FOR_DEPARTURE;
  }
  else if ( _direction == DIRECTION_NORTH_WEST || _direction == DIRECTION_SOUTH_WEST )
  {
    analogWrite( _pinTrainEast, 0 );
    analogWrite( _pinTrainWest, speed );
    _state = STATE_WAIT_FOR_DEPARTURE;
  }
  else
  {
    _state = STATE_EMERGENCY_STOP;
  }
}

/*
  Warten, bis Zug losgefahren ist
*/
void arduTram_c::waitForDeparture()
{
  if ( ( directionNorth() && !trackNorthOccupied() ) ||
       ( directionSouth() && !trackSouthOccupied() ) )
  {
    _state = STATE_WAIT_FOR_ARRIVAL;
  }

  if ( ( directionNorth() && !trackSouthOccupied() ) ||
       ( directionSouth() && !trackNorthOccupied() ) )
  {
    _state = STATE_EMERGENCY_STOP;
  }
}

/*
  Warten, bis Zug angekommen ist
*/
void arduTram_c::waitForArrival()
{
  if ( ( directionNorth() && trackNorthOccupied() ) ||
       ( directionSouth() && trackSouthOccupied() ) )
  {
    _state = STATE_STOP_TRAIN;
  }

  if ( ( directionNorth() && !trackSouthOccupied() ) ||
       ( directionSouth() && !trackNorthOccupied() ) )
  {
    _state = STATE_EMERGENCY_STOP;
  }
}

/*
  Zug anhalten
*/
void arduTram_c::stopTrain()
{
  doStopTrain();
  _state = STATE_FINISH_ACTION;
}

/*
  Beendet eine Zugfahrt
*/
void arduTram_c::finishAction()
{
  _state = STATE_IDLE;
  _direction = DIRECTION_NONE;
}

/*
  Stoppt sofort den Zug
*/
void arduTram_c::emergencyStop()
{
  doStopTrain();
  _state = STATE_STOPPED;
  _direction = DIRECTION_NONE;
}

/*
  Auf Reset warten
*/
void arduTram_c::stopped()
{
  // nichts
}

/*
  Reset
*/
void arduTram_c::reset()
{
  _state = STATE_SELFTEST;
  _direction = DIRECTION_NONE;
}

/*!
  Fotowiderstände auslesen
*/
void arduTram_c::ldrRead()
{
  _ldrNorthLastValue = analogRead( _pinLdrNorth );
  _ldrSouthLastValue = analogRead( _pinLdrSouth );
}

/*!
  Test ob das Nordgleis belegt ist
*/
bool arduTram_c::trackNorthOccupied()
{
  return trackOccupied( _ldrNorthReferenceValue, _ldrNorthLastValue );
}

/*!
  Test ob das Südgleis belegt ist
*/
bool arduTram_c::trackSouthOccupied()
{
  return trackOccupied( _ldrSouthReferenceValue, _ldrSouthLastValue );
}

/*!
  Test ob ein Gleis belegt ist
*/
bool arduTram_c::trackOccupied( int referenceValue, int lastValue ) const
{
  return lastValue < referenceValue + 120;
}

/*!
  Liefert ob der Wert von _direction das Nord-Gleis betrifft
*/
bool arduTram_c::directionNorth()
{
  return ( _direction == DIRECTION_NORTH_EAST ) || ( _direction == DIRECTION_NORTH_WEST );
}

/*!
  Liefert ob der Wert von _direction das Süd-Gleis betrifft
*/
bool arduTram_c::directionSouth()
{
  return ( _direction == DIRECTION_SOUTH_EAST ) || ( _direction == DIRECTION_SOUTH_WEST );
}

/*
  Zug anhalten
*/
void arduTram_c::doStopTrain()
{
  analogWrite( _pinTrainEast, 0 );
  analogWrite( _pinTrainWest, 0 );
}
