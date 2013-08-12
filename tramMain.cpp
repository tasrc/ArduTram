#include <SPI.h>
#include <Ethernet.h>

#include "arduTram.h"

namespace
{
  byte mac[] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x24 };
  byte ip[] = { 10, 128, 53, 4 };
  byte gateway[] = { 10, 128, 255, 254 };
  byte subnet[] = { 255, 255, 0, 0 };
  EthernetServer server( 80 );
  arduTram_c arduTram;
}

void setup()
{
  arduTram.setPinLdr( 0, 1 ); // Analog
  arduTram.setPinSwitchEast( 5, 6 ); // Digital
  arduTram.setPinSwitchWest( 8, 7 ); // Digital
  arduTram.setPinTrain( 3, 9 ); // Digital/PWM
  arduTram.setSpeedNorth( 100 );
  arduTram.setSpeedSouth( 60 );
  arduTram.init();

  Ethernet.begin( mac, ip, gateway, subnet );
  Serial.begin( 9600 );
  server.begin();
}

void loop()
{
  arduTram.httpRequest( server );
  arduTram.stateMachine();
}
