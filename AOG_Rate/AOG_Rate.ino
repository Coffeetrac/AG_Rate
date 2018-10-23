#define LED_PIN 13
#define WORKSW_PIN 4  //PD4

#define   DIR_PIN    12  //PB4
#define   PWM_PIN    11  //PB2
// TODO pins for pwm of second valve

#define encPinLeft 2     //int0 D2 pin 2
#define encPinRight 3     //int1 D3 pin 3

#define RELAY1_PIN 5  //PD5
#define RELAY2_PIN 6  //PD6
#define RELAY3_PIN 7  //PD7
#define RELAY4_PIN 8  //PD8
#define RELAY5_PIN 9  //PD9
#define RELAY6_PIN 10 //PD10
#define RELAY7_PIN 13 //PD13
#define RELAY8_PIN A0  //PC0
#define RELAY9_PIN A1  //PC1
#define RELAY10_PIN A2  //PC2
#define RELAY11_PIN A3  //PC3
#define RELAY12_PIN A4  //PC4
#include "Var.h";

//loop time variables in microseconds
const unsigned long LOOP_TIME = 200; //in msec = 5hz
unsigned long lastTime = LOOP_TIME;
unsigned long currentTime = LOOP_TIME;
unsigned int dT = 100;
byte watchdogTimer = 0;

//the ISR counter
volatile unsigned long pulseCountLeft = 0, pulseDurationLeft;
volatile unsigned long pulseCountRight = 0, pulseDurationRight;

void setup()
{
  //pinMode(LED_PIN, OUTPUT); //configure LED for output
  pinMode(RELAY1_PIN, OUTPUT); //configure RELAY1 for output //Pin 5
  pinMode(RELAY2_PIN, OUTPUT); //configure RELAY2 for output //Pin 6
  pinMode(RELAY3_PIN, OUTPUT); //configure RELAY3 for output //Pin 7
  pinMode(RELAY4_PIN, OUTPUT); //configure RELAY4 for output //Pin 8
  pinMode(RELAY5_PIN, OUTPUT); //configure RELAY5 for output //Pin 9
  pinMode(RELAY6_PIN, OUTPUT); //configure RELAY6 for output //Pin 10
  pinMode(RELAY7_PIN, OUTPUT); //configure RELAY7 for output //Pin 13
  pinMode(RELAY8_PIN, OUTPUT); //configure RELAY8 for output //Pin A0
  pinMode(RELAY9_PIN, OUTPUT); //configure RELAY9 for output //Pin A1
  pinMode(RELAY10_PIN, OUTPUT); //configure RELAY10 for output //Pin A2
  pinMode(RELAY11_PIN, OUTPUT); //configure RELAY11 for output //Pin A3
  pinMode(RELAY12_PIN, OUTPUT); //configure RELAY12 for output //Pin A4
  //set up communication
  Serial.begin(38400);

  //use CHANGE for more ticks per liter
  attachInterrupt(digitalPinToInterrupt(encPinLeft), pinLeftChangeISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encPinRight), pinRightChangeISR, CHANGE);
}

