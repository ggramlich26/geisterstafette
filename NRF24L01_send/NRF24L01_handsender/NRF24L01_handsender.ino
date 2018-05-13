#include <SPI.h>
#include "RF24.h"
#include "Button.h"
#include "LedBlink.h"

#define DEVICE_ID  0x01			//0x01-0x6F Transmitters
								//0x70-0x8F Routers
								//0x90-0xFF Recievers
#define TOTAL_RECIEVERS	6		//max number of recievers data will be sent to
#define TOTAL_TRANSMITTERS	3	//max number of transmitters data will be sent to (for turning on their LED)

#define MAX_PAYLOAD_SIZE	5	//1 Byte for the address and 4 Bytes Data will be transmitted max per transmission

#define	LED_1		A3
#define LED_2		10
#define ON_1		5
#define ON_2		4
#define ON_3		3
#define ON_4		2
#define OFF_1		6
#define OFF_2		7
#define OFF_3		8
#define OFF_4		9
#define MODE_1		A4
#define MODE_2		A5

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

//////////////////////////////////////////////////////////////////////////
//							Beschreibung Handsender						//
//																		//
//	Die linke Tastenreihe dient zum anschalten, die rechte zum			//
//	ausschalten. Des Weiteren gibt es ganz links zwei Funktionstasten.	//
//	Ohne Funktionstaste wird FUNCTION_ON bzw. FUNCTON_OFF gesendet.		//
//	Mit vorher aktivierter Funktionstaste 1 (LED grün) wird LED_ON/OFF	//
//	gesendet und mit vorher aktivierter Funktionstaste 2 (LED rot)		//
//	wird FUNCTON_ON_SEC bzw. FUNCTON_OFF_SEC gesendet.					//
//	Sind beide Funktionstasten aktiviert (LED orange), wird nichts		//
//	gesendet.															//
//	Die Adressen, an die gesendet werden, werden von 90 an hochgezählt.	//
//	Allerdings hat die vierte Reihe eine Sonderfunktion und zählt nicht	//
//	mit. Mit einem langen Tastendruck (1s) kann weiter gezählt werden.	//
//	So kann mit der ersten Reihe sowohl die Adresse 90 als auch 93		//
//	angesteuert werden.													//
//	Reihe 4 sendet bei kurzem Druck an die Adressen 90-92 (das sollten)	//
//	alle Blitze sein. Bei einem langen Druck sendet sie an alle 		//
//	hinterlegten Adressen. Sollten welche hinzukommen, muss				//
//	TOTAL_RECIEVERS angepasst werden. Sofern die neu hinzugekommenen	//
//	Adressen einfach weiter hochgezählt wurden, sind diese dann 		//
//	automatisch im Broadcast enthalten.									//
//																		//
//////////////////////////////////////////////////////////////////////////


//commands
#define START_FUNCTION		0x01
#define STOP_FUNCTION		0x02
#define START_LED			0x03
#define STOP_LED			0x04
#define START_FUNCTION_SEC	0x05
#define STOP_FUNCTION_SEC	0x06
#define	INVALID				0xFF
                      

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(16,15);
/**********************************************************/

void button_callback_handler(btn_action action, int pin);
                                                                           // Topology
byte routerAddresses[2][5];
uint8_t number_routers = 0;

byte txPipes[TOTAL_RECIEVERS][5];	//HW Addresses
uint8_t dest_ID_array[TOTAL_RECIEVERS];		//IP Addresses
uint8_t number_recievers = 0;

//arrays for sending to transmitters (for turning on their LED)
byte trans_txPipes[TOTAL_TRANSMITTERS][5];
uint8_t trans_dest_ID_array[TOTAL_TRANSMITTERS];
uint8_t number_transmitters = 0;

