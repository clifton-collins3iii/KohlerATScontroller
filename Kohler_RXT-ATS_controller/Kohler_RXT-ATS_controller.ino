/*
Projects by Robbie and Clifton
TEST program for: Kohler RXT-ATS and RDC2 Generator controller
For Louisiana Standard Contractor's customer Ross Wolkart
Target is Arduino Uno R3
Tip:  Use CTRL + Shift + U to upload using programmer the sketch
https://github.com/Aasim-A/AsyncTimer
*/
#include <AsyncTimer.h> // JavaScript-like async timing functions(settimeout, setInterval)
AsyncTimer atimer(3);   // configure seven timers

unsigned short timer_startupDelay = 0;
unsigned short timer_runloopInterval = 0;
unsigned short timer_operationtoperform = 0;
//
int input_UtilityPowerSensePin = 2;
int input_GeneratorPowerSensePin = 3;
int input_HousePowerSensePin = 4;
//
int output_Indicator_ATSonGenerator = 6;
int output_Indicator_HousePower = 7;
int output_Indicator_UtilityPower = 8;
int output_Indicator_EngineRunPin = 9;
int output_Indicator_GeneratorPower = 10;
int output_UtilityPowerSwitchTo = 11;
int output_EngineRunPin = 12;
int output_GeneratorPowerSwitchTo = 13;
//
int counter = 0;
int indicator_engineRunning = 0;
int indicator_utilityPower = 0;   // initially zero and only 1 when the utility power closes the relay
int indicator_generatorPower = 0; // initially zero and only 1 when the generator power closes the relay
int indicator_housePower = 0;     // initially zero and only 1 when there is power to the house that closes the relay
int indicator_atsPosition = 0;  // default position is Utility Power = 0.  Generator Power = 1
int relay_engineRunning = 0;
int relay_utilityPowerSwitchTo = 0;
int relay_generatorPowerSwitchTo = 0;
/*
---  Inputs  ---
UP is Utility Power
GP is Generator Power
HP is House Power
--- Outputs ---
SP is ATS or Switch Position (Derived from HP & (UP | SP))
ER is Engine Running 2 wire start signal (relay)
US is ATS to utility signal (relay)
GS is ATS to generator signal (relay)
*/
int def_state = 0; // State is the measured / observed inputs + current outputs
int def_disposition = 0; // Disposition is the expected or to be configured/set State
int def_status = 0;  // The current outputs value
int def_operation = 0; // Set the outputs to this value
//
int cfg_relayactiondelay = 2000;      // 2 seconds for a relay to switch and mechanical operates
int cfg_powerrestoredelay = 300000;    //30 or less for testing, 300 for production or 5 minutes
int cfg_engineCrankdelay = 10000;     // ten second delay setting is interval in milliseconds.
int cfg_short_engineCrankdelay = 2500;
int cfg_transitiontoGeneratordelay = 15000;
int cfg_short_transitiontoGeneratordelay = 3000;
int cfg_engineShutdowndelay = 60000;

void setup() {
  pinMode(PIN_A0, INPUT); // reserved
	pinMode(input_UtilityPowerSensePin, INPUT);    // 1 = Utility Power Present
  pinMode(input_GeneratorPowerSensePin, INPUT);    // 1 = Generator Power Present
  pinMode(input_HousePowerSensePin, INPUT);       // 1 = House Power Present
    // 
  pinMode(output_Indicator_ATSonGenerator, OUTPUT); // 1 = ATS on Generator
  pinMode(output_Indicator_HousePower, OUTPUT);   // 1 = Power to house present
  pinMode(output_Indicator_UtilityPower, OUTPUT);   // 1 = Utility Power Present
  pinMode(output_Indicator_EngineRunPin, OUTPUT);   //  1 = engineRunning
  pinMode(output_Indicator_GeneratorPower, OUTPUT);  // 1 = Generator Power Present
  //
  pinMode(output_UtilityPowerSwitchTo, OUTPUT);   // 1 = Switch to Utility Power
  pinMode(output_EngineRunPin, OUTPUT);   //  1 = engineRunning
  pinMode(output_GeneratorPowerSwitchTo, OUTPUT);  // 1 = Switch to Generator Power
  //
  timer_startupDelay = atimer.setTimeout(function_startUp, 1000);
}

void function_startUp() {
  function_setinitialstate();
  timer_runloopInterval = atimer.setInterval(function_runloop, 1000);
}

void function_setinitialstate() {
  function_gatherinformation();
  function_setoutput();
  function_setstate();
  def_disposition = -1;   // set for difference on the first runloop.
                          // specifically looking for power out, crank generator...
  function_determineoperation();
}

void function_runloop() {
  function_gatherinformation();
  //function_setstate();
  //function_setstatus();
  function_determineoperation();
}

void function_setstate()
{
  def_disposition = def_state;
  def_state = (indicator_utilityPower * 64) + (indicator_generatorPower * 32) + (indicator_housePower * 16) + (indicator_atsPosition * 8) + (relay_engineRunning * 4) + (relay_utilityPowerSwitchTo * 2) + (relay_generatorPowerSwitchTo * 1);
}

void function_setstatus()
{
  def_status = (indicator_atsPosition * 8) + (relay_engineRunning * 4) + (relay_utilityPowerSwitchTo * 2) + (relay_generatorPowerSwitchTo * 1);
}

