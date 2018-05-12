#include <SPI.h>
#include "RF24.h"
#include "stimer.h"

#define DEVICE_ID  0x90			//0x01-0x6F Transmitters
								//0x70-0x8F Routers
								//0x90-0xFF Recievers
#define MAX_RECIEVERS	6		//max number of recievers data will be sent to

#define MAX_PAYLOAD_SIZE	5	//1 Byte for the address and 4 Bytes Data will be transmitted max per transmission

#define STEPUP_PIN				3	//OC2B = PD3 = P3
#define FLASH_PIN				5	//P5
#define STEPUP_VOLTAGE_PIN		7	//ADC7 = A7
#define BATTERY_VOLTAGE_PIN		6	//ADC6 = A6
#define LED_PIN					4	//P4
#define CE						16	//P16
#define	CSN						15	//P15
#define BATTERY_CONV_FACTOR		5*1.1
#define	STEPUP_CONV_FACTOR		300*1.1
#define	MIN_BAT_VOLTAGE			3.3

//////////////////////////////////////////////////////////////////////////
//								Idee									//
//																		//
//	Jedes Gerät hat eine 8-Bit ID (entspricht IP Adresse).				//
//	Außerdem hat jedes Gerät eine hwAdresse (entspricht mac Adresse)	//
//	Gesendet wird also an eine Reihe von hwAdressen, wobei jedes mal die//
//	ID des Zielgerätes übergeben wird. Auf diese Weise lässt sich ein	//
//	ein einfacher Router bauen, der Daten, die für ein anderes Gerät	//
//	bestimmt sind weiterleitet.											//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//							Namensgebung für Pipes						//
//																		//
//	Jeder Transmitter hat eine spezielle ID, diese ist das erste Byte	//
//	des Namens der Pipe.												//
//	Jeder Reciever hat eine spezielle ID, diese ist das zweite Byte		//
//	des Namens der Pipe.												//
//	Byte 2-4 bekommen einen default Wert, hier 0x630266					//
//																		//
//	Beispiel:															//
//		Transmitter 02, Reciever 83 -> Pipe: 0x0283630266				//
//////////////////////////////////////////////////////////////////////////


//commands
#define START_FUNCTION		0x01
#define STOP_FUNCTION		0x02
#define START_LED			0x03
#define STOP_LED			0x04
#define START_FUNCTION_SEC	0x05
#define STOP_FUNCTION_SEC	0x06
                      
void updateStepupFrequency();

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
//RF24 radio(7,8);
RF24 radio(CE,CSN);
/**********************************************************/
                                                                           // Topology
uint8_t  routerAddresses[2][5];
uint8_t number_routers = 2;

uint8_t  rxPipes[6][5];	//maximum 6 parallel receptions possible
uint8_t number_rx_pipes = 0;

bool stepup_enabled = false;

void setup(){

	Serial.begin(115200);
	Serial.println(F("NRF24L01 Reciever"));

	// Setup and configure radio

	analogReference(INTERNAL);	//use internal 1,1V Reference Voltage

	pinMode(LED_PIN, OUTPUT);
	pinMode(STEPUP_PIN, OUTPUT);
	pinMode(FLASH_PIN, OUTPUT);

	stimer_setCallback(updateStepupFrequency);

	//Initialize Timer 2 for PWM Mode on OC0A, fast PWM mode, count till MAX
	TCCR2A = (1<<COM2B1)|(1<<WGM21)|(1<<WGM20);
	  

	radio.begin();
	radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(108);
	radio.setPayloadSize(MAX_PAYLOAD_SIZE);

//	radio.enableAckPayload();                     // Allow optional ack payloads
//	radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

	
	//init transmit_array
	//todo: should be adapted to match for example the configuration set with dip switches
	for(uint8_t i = 0; i < 6; i++){		//replace 5 by the actual number of transmitters (without routers) that should be listened to
		rxPipes[i][0] = 0x01 + i;
		rxPipes[i][1] = DEVICE_ID;
		rxPipes[i][2] = 0x63;
		rxPipes[i][3] = 0x02;
		rxPipes[i][4] = 0x66;
		number_rx_pipes++;
	}

	// initialize router addresses
	for(uint8_t i = 0; i < number_routers; i++){
		routerAddresses[i][0] = 0x70 + i;
		routerAddresses[i][1] = DEVICE_ID;
		routerAddresses[i][2] = 0x63;
		routerAddresses[i][3] = 0x02;
		routerAddresses[i][4] = 0x66;
	}
	
	//copy router Addresses to rxPipes, so that we will listen to them, too
	for(uint8_t i = number_rx_pipes; i < 6 && i < number_rx_pipes + number_routers; i++){
		memcpy(rxPipes[i], routerAddresses[i-number_rx_pipes], 5);
	}
	number_rx_pipes = number_rx_pipes+number_routers<6?number_rx_pipes+number_routers:6;

	//open pipes for reading
	for(uint8_t i = 0; i < number_rx_pipes; i++){
		radio.openReadingPipe(i, rxPipes[i]);
	}


	//possibly init gpio


	radio.startListening();


}

