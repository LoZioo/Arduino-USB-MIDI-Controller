#include <Arduino.h>
#include <MIDIUSB.h>

#define MIDI_CC_OFF	0
#define MIDI_CC_ON	127

#define GPIO_PEDAL 8
#define DEBOUNCE_DELAY_MS 50

/**
 * @brief CONTROL CHANGE (CC)
 *  Ci sono 16 canali MIDI, ogni canale ha 128 Control Change (da 0 a 127).
 *  Servono per modificare parametri specifici o stati di singoli effetti (es. accendere/spegnere un pedale).
 *  Gestiscono valori continui o digitali: valori digitali usando solamente 0 o 127, o analogici in tutto il range da 0 a 127.
 *  Nota: Alcuni valori di `midi_control_index` seguono standard prestabiliti, quindi prima di usarne un indice nuovo, informarsi.
 *
 * @param midi_channel_index Indice del canale MIDI [0-15].
 * @param midi_control_index Indice del parametro di Control Change [0-127].
 * @param midi_control_value Valore da assegnare al parametro [0-127].
 */
void midi_control_change(uint8_t midi_channel_index, uint8_t midi_control_index, uint8_t midi_control_value){
	/**
	 * Struttura del pacchetto USB-MIDI nativo per Control Change:
	 *  Byte 0 (0x0B): Code Index Number (CIN) per un messaggio MIDI standard a 3 byte (Control Change).
	 *  Byte 1 (0xB0 | channel): Status byte (0xB = Control Change) combinato con il canale (0-15).
	 *  Byte 2: Numero del controller CC da modificare (es. 20).
	 *  Byte 3: Valore del comando (0 minimo, 127 massimo).
	 */
	midiEventPacket_t event = {
		0x0B,
		(uint8_t)(0xB0 | midi_channel_index),
		(uint8_t)constrain(midi_control_index, 0, 127),
		(uint8_t)constrain(midi_control_value, 0, 127)
	};

	MidiUSB.sendMIDI(event);
	MidiUSB.flush();
}

/**
 * @brief PROGRAM CHANGE (PC)
 *  Ci sono 16 canali MIDI, ogni canale ha 128 Program Change (da 0 a 127).
 *  Servono per cambiare il preset (es. patch del suono), a differenza dei CC che modificano singoli parametri o effetti all'interno dello stesso preset.
 *  A differenza dei CC, i PC non richiedono un valore di intensità (0-127), ma solo il numero del programma da richiamare.
 *
 * @param midi_channel_index Indice del canale MIDI [0-15].
 * @param midi_program_index Numero del programma/preset da caricare [0-127].
 */
void midi_program_change(uint8_t midi_channel_index, uint8_t midi_program_index){
	/**
	 * Struttura del pacchetto USB-MIDI nativo per Program Change:
	 *  Byte 0 (0x0C): Code Index Number (CIN) per un messaggio MIDI a 2 byte (Program Change).
	 *  Byte 1 (0xC0 | channel): Status byte (0xC = Program Change) combinato con il canale (0-15).
	 *  Byte 2: Numero del preset o programma da caricare (da 0 a 127).
	 *  Byte 3: Non utilizzato nei Program Change (lo impostiamo a 0).
	 */
	midiEventPacket_t event = {
		0x0C,
		(uint8_t)(0xC0 | midi_channel_index),
		(uint8_t)constrain(midi_program_index, 0, 127),
		0x00
	};

	MidiUSB.sendMIDI(event);
	MidiUSB.flush();
}

void setup(){
	pinMode(GPIO_PEDAL, INPUT_PULLUP);
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop(){
	static uint32_t t0 = 0;
	static uint8_t last_state = HIGH;
	static bool toggle = false;

	uint8_t reading = digitalRead(GPIO_PEDAL);
	if(reading != last_state)
		t0 = millis();

	if((millis() - t0) > DEBOUNCE_DELAY_MS){
		static uint8_t committed_state = HIGH;

		if(reading != committed_state){
			committed_state = reading;

			if(committed_state == HIGH){
				toggle = !toggle;
				digitalWrite(LED_BUILTIN, toggle ? HIGH : LOW);
				midi_control_change(0, 20, toggle * MIDI_CC_ON);
			}
		}
	}

	last_state = reading;
}
