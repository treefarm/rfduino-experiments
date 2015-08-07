#include <RFduinoBLE.h>
#include "Button.h"
#include "MultiColorLed.h"

using namespace Treefarm;

// pin 2 on the RGB shield is the red led
// pin 3 on the RGB shield is the green led
// pin 4 on the RGB shield is the blue led
MultiColorLed led(2, 3, 4);

// define colors for use with the led
Color red(10, 0, 0);
Color green(0, 10, 0);
Color blue(0, 0, 100);
Color yellow(10, 10, 0);
Color purple(100, 0, 100);

// pin 5 on the RGB shield is button 1
Button nextBtn(5);
// pin 6 on the RGB shield is button 2
Button prevBtn(6);

enum Region {
  In=0,
  Alert1,
  Alert2,
  Out
};
Region currentRegion(In);

void setup() {
  Serial.begin(9600);

  // this is the data we want to appear in the advertisement
  // (the deviceName length plus the advertisement length must be <= 18 bytes
  RFduinoBLE.advertisementData = "treefarm";
  
  // set the power level
  RFduinoBLE.txPowerLevel = 0;
  
  // start the BLE stack
  Serial.println("Starting BLE stack");
  RFduinoBLE.begin();
  
  // setup pinwake callbacks
  Serial.println("Configuring pinWake callbacks");
  RFduino_pinWakeCallback(nextBtn.GetPin(), HIGH, nextBtnPressed);
  RFduino_pinWakeCallback(prevBtn.GetPin(), HIGH, prevBtnPressed);
  
  setColorForRegion();
}

void setColorForRegion()
{
  switch(currentRegion) {
    case In:
      led.TurnOn(green);
      break;
    case Alert1:
      led.TurnOn(blue);
      break;
    case Alert2:
      led.TurnOn(yellow);
      break;
    case Out:
    default:
      led.TurnOn(red);
      break;
  }   
}

int nextBtnPressed(uint32_t ulPin)
{
  Serial.println("Entered nextBtn pinwake callback");
  if (nextBtn.Debounce(HIGH))
  {
    Serial.println("Sending nextBtn signal");
//    led.BlinkAndRestoreState(300, 0, 1, blue);
    
    if (static_cast<int>(currentRegion) < static_cast<int>(Out)) {
      currentRegion = static_cast<Region>(static_cast<int>(currentRegion) + 1);
    }
    setColorForRegion();
    RFduinoBLE.send(static_cast<int>(currentRegion));

    return 1;
  }
  
  return 0;  // don't exit RFduino_ULPDelay
}

int prevBtnPressed(uint32_t ulPin)
{
  Serial.println("Entered prevBtn pinwake callback");
  if (prevBtn.Debounce(HIGH))
  {
    Serial.println("Sending prevBtn signal");
//    led.BlinkAndRestoreState(300, 0, 1, green);
    
    if (static_cast<int>(currentRegion) > static_cast<int>(In)) {
      currentRegion = static_cast<Region>(static_cast<int>(currentRegion) - 1);
    }
    setColorForRegion();
    RFduinoBLE.send(static_cast<int>(currentRegion));
    
    return 1;
  }
  
  return 0;  // don't exit RFduino_ULPDelay
}

void loop() {
  Serial.println("Entering ULPDelay(INFINITE) in delay_until_button");
  // switch to lower power mode until a button edge wakes us up
  RFduino_ULPDelay(INFINITE);
  Serial.println("Leaving ULPDelay(INFINITE) in delay_until_button");
  
  // clear pin wake when the button is released - this will cause it to enter low power mode the next time through the loop
  if ((RFduino_pinWoke(nextBtn.GetPin()) && nextBtn.Debounce(LOW)) || (RFduino_pinWoke(prevBtn.GetPin()) && prevBtn.Debounce(LOW)))
  {
      RFduino_resetPinWake(nextBtn.GetPin());
      RFduino_resetPinWake(prevBtn.GetPin());
  }
  
  // if any button has been down for over 3 seconds, shut down
//  if (nextBtn.GetHoldDuration() > 3000 || prevBtn.GetHoldDuration() > 3000)
//      shutdown();
}

void RFduinoBLE_onConnect()
{
}

void RFduinoBLE_onReceive(char *data, int len)
{
  if (data[0] >= In && data[0] <= Out) {
    currentRegion = static_cast<Region>(static_cast<int>(data[0]));
    setColorForRegion();
    RFduinoBLE.send(static_cast<int>(currentRegion));
  } else if (data[0] == 77) {
    RFduinoBLE.send(static_cast<int>(currentRegion));
  } else {
    led.BlinkAndRestoreState(300, 0, 1, purple);
  }
}

void RFduinoBLE_onDisconnect()
{
//  led.TurnOn(red);
}

void shutdown()
{
  // blink the red led to indicate shutdown
  led.Blink(200, 200, 5, red);
  
  // this is so that the button release doesn't wake us up again
  delay(3000);
  RFduino_systemOff();
}
