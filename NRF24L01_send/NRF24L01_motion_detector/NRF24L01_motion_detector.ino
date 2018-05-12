#include <SPI.h>
#include "RF24.h"

#define DEVICE_ID  0x02			//0x01-0x6F Transmitters
								//0x70-0x8F Routers
								//0x90-0xFF Recievers
#define MAX_RECIEVERS	5		//max number of recievers data will be sent to

#define MAX_PAYLOAD_SIZE	5	//1 Byte for the address and 4 Bytes Data will be transmitted max per transmission

#define DIP_AVAILABLE			//device only send to recievers selected by dip switches if available and broadcast otherwise

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
                      

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 16 & 15 */
RF24 radio(16,15);
//RF24 radio(7,8);
/**********************************************************/

byte routerAddresses[2][5];
uint8_t number_routers = 0;

byte txPipes[MAX_RECIEVERS][5];
uint8_t dest_ID_array[MAX_RECIEVERS];
uint8_t number_recievers = 0;

//this will be used for listening to the handheld sender
uint8_t rxPipes[6][5];	//maximum 6 parallel receptions possible
uint8_t number_rx_pipes = 0;

void setup(){

//	Serial.begin(115200);

	// Setup and configure radio
	radio.begin();
	radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(108);
	radio.setPayloadSize(MAX_PAYLOAD_SIZE);

//	radio.enableAckPayload();                     // Allow optional ack payloads
//	radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

	dev_init();


	//init transmit_array
#ifdef DIP_AVAILABLE
	if(dev_getDip0()){
		txPipes[number_recievers][0] = DEVICE_ID;
		txPipes[number_recievers][1] = 0x90 + 0;
		txPipes[number_recievers][2] = 0x63;
		txPipes[number_recievers][3] = 0x02;
		txPipes[number_recievers][4] = 0x66;
		number_recievers++;
		dest_ID_array[number_recievers] = 0x90 + 0;
	}
	if(dev_getDip1()){
		txPipes[number_recievers][0] = DEVICE_ID;
		txPipes[number_recievers][1] = 0x90 + 1;
		txPipes[number_recievers][2] = 0x63;
		txPipes[number_recievers][3] = 0x02;
		txPipes[number_recievers][4] = 0x66;
		number_recievers++;
		dest_ID_array[number_recievers] = 0x90 + 1;
	}
	if(dev_getDip2()){
		txPipes[number_recievers][0] = DEVICE_ID;
		txPipes[number_recievers][1] = 0x90 + 2;
		txPipes[number_recievers][2] = 0x63;
		txPipes[number_recievers][3] = 0x02;
		txPipes[number_recievers][4] = 0x66;
		number_recievers++;
		dest_ID_array[number_recievers] = 0x90 + 2;
	}
	if(dev_getDip3()){
		txPipes[number_recievers][0] = DEVICE_ID;
		txPipes[number_recievers][1] = 0x90 + 3;
		txPipes[number_recievers][2] = 0x63;
		txPipes[number_recievers][3] = 0x02;
		txPipes[number_recievers][4] = 0x66;
		number_recievers++;
		dest_ID_array[number_recievers] = 0x90 + 3;
	}
#else
	for(uint8_t i = 0; i < MAX_RECIEVERS; i++){
		txPipes[i][0] = DEVICE_ID;
		txPipes[i][1] = 0x90 + i;
		txPipes[i][2] = 0x63;
		txPipes[i][3] = 0x02;
		txPipes[i][4] = 0x66;
		number_recievers++;
		dest_ID_array[i] = 0x90 + i;	//change to actual dest Addresses
	}
#endif

	// initialize router addresses
	for(uint8_t i = 0; i < number_routers; i++){
		routerAddresses[i][0] = DEVICE_ID;
		routerAddresses[i][1] = 0x70 + i;
		routerAddresses[i][2] = 0x63;
		routerAddresses[i][3] = 0x02;
		routerAddresses[i][4] = 0x66;
	}


	//initialize recieving

	//listen to handheld remote
	rxPipes[number_rx_pipes][0] = 0x01;
	rxPipes[number_rx_pipes][1] = DEVICE_ID;
	rxPipes[number_rx_pipes][2] = 0x63;
	rxPipes[number_rx_pipes][3] = 0x02;
	rxPipes[number_rx_pipes][4] = 0x66;
	number_rx_pipes++;

	//open pipes for reading
	for(uint8_t i = 0; i < number_rx_pipes; i++){
		radio.openReadingPipe(i, rxPipes[i]);
	}

	radio.startListening();
}