void setup(){

//	Serial.begin(115200);

	// Setup and configure radio
	  

	radio.begin();
	//radio.setPALevel(RF24_PA_MAX);
	radio.setPALevel(RF24_PA_MAX);
	radio.setChannel(108);
	radio.setPayloadSize(MAX_PAYLOAD_SIZE);

	btn_track_pin(ON_1, button_callback_handler, true);
	btn_track_pin(ON_2, button_callback_handler, true);
	btn_track_pin(ON_3, button_callback_handler, true);
	btn_track_pin(ON_4, button_callback_handler, true);
	btn_track_pin(OFF_1, button_callback_handler, true);
	btn_track_pin(OFF_2, button_callback_handler, true);
	btn_track_pin(OFF_3, button_callback_handler, true);
	btn_track_pin(OFF_4, button_callback_handler, true);
	btn_track_pin(MODE_1, button_callback_handler, true);
	btn_track_pin(MODE_2, button_callback_handler, true);

	blk_init(LED_1, LED_2);

//	radio.enableAckPayload();                     // Allow optional ack payloads
//	radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

	
	//init transmit_array
	//As all recievers should have sequential numbers starting from 90, it is legit
	//to initialize them in a loop if the total number of recievers is known
	for(uint8_t i = 0; i < TOTAL_RECIEVERS; i++){
		txPipes[i][0] = DEVICE_ID;
		txPipes[i][1] = 0x90 + i;
		txPipes[i][2] = 0x63;
		txPipes[i][3] = 0x02;
		txPipes[i][4] = 0x66;
		dest_ID_array[i] = 0x90 + i;	//change to actual dest Addresses
		number_recievers++;
	}

	// initialize router addresses
	for(uint8_t i = 0; i < number_routers; i++){
		routerAddresses[i][0] = DEVICE_ID;
		routerAddresses[i][1] = 0x70 + i;
		routerAddresses[i][2] = 0x63;
		routerAddresses[i][3] = 0x02;
		routerAddresses[i][4] = 0x66;
	}

	//init array of destinations that are transmitters
	//As all other transmitters should have sequential numbers starting from 02, it is legit
	//to initialize them in a loop if the total number of recievers is known
	for(uint8_t i = 0; i < TOTAL_TRANSMITTERS; i++){
		trans_txPipes[i][0] = DEVICE_ID;
		trans_txPipes[i][1] = 0x02 + i;
		trans_txPipes[i][2] = 0x63;
		trans_txPipes[i][3] = 0x02;
		trans_txPipes[i][4] = 0x66;
		trans_dest_ID_array[i] = 0x02 + i;
		number_transmitters++;
	}
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
	btn_main();
	blk_main();
}

bool mode1 = false;
bool mode2 = false;
long last_mode_time = 0;
bool dir_on = true;
#define MAX_SIMULTANEOUS_DELAY	250	//time in ms that can pass between the recognition of two button presses so that they will still be considered sumultaneous

