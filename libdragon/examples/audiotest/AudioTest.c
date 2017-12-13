#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <libdragon.h>

short nextSinSample(double sampleFrequency, double waveFrequency, double timeIndex) {
	return (short)( 32767.0 * sin( ( ( 2.0 * M_PI ) * waveFrequency * timeIndex ) / sampleFrequency ) );
}

int main(void) {

	init_interrupts();
	controller_init();
	console_init();
	audio_init(44100, 4);

	double sampleFreq = audio_get_frequency() * 1.0;
	double waveFreq = 100.0;
	double timeIndex = 0.0;

	int buffer_len = audio_get_buffer_length();
	short *audio_buffer = (short*)malloc(buffer_len*sizeof(short)*2);

	while(1) {

		controller_scan();

		struct controller_data keys_pressed = get_keys_down();
		struct controller_data keys_held = get_keys_held();
		struct SI_condat pressed = keys_pressed.c[0];
		struct SI_condat held = keys_held.c[0];

		if(pressed.up) {
			waveFreq += 20.0;
		}

		if(pressed.down) {

			waveFreq -= 20.0;

			if(waveFreq < 0.0) {
				waveFreq = 0.0;
			}
		}

		if(pressed.A) {
			waveFreq = 0.0;
		}

		if(pressed.B) {
			waveFreq = 22050.0;
		}

		if(held.right) {
			waveFreq += 100.0;
		}

		if(held.left) {

			waveFreq -= 100.0;

			if(waveFreq < 0.0) {
				waveFreq = 0.0;
			}
		}

		printf("Current frequency: %f Hz\n", waveFreq);

		if(audio_can_write()) {

			for(int i=0;i<buffer_len;i++) {

				short nextVal = nextSinSample(sampleFreq, waveFreq, timeIndex);

	                        timeIndex += 1.0;

				audio_buffer[(i*2) + 0] = nextVal;
				audio_buffer[(i*2) + 1] = nextVal;
			}

			audio_write(audio_buffer);
		}
	}

	return 0;
}
