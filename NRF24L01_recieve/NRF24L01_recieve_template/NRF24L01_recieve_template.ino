#include <SPI.h>
#include "RF24.h"

#define DEVICE_ID  0x90			//0x01-0x6F Transmitters
								//0x70-0x8F Routers
								//0x90-0xFF Recievers
#define MAX_RECIEVERS	6		//max number of recievers data will be sent to

#define MAX_PAYLOAD_SIZE	5	//1 Byte for the address and 4 Bytes Data will be transmitted max per transmission

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
RF24 radio(7,8);
//RF24 radio(16,15);
/**********************************************************/
                                                                           // Topology
uint8_t  routerAddresses[2][5];
uint8_t number_routers = 2;

uint8_t  rxPipes[6][5];	//maximum 6 parallel receptions possible
uint8_t number_rx_pipes = 0;

void setup(){

	Serial.begin(115200);
	Serial.println(F("NRF24L01 Reciever"));

	// Setup and configure radio
	  

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

void loop(void){
	uint8_t addr;
	uint8_t data[MAX_PAYLOAD_SIZE];
	memset(data, 0, MAX_PAYLOAD_SIZE);
	if(radio.available()){
		radio.read(data, MAX_PAYLOAD_SIZE);
		addr = data[0];
		Serial.print("Addr: " + String(addr) + "; Data: ");
		Serial.println(String((int)data[1])+", "+String((int)data[2]));
		if(addr == DEVICE_ID){
			uint8_t cmd = data[1];
			switch(cmd){
					break;
				case START_FUNCTION:
						Serial.println(F("START_FUNCTION"));
					break;
				case STOP_FUNCTION:
						Serial.println(F("STOP_FUNCTION"));
					break;
				case START_LED:
						Serial.println(F("START_LED"));
					break;
				case STOP_LED:
						Serial.println(F("STOP_LED"));
					break;
				case START_FUNCTION_SEC:
						Serial.println(F("START_FUNCTION_SEC"));
					break;
				case STOP_FUNCTION_SEC:
						Serial.println(F("START_FUNCTION_SEC"));
					break;
			}
		}
		while(radio.available()) radio.read(data, MAX_PAYLOAD_SIZE); //Empty RX buffer because we can't distinguish multiple recpetions
	}
}
