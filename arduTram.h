class EthernetClient;
class EthernetServer;

class arduTram_c
{
private:
  enum state_t
  {
    STATE_SELFTEST,
    STATE_IDLE,
    STATE_BEGIN_ACTION,
    STATE_THROW_SWITCHES,
    STATE_SWITCHES_THROWN,
    STATE_START_TRAIN,
    STATE_WAIT_FOR_DEPARTURE,
    STATE_WAIT_FOR_ARRIVAL,
    STATE_STOP_TRAIN,
    STATE_FINISH_ACTION,
    STATE_EMERGENCY_STOP,
    STATE_STOPPED,
    STATE_RESET
  };
  
  enum request_t
  {
    REQUEST_NONE             = 0,
    REQUEST_TRAIN_NORTH_WEST = 1,
    REQUEST_TRAIN_NORTH_EAST = 2,
    REQUEST_TRAIN_SOUTH_WEST = 3,
    REQUEST_TRAIN_SOUTH_EAST = 4,
    REQUEST_STOP             = 5,
    REQUEST_RESET            = 6
  };

  enum direction_t
  {
    DIRECTION_NONE,
    DIRECTION_NORTH_WEST,
    DIRECTION_NORTH_EAST,
    DIRECTION_SOUTH_WEST,
    DIRECTION_SOUTH_EAST
  };
  
public:
  arduTram_c();
  
  void httpRequest( EthernetServer & );
  void init();
  void setPinLdr( int, int );
  void setPinSwitchEast( int, int );
  void setPinSwitchWest( int, int );
  void setPinTrain( int, int );
  void setSpeedNorth( int );
  void setSpeedSouth( int );
  void stateMachine();
  
private:
  void beginAction();
  bool directionNorth();
  bool directionSouth();
  void doStopTrain();
  void emergencyStop();
  void finishAction();
  void idle();
  void ldrRead();
  void printLink( EthernetClient &, int );
  void reset();
  void selftest();
  void startTrain();
  void stopped();
  void stopTrain();
  void switchesThrown();
  void throwSwitch( int );
  void throwSwitches();
  bool trackNorthOccupied();
  bool trackOccupied( int, int ) const;
  bool trackSouthOccupied();
  void waitForArrival();
  void waitForDeparture();

  direction_t _direction;
  int         _lastSequence;
  int         _ldrNorthLastValue;
  int         _ldrNorthReferenceValue;
  int         _ldrSouthLastValue;
  int         _ldrSouthReferenceValue;
  bool        _northTrackOccupied;
  int         _pinLdrNorth;
  int         _pinLdrSouth;
  int         _pinSwitchEastNorth;
  int         _pinSwitchEastSouth;
  int         _pinSwitchWestNorth;
  int         _pinSwitchWestSouth;
  int         _pinTrainEast;
  int         _pinTrainWest;
  bool        _southTrackOccupied;
  int         _speedNorth;
  int         _speedSouth;
  state_t     _state;
};
