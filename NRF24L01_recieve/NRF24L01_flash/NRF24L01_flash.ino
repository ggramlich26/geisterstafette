#include <SPI.h>
#include "RF24.h"
#include "stimer.h"
#include "dev.h"

#define DEVICE_ID  0x90			//0x01-0x6F Transmitters
								//0x70-0x8F Routers
								//0x90-0xFF Recievers
#define MAX_RECIEVERS	6		//max number of recievers data will be sent to

#define MAX_PAYLOAD_SIZE	5	//1 Byte for the address and 4 Bytes Data will be transmitted max per transmission

#define CE						16	//P16
#define	CSN						15	//P15

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
                      

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
//RF24 radio(7,8);
RF24 radio(CE,CSN);
/**********************************************************/
                                                                           // Topology
uint8_t  routerAddresses[2][5];
uint8_t number_routers = 2;

uint8_t  rxPipes[6][5];	//maximum 6 parallel receptions possible
uint8_t number_rx_pipes = 0;

void setup(){

//	Serial.begin(115200);
//	Serial.println("Blitz Empfaenger");


	// Setup and configure radio
	radio.begin();
	radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(108);
	radio.setPayloadSize(MAX_PAYLOAD_SIZE);

//	radio.enableAckPayload();                     // Allow optional ack payloads
//	radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

	
	//init rxPipes
	//always listen to manual remote
	rxPipes[number_rx_pipes][0] = 0x01;
	rxPipes[number_rx_pipes][1] = DEVICE_ID;
	rxPipes[number_rx_pipes][2] = 0x63;
	rxPipes[number_rx_pipes][3] = 0x02;
	rxPipes[number_rx_pipes][4] = 0x66;
	number_rx_pipes++;

	if(dev_getDip0()){
		rxPipes[number_rx_pipes][0] = 0x01 + 1;
		rxPipes[number_rx_pipes][1] = DEVICE_ID;
		rxPipes[number_rx_pipes][2] = 0x63;
		rxPipes[number_rx_pipes][3] = 0x02;
		rxPipes[number_rx_pipes][4] = 0x66;
		number_rx_pipes++;
	}
	if(dev_getDip1()){
		rxPipes[number_rx_pipes][0] = 0x01 + 2;
		rxPipes[number_rx_pipes][1] = DEVICE_ID;
		rxPipes[number_rx_pipes][2] = 0x63;
		rxPipes[number_rx_pipes][3] = 0x02;
		rxPipes[number_rx_pipes][4] = 0x66;
		number_rx_pipes++;
	}
	if(dev_getDip2()){
		rxPipes[number_rx_pipes][0] = 0x01 + 3;
		rxPipes[number_rx_pipes][1] = DEVICE_ID;
		rxPipes[number_rx_pipes][2] = 0x63;
		rxPipes[number_rx_pipes][3] = 0x02;
		rxPipes[number_rx_pipes][4] = 0x66;
		number_rx_pipes++;
	}
	if(dev_getDip3()){
		rxPipes[number_rx_pipes][0] = 0x01 + 4;
		rxPipes[number_rx_pipes][1] = DEVICE_ID;
		rxPipes[number_rx_pipes][2] = 0x63;
		rxPipes[number_rx_pipes][3] = 0x02;
		rxPipes[number_rx_pipes][4] = 0x66;
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

	dev_init();


	radio.startListening();
}

void loop(void){
	stimer_update();


	if(dev_getBatteryLow()){
		radio.powerDown();
		dev_stopStepup();
		while(true){
			dev_ledOn();
			delay(100);
			dev_ledOff();
			delay(5000);
			if(!dev_getBatteryLow()){
				radio.powerUp();
				delay(10);
				break;
			}
		}
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
						//digitalWrite(STEPUP_PIN, HIGH);
						dev_flash();
					break;
				case STOP_FUNCTION:
					//digitalWrite(STEPUP_PIN, LOW);
					break;
				case START_LED:
						dev_ledOn();
					break;
				case STOP_LED:
						dev_ledOff();
					break;
				case START_FUNCTION_SEC:
						dev_startStepup();
					break;
				case STOP_FUNCTION_SEC:
						dev_stopStepup();
					break;
			}
		}
		while(radio.available()) radio.read(data, MAX_PAYLOAD_SIZE); //Empty RX buffer because we can't distinguish multiple recpetions
	}
}