void loop()
{
	currentTime = millis();
	unsigned int time = currentTime;

	if (currentTime - lastTime >= LOOP_TIME)
	{
		dT = currentTime - lastTime;
		lastTime = currentTime;

		//reset ISR Left Side
		countsThisLoopLeft = pulseCountLeft;
		pulseCountLeft = 0;

		if (countsThisLoopLeft)
		{
			//total pulse time over counts in microseconds
			pulseAverageLeft = pulseDurationLeft / countsThisLoopLeft;
			pulseDurationLeft = 0;
		}

		//Right Side
		countsThisLoopRight = pulseCountRight;
		pulseCountRight = 0;

		if (countsThisLoopRight)
		{
			//total pulse time over counts in microseconds
			pulseAverageRight = pulseDurationRight / countsThisLoopRight;
			pulseDurationRight = 0;
		}

		//increase the watchdog - reset in data recv.
		watchdogTimer++;

		//clean out serial buffer
		if (watchdogTimer > 10)
		{
			while (Serial.available() > 0) char t = Serial.read();
			watchdogTimer = 0;
		}

		//accumulated counts from this cycle
		accumulatedCountsLeft += countsThisLoopLeft;
		accumulatedCountsRight += countsThisLoopRight;

		//only if there is flow
		if (countsThisLoopLeft) 
		{    //what is current flowrate from meter
			rateKLeft = (float)pulseAverageLeft * 0.001;
			if (rateKLeft < .001) rateKLeft = 0.1;//prevent divide by zero      //
			else rateKLeft = ((1.0 / rateKLeft) * 60) / flowmeterCalFactorLeft;

			//Kalman filter
			PcLeft = PLeft + varProcessLeft;
			GLeft = PcLeft / (PcLeft + varRateLeft);
			PLeft = (1 - GLeft) * PcLeft; XpLeft = XeRateLeft; ZpLeft = XpLeft;
			XeRateLeft = GLeft * (rateKLeft - ZpLeft) + XpLeft;

			rateAppliedLPMLeft = XeRateLeft * 100; //fill in formatted rateApplied
		}
		else 
		{
			rateKLeft = 0.1;
			rateAppliedLPMLeft = 0;
		}

		//only if there is flow
		if (countsThisLoopRight) 
		{      //what is current flowrate from meter
			rateKRight = (float)pulseAverageRight * 0.001;
			if (rateKRight < .001) rateKRight = 0.1;//prevent divide by zero    //
			else rateKRight = ((1.0 / rateKRight) * 60) / flowmeterCalFactorRight;

			//Kalman filter
			PcRight = PRight + varProcessRight;
			GRight = PcRight / (PcRight + varRateRight);
			PRight = (1 - GRight) * PcRight; XpRight = XeRateRight; ZpRight = XpRight;
			XeRateRight = GRight * (rateKRight - ZpRight) + XpRight;

			rateAppliedLPMRight = XeRateRight * 100; //fill in formatted rateApplied
		}
		else 
		{
			rateKRight = 0.1;
			rateAppliedLPMRight = 0;
		}

		//turn on appropriate sections
		SetRelays();

		//Do the PID - this placed in code depending on valve style
		rateErrorLeft = rateSetPointLeft - rateKLeft;
		rateErrorRight = rateSetPointRight - rateKRight;

		//Left side or single meter
		calcRatePIDLeft();
		motorDriveLeft();

		//Also needs right side for dual
		calcRatePIDRight();
		motorDriveRight();

		//Send to agopenGPS, once per second
		Serial.print(rateAppliedLPMLeft); //100 x actual!
		Serial.print(",");
		Serial.print(rateAppliedLPMRight); //100 x actual!
		Serial.print(",");
		Serial.println((int)((float)accumulatedCountsLeft / (float)flowmeterCalFactorLeft +
			(float)accumulatedCountsRight / (float)flowmeterCalFactorRight));
		//Serial.print(",");
		//Serial.print( accumulatedCountsLeft );
		//Serial.print(",");
		//Serial.println( ctr );

		// flush out buffer
		Serial.flush();
	} //end of timed loop

	  //****************************************************************************************
	  //This runs continuously, outside of the timed loop, keeps checking UART for new data
	  // header high/low, relayHi/Lo byte, speed byte, rateSetPoint hi/lo
	if (Serial.available() > 0 && !isDataFound && !isSettingFound) //find the header,
	{
		int temp = Serial.read();
		header = tempHeader << 8 | temp;               //high,low bytes to make int
		tempHeader = temp;                             //save for next time
		if (header == 32762) isDataFound = true;     //Do we have a match?
		if (header == 32760) isSettingFound = true;     //Do we have a match?
	}

	//DATA Header has been found, so the next 4 bytes are the data -- 127H + 250L = 32762
	if (Serial.available() == 8 && isDataFound)
	{
		isDataFound = false;
		relayHi = Serial.read();   // read relay control from AgOpenGPS
		relayLo = Serial.read();   // read relay control from AgOpenGPS
		groundSpeed = Serial.read() >> 2;  //actual speed times 4, single byte

		// sent as 100 times value in liters per minute
		rateSetPointLeft = (float)(Serial.read() << 8 | Serial.read());   //high,low bytes
		rateSetPointLeft *= 0.01;
		rateSetPointRight = (float)(Serial.read() << 8 | Serial.read());   //high,low bytes
		rateSetPointRight *= 0.01;

		//UTurn byte
		uTurn = Serial.read();

		//reset watchdog as we just heard from AgOpenGPS
		watchdogTimer = 0;
	}

	//SETTINGS Header has been found,  6 bytes are the settings -- 127H + 248L = 32760
	if (Serial.available() == 6 && isSettingFound)
	{
		isSettingFound = false;  //reset the flag

		//accumulated volume, 0 it if 32700 is sent
		float tempf = (float)(Serial.read() << 8 | Serial.read());   //high,low bytes
		if (tempf == 32700)
		{
			accumulatedCountsLeft = 0;
			accumulatedCountsRight = 0;
		}

		//flow meter cal factor in counts per Liter
		flowmeterCalFactorLeft = ((float)(Serial.read() << 8 | Serial.read()));   //high,low bytes
		flowmeterCalFactorRight = ((float)(Serial.read() << 8 | Serial.read()));   //high,low bytes
	}
}

//ISR
void pinLeftChangeISR() {
	static unsigned long pulseStartLeft = 0;
	pulseCountLeft++;
	pulseDurationLeft += (millis() - pulseStartLeft); // get the pulse length
	pulseStartLeft = millis(); // store the current microseconds and start clock again
}

void pinRightChangeISR() {
	static unsigned long pulseStartRight = 0;
	pulseCountRight++;
	pulseDurationRight += (millis() - pulseStartRight); // get the pulse length
	pulseStartRight = millis(); // store the current microseconds and start clock again       ctr++;
}