void button_callback_handler(btn_action action, int pin){
	if(pin == ON_1){
		uint8_t addr_offset = 0+(long_press==action?3:0);
		uint8_t cmd;
		bool success = false;
		bool send_to_trans = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = START_LED;
		}
		else if(mode1){
			cmd = START_LED;
		}
		else if(mode2){
			cmd = START_FUNCTION_SEC;
		}
		else{
			cmd = START_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(addr_offset < number_transmitters){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == OFF_1){
		uint8_t addr_offset = 0+(long_press==action?3:0);
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = STOP_LED;
		}
		else if(mode1){
			cmd = STOP_LED;
		}
		else if(mode2){
			cmd = STOP_FUNCTION_SEC;
		}
		else{
			cmd = STOP_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(number_transmitters > addr_offset){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == ON_2){
		uint8_t addr_offset = 1+(long_press==action?3:0);
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = START_LED;
		}
		else if(mode1){
			cmd = START_LED;
		}
		else if(mode2){
			cmd = START_FUNCTION_SEC;
		}
		else{
			cmd = START_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(number_transmitters > addr_offset){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == OFF_2){
		uint8_t addr_offset = 1+(long_press==action?3:0);
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = STOP_LED;
		}
		else if(mode1){
			cmd = STOP_LED;
		}
		else if(mode2){
			cmd = STOP_FUNCTION_SEC;
		}
		else{
			cmd = STOP_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(number_transmitters > addr_offset){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == ON_3){
		uint8_t addr_offset = 2+(long_press==action?3:0);
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = START_LED;
		}
		else if(mode1){
			cmd = START_LED;
		}
		else if(mode2){
			cmd = START_FUNCTION_SEC;
		}
		else{
			cmd = START_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(number_transmitters > addr_offset){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == OFF_3){
		uint8_t addr_offset = 2+(long_press==action?3:0);
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = STOP_LED;
		}
		else if(mode1){
			cmd = STOP_LED;
		}
		else if(mode2){
			cmd = STOP_FUNCTION_SEC;
		}
		else{
			cmd = STOP_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				success = unicast(txPipes[addr_offset], dest_ID_array[addr_offset], &cmd, 1, false);
			}
			else if(number_transmitters > addr_offset){
				success = unicast(trans_txPipes[addr_offset], trans_dest_ID_array[addr_offset], &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == ON_4){
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = START_LED;
		}
		else if(mode1){
			cmd = START_LED;
		}
		else if(mode2){
			cmd = START_FUNCTION_SEC;
		}
		else{
			cmd = START_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				if(long_press!=action){
					//send command to the first three devices (should be all flashes)
					success = multicast(txPipes, 3, dest_ID_array, &cmd, 1, false);
				}
				else{
					//send command as broadcast to every device on long press
					success = multicast(txPipes, TOTAL_RECIEVERS, dest_ID_array, &cmd, 1, false);
				}
			}
			else{
					success = multicast(trans_txPipes, TOTAL_TRANSMITTERS, trans_dest_ID_array, &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == OFF_4){
		uint8_t cmd;
		bool send_to_trans = false;
		bool success = false;
		if(mode1 && mode2){
			send_to_trans = true;
			cmd = STOP_LED;
		}
		else if(mode1){
			cmd = STOP_LED;
		}
		else if(mode2){
			cmd = STOP_FUNCTION_SEC;
		}
		else{
			cmd = STOP_FUNCTION;
		}
		if(cmd != INVALID){
			if(!send_to_trans){
				if(long_press!=action){
					//send command to the first three devices (should be all flashes)
					success = multicast(txPipes, 3, dest_ID_array, &cmd, 1, false);
				}
				else{
					//send command as broadcast to every device on long press
					success = multicast(txPipes, TOTAL_RECIEVERS, dest_ID_array, &cmd, 1, false);
				}
			}
			else{
					success = multicast(trans_txPipes, TOTAL_TRANSMITTERS, trans_dest_ID_array, &cmd, 1, false);
			}
		}
		if(!success){
			blk_set_mode(three_fast, red);
		}
		else{
			blk_set_mode(off, green);
		}
		mode1 = false;
		mode2 = false;
	}
	else if(pin == MODE_1){
		if(millis()-last_mode_time > MAX_SIMULTANEOUS_DELAY){
			if(mode1 && mode2){
				dir_on = false;
				mode1 = true;
				blk_set_mode(steady, green);
			}
			else if(mode1){
				dir_on = false;
				mode1 = false;
				blk_set_mode(off, green);
			}
			else{
				dir_on = true;
				mode1 = true;
				blk_set_mode(steady, green);
			}
			mode2 = false;
		}
		else if(mode2){
			if(dir_on){
				mode1 = true;
				blk_set_mode(steady, orange);
			}
			else{
				mode1 = false;
				mode2 = false;
				blk_set_mode(off, orange);
			}
		}
		else{
			mode1 = true;
			dir_on = true;
			blk_set_mode(steady, green);
		}
		last_mode_time = millis();
	}
	else if(pin == MODE_2){
		if(millis()-last_mode_time > MAX_SIMULTANEOUS_DELAY){
			if(mode1 && mode2){
				dir_on = false;
				mode2 = true;
				blk_set_mode(steady, red);
			}
			else if(mode2){
				dir_on = false;
				mode2 = false;
				blk_set_mode(off, red);
			}
			else{
				dir_on = true;
				mode2 = true;
				blk_set_mode(steady, red);
			}
			mode1 = false;
		}
		else if(mode1){
			if(dir_on){
				mode2 = true;
				blk_set_mode(steady, orange);
			}
			else{
				mode1 = false;
				mode2 = false;
				blk_set_mode(off, orange);
			}
		}
		else{
			mode2 = true;
			dir_on = true;
			blk_set_mode(steady, red);
		}
		last_mode_time = millis();
	}
}