void ledOn(){
	digitalWrite(LED_PIN, HIGH);
}

void ledOff(){
	digitalWrite(LED_PIN, LOW);
}

/** The stepup pin will be cleared when reaching val, so a value of 255
 *  results in one low cycle per Period
**/
void setStepupValue(uint8_t val){
	OCR2B = val;
}

void updateStepupFrequency(){
	uint8_t val[] = {250, 252, 254, 255};
	uint16_t voltage[] = {120, 220, 270, 300};
	uint16_t times[] = {700,1400,2100,3200};
	uint8_t steps = 4;
	uint16_t cur_voltage = analogRead(STEPUP_VOLTAGE_PIN)/1024.0*STEPUP_CONV_FACTOR;
	if(cur_voltage >= 300){
		stopStepup();
		stimer_startTimer(5000);
		return;
	}
	uint8_t i = 0;
	for(i = 0; i < steps; i++){
		if(cur_voltage < voltage[i]){
			break;
		}
	}
	setStepupValue(val[i]);
	if(i > 0){
		stimer_startTimer((times[i]-times[i-1])*(float)(voltage[i]-cur_voltage)/(voltage[i]-voltage[i-1]));
	}
	else{
		stimer_startTimer((times[0])*(float)(voltage[0])/(voltage[0]));
	}
}

void startStepup(){
	//start PWM with prescaler 8
	TCCR2B = (1<<CS21);
	updateStepupFrequency();
}

void stopStepup(){
	stimer_stopTimer();
	TCCR2B = 0x00;
	digitalWrite(STEPUP_PIN, LOW);
}

void flash(){
	stopStepup();
	digitalWrite(FLASH_PIN, HIGH);
	delay(5);
	digitalWrite(FLASH_PIN, LOW);
	if(stepup_enabled){
		startStepup();
	}
}

void loop(void){
	stimer_update();

	if(analogRead(BATTERY_VOLTAGE_PIN)*BATTERY_CONV_FACTOR < MIN_BAT_VOLTAGE){
		stepup_enabled = false;
		stopStepup();
		ledOff();
		//todo: possibly go to standbye mode, also for NRF24L01
	}


	uint8_t addr;
	uint8_t data[MAX_PAYLOAD_SIZE];
	memset(data, 0, MAX_PAYLOAD_SIZE);
	if(radio.available()){
		radio.read(data, MAX_PAYLOAD_SIZE);
		addr = data[0];
		
		//tread recieved data
		if(addr == DEVICE_ID){
			uint8_t cmd = data[1];
			switch(cmd){
					break;
				case START_FUNCTION:
						flash();
					break;
				case STOP_FUNCTION:
					break;
				case START_LED:
						ledOn();
					break;
				case STOP_LED:
						ledOff();
					break;
				case START_FUNCTION_SEC:
						stepup_enabled = true;
						startStepup();
					break;
				case STOP_FUNCTION_SEC:
						stepup_enabled = false;
						stopStepup();
					break;
			}
		}
		while(radio.available()) radio.read(data, MAX_PAYLOAD_SIZE); //Empty RX buffer because we can't distinguish multiple recpetions
	}
}