/**
 *	This function should only be called from within a send function because
 *	it doesn't call radio.stopListening() and radio.startListening()
 *	itself.
**/
void send_to_routers(uint8_t dest_ID, uint8_t *data, uint8_t data_len){
	if(data_len < MAX_PAYLOAD_SIZE){
		uint8_t payload[MAX_PAYLOAD_SIZE];
		payload[0] = dest_ID;
		memcpy(&(payload[1]), data, data_len);
		for(uint8_t i = 0; i < number_routers; i++){
			radio.openWritingPipe(routerAddresses[i]);
			radio.write(payload, data_len + 1);
		}
	}
}

/** This function will send the data given to one specified reciever.
  *	@address the HW address of the device to send to
  * @dest_ID the IP Address of the device to send to
  * @*data the actual data to be sent. It's lenght has to be <= MAX_PAYLOAD_SIZE-1
  * @data_len the length in Byte of the data
  * @force_direct false allows trying to send data via routers in case of failure to send
**/
bool unicast(uint8_t address[5], uint8_t dest_ID, uint8_t *data, uint8_t data_len, bool force_direct){
	//if data wont fit in one package refuse sending
	if(data_len >= MAX_PAYLOAD_SIZE){
		return false;
	}
	radio.stopListening();
	delay(10);
	radio.openWritingPipe(address);
	bool success = false;
	
	//copy address and data to one payload array
	uint8_t payload[MAX_PAYLOAD_SIZE];
	payload[0] = dest_ID;
	memcpy(&(payload[1]), data, data_len);

	//try sending for max 50ms
	unsigned long time = micros();
	while(micros() - time < 50000){
		if(radio.write(payload, data_len + 1)){
			success = true;
			break;
		}
		delay(1);
	}

	//try sending via routers in case of failure
	if(!success && !force_direct){
		//try sending to router
		send_to_routers(dest_ID, data, data_len);
	}

	radio.startListening();
	return success;
}

/** This function will send the data given to multiple reciever.
  *	@addresses an array of HW addresses of the devices to send to
  * @n_recievers the number of recievers the data should be sent to
  * @dest_ID an array of IP Addresses of the devices to send to
  * @*data the actual data to be sent. It's lenght has to be <= MAX_PAYLOAD_SIZE-1
  * @data_len the length in Byte of the data
  * @force_direct false allows trying to send data via routers in case of failure to send
**/
bool multicast(byte addresses[][5], uint8_t n_recievers, uint8_t dest_ID[], uint8_t *data, uint8_t data_len, bool force_direct){
	if(data_len >= MAX_PAYLOAD_SIZE){
		return false;
	}
	bool all_success = true;
	radio.stopListening();

	//create one array with all the data to transmit
	uint8_t payload[MAX_PAYLOAD_SIZE];
	memcpy(&(payload[1]), data, data_len);

	for(uint8_t i = 0; i < n_recievers; i++){
		//open writing pipe and adapt target address in payload array
		radio.openWritingPipe(addresses[i]);
		bool success = false;
		payload[0] = dest_ID[i];
	
		//Try writing for max 10ms
		unsigned long time = micros();
		while(micros() - time < 10000){
			if(radio.write(payload, data_len + 1)){
				success = true;
				break;
			}
			delay(1);
		}

		//Try sending via routers in case of failure
		if(!success && !force_direct){
			//try sending to router
			send_to_routers(dest_ID[i], data, data_len);
		}

		if(!success){
			all_success = false;
		}
	}
	radio.startListening();
	return all_success;
}


void loop(void){
	static bool sensor_status = false;
	bool new_sensor = dev_getSensor();
	//if sensor toggled
	if(new_sensor && !sensor_status){
		uint8_t cmd = START_FUNCTION;
		bool success = multicast(txPipes, number_recievers, dest_ID_array, &cmd, 1, false);
		if(!success){
			//sending failed
		}
	}
	sensor_status = new_sensor;


	//handle data reception
	uint8_t addr;
	uint8_t data[MAX_PAYLOAD_SIZE];
	memset(data, 0, MAX_PAYLOAD_SIZE);
	if(radio.available()){
		radio.read(data, MAX_PAYLOAD_SIZE);
		addr = data[0];

		//treat recieved data
		if(addr == DEVICE_ID){
			uint8_t cmd = data[1];
			switch(cmd){
					break;
				case START_FUNCTION:
					break;
				case STOP_FUNCTION:
					break;
				case START_LED:
						dev_ledOn();
					break;
				case STOP_LED:
						dev_ledOff();
					break;
				case START_FUNCTION_SEC:
					break;
				case STOP_FUNCTION_SEC:
					break;
			}
		}
		while(radio.available()) radio.read(data, MAX_PAYLOAD_SIZE); //Empty RX buffer because we can't distinguish multiple recpetions
	}
}