void function_determineoperation()
{
  if (def_disposition != def_state){
    switch (def_state) {
      case 0: 
        function_timer_operationtoperform(cfg_engineCrankdelay, 4); 
        break;
      case 80: 
          function_assert(0); 
          break; // on Utility Power set atsPosition to 0
      case 88: 
          function_assert(0);
          break; // NOP
      case 4: 
          indicator_engineRunning = 1;
          function_setindicator();
          //function_timer_operationtoperform(cfg_transitiontoGeneratordelay, 5); 
          break;
      case 53: 
          function_timer_operationtoperform(cfg_relayactiondelay, 13); 
          break;
      case 112: break; // NOP
      case 32: function_timer_operationtoperform(cfg_short_engineCrankdelay, 4); break;
      case 36: function_timer_operationtoperform(cfg_short_transitiontoGeneratordelay, 5); break;
      case 37: 
            break;
      case 61: function_timer_operationtoperform(cfg_relayactiondelay, 12); break;
      case 60: 
          cancel_Timeout(timer_operationtoperform);   // if utility power dropped before power restoredelay
          break; // NOP
      case 125: function_timer_operationtoperform(cfg_relayactiondelay, 12); break;
      case 124: function_timer_operationtoperform(cfg_powerrestoredelay, 14); break;
      case 110: break; //NOP
      case 126: function_timer_operationtoperform(cfg_relayactiondelay, 6); break;
      case 118: function_timer_operationtoperform(cfg_engineShutdowndelay, 2); break;
      case 114: 
          indicator_engineRunning = 0;
          function_setindicator();
          function_timer_operationtoperform(cfg_relayactiondelay, 0); 
          break;
      case 82: function_timer_operationtoperform(cfg_relayactiondelay, 0); break;
      case 127:
      case 96:
      case 104:
      case 111:
      case 48:
      case 63:
      case 56:
      case 59:
            function_assert(0); 
            break;
      default:
            function_assert(16);
            break;
    }
  }
}

void function_gatherinformation() {
  indicator_utilityPower = digitalRead(input_UtilityPowerSensePin);
  indicator_generatorPower = digitalRead(input_GeneratorPowerSensePin);
  indicator_housePower = digitalRead(input_HousePowerSensePin);
  function_setstate();
  function_setstatus();
  switch (def_state) {  // derive or set the atsPosition; SP
    case 0: 
    case 80:
    case 4:
    case 53:
    case 112:
    case 32:
    case 36:
    case 37:
    case 118:
    case 114:
    case 82:
      indicator_atsPosition = 0;  // SP is UP;  atsPosition = Utility Power
      break;
    case 88:
    case 61:
    case 60:
    case 125:
    case 124:
    case 110:
    case 126:
      indicator_atsPosition = 1; 
      break;
  }
  function_setindicator();
}

void function_timer_operationtoperform(int delayms, int action)
{
    // cancels previous timer timeout
    // configure timeout to delay the action to perform.
    cancel_Timeout(timer_operationtoperform);
    def_operation = action;
    timer_operationtoperform = atimer.setTimeout([action](){
      function_assert(action);
      }, delayms);
}

void function_assert(int disposition)
{
  def_operation = disposition;
  relay_generatorPowerSwitchTo = getBit(disposition, 0);
  relay_utilityPowerSwitchTo = getBit(disposition, 1);
  relay_engineRunning = getBit(disposition, 2);
  indicator_engineRunning = getBit(disposition, 2);
  indicator_atsPosition = getBit(disposition, 3);
  function_setoutput();
}

int getBit(int number, int bitPosition) {
  return (number & (1 << bitPosition)) == 0 ? 0 : 1;
}

void function_setoutput(){
  digitalWrite(output_Indicator_EngineRunPin, indicator_engineRunning);
  digitalWrite(output_Indicator_HousePower, indicator_housePower);
  digitalWrite(output_Indicator_UtilityPower, indicator_utilityPower);
  digitalWrite(output_Indicator_GeneratorPower, indicator_generatorPower);
  digitalWrite(output_EngineRunPin, relay_engineRunning);
  digitalWrite(output_UtilityPowerSwitchTo, relay_utilityPowerSwitchTo);
  digitalWrite(output_GeneratorPowerSwitchTo, relay_generatorPowerSwitchTo);
  function_setstatus();
}

void function_setindicator(){
  digitalWrite(output_Indicator_EngineRunPin, indicator_engineRunning);
  digitalWrite(output_Indicator_HousePower, indicator_housePower);
  digitalWrite(output_Indicator_UtilityPower, indicator_utilityPower);
  digitalWrite(output_Indicator_GeneratorPower, indicator_generatorPower);
  digitalWrite(output_Indicator_ATSonGenerator, indicator_atsPosition);
  function_setstatus();
}

void cancel_Timeout(int id) {
  if (id != 0) {
    atimer.cancel(id);
  }
}

// void clear_Timeouts() {
//   cancel_Timeout(loopTimeout);
// }


// void set_cooldownTimeout() {
//   cancel_Timeout(cooldownTimeout);
//   cooldownTimeout = atimer.setTimeout(engineshutdown, cfg_engineShutdowndelay);
// }

void testloop() {
  // Closes and opens the contact of relay 1 and turns on/off led 1
  digitalWrite(output_Indicator_UtilityPower, HIGH); // Sets the relay 1 on
  delay(1000);
  digitalWrite(output_Indicator_UtilityPower, LOW); // Sets the relay 1 off
  delay(1000);

  // Closes and opens the contact of relay 2 and turns on/off led 2
  digitalWrite(output_Indicator_EngineRunPin, HIGH); // Sets the relay 2 on
  delay(1000); 
  digitalWrite(output_Indicator_EngineRunPin, LOW); // Sets the relay 2 off
  delay(1000);

  // Closes and opens the contact of relay 3 and turns on/off led 3
  digitalWrite(output_Indicator_GeneratorPower, HIGH); // Sets the relay 3 on
  delay(1000);
  digitalWrite(output_Indicator_GeneratorPower, LOW); // Sets the relay 3 off
  delay(1000);  
  delay(1000);
}

void loop() {
  atimer.handle();
}