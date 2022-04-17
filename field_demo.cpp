#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
// the class that contains info about the field is called field - in a lot of examples it's called hw
DaisyField field;
// unique voice for every button on the field - do we need this - no! It was more a curiosity - something similar to the Midi.cpp example
const unsigned int num_voices = 16;

// Again - I think this is a modified version of the Midi.cpp example
class Voice
{
	public:
		Voice() {}
		~Voice() {}
	        // Initialization call for each voice
		void Init(float sample_rate,float freq)
		{
			// Initialize the oscillator and set a few params
			osc_.Init(sample_rate);
			osc_.SetAmp(1.f);
			osc_.SetWaveform(Oscillator::WAVE_SIN);
			osc_.SetFreq(freq);
			// Initialize the ADSR and set a few default params
			env_.Init(sample_rate);
			env_.SetSustainLevel(0.5f);
			env_.SetTime(ADSR_SEG_ATTACK,  0.005f);
			env_.SetTime(ADSR_SEG_DECAY,   0.010f);
			env_.SetTime(ADSR_SEG_RELEASE, 1.0f);
			// Not using this yet
			filt_.Init(sample_rate);
			filt_.SetFreq(6000.f);
			filt_.SetRes(0.6f);
			filt_.SetDrive(0.8f);
		}

		float Process()
		{
			// get the scaling from the ADSR for the current sample
			amp_ = env_.Process(env_gate_);
			// get the oscillator value for the current sample
			sig_ = osc_.Process();
			// multiply them together and return the value the AudioCallBack interrupt routine
			return sig_*amp_;
		}
 
	        // These should probably be private but really just getting started with making beeps
		///	private:
		Oscillator osc_;
		Svf        filt_;
		Adsr       env_;
		float      note_, velocity_, sig_, amp_;
		bool       active_;
		bool       env_gate_;
};

// global access to the 16 voices - v
Voice v[num_voices];
float bright=0;
// global access to the button and knob values - I'm updating these in the AudioCallBack interrupt routine - is this good practice? 
// Use two side buttons to change octaves.
float knob_vals[8];
// I guess this could be a bool?
uint_8 key_vals[16];

// the AudioCallback interrupt routine - this routine should be light on it's feet
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
		AudioHandle::InterleavingOutputBuffer out,
		size_t                                size) {
	// update digital controls - buttons 
	field.ProcessDigitalControls();
	// update analog controls - knobs
	field.ProcessAnalogControls();
	// update the arrays that contain these values
	for(size_t i = 0; i < 8; i++)
	{
		// knobs are analog - 0 to 1 float
		knob_vals[i] = field.GetKnobValue(i);
		// keyboards states are 0 or 1
		key_vals[i]  = field.KeyboardState(i);
		key_vals[i+8]= field.KeyboardState(i+8);
	}
	// update the audio out
	// out[i] is stereo left
	// out[i+1] is stereo right
	// this is mono so they are the same - did the processing on the left channel and copied it to right to save time
	for(size_t i = 0; i < size; i += 2)
	{
		out[i]        = 0.f;
		for(unsigned int j = 0; j < num_voices; j++) {
			v[j].env_gate_=field.KeyboardRisingEdge(j);
			out[i]     +=v[j].Process();
		}
		out[i+1]=out[i];
	}
}

// update the LED display - use this for debugging - mostly I write out knob values
// I use these times to put a delay in for using interface - we don't want to update the display every microsecond - it will never finish
// writing and will look glitchy 
uint32_t display_time=0, led_time=0, adsr_time=0, wf_time=0, octave_time=0;
void UpdateDisplay(){
	//display
	char line1[32];
	// update every second
	if(System::GetNow()+1000>display_time){
		display_time=System::GetNow()+1000;
		// create a character string of what is written to the top row (0,0) location
		snprintf(line1,32,"%d %d %d %d", int(100*knob_vals[0]), int(100*knob_vals[1]), int(100*knob_vals[2]), int(100*knob_vals[3]));
		field.display.SetCursor(0,0);
		// write it
		field.display.WriteString(line1, Font_11x18 , true);
		// create a character string of what is writtent to the 2nd row (0,20) position
		snprintf(line1,32,"%d %d %d %d", int(100*knob_vals[4]), int(100*knob_vals[5]), int(88*knob_vals[6]), int(8*knob_vals[7]));
		field.display.SetCursor(0,20);
		// write it 
		field.display.WriteString(line1, Font_11x18 , true);
	}
	// tell the display to update
	field.display.Update();
}

// Want to make the keyboard lights flash when pressed
void UpdateKeyboardLeds(){
	// seems sort of like the daisy way to do this - the DaisyField class has variables defined that corresponde to the array location of each
	// key in the hardware
	// so LED_KEY_A1 == 0, LED_KEY_A2 == 1 etc etc
	uint8_t led_grp[] = {
		DaisyField::LED_KEY_A1,
		DaisyField::LED_KEY_A2,
		DaisyField::LED_KEY_A3,
		DaisyField::LED_KEY_A4,
		DaisyField::LED_KEY_A5,
		DaisyField::LED_KEY_A6,
		DaisyField::LED_KEY_A7,
		DaisyField::LED_KEY_A8,
		DaisyField::LED_KEY_B1,
		DaisyField::LED_KEY_B2,
		DaisyField::LED_KEY_B3,
		DaisyField::LED_KEY_B4,
		DaisyField::LED_KEY_B5,
		DaisyField::LED_KEY_B6,
		DaisyField::LED_KEY_B7,
		DaisyField::LED_KEY_B8};
	// same w/ knobs
	uint8_t led_knob[] = {
		DaisyField::LED_KNOB_1,
		DaisyField::LED_KNOB_2,
		DaisyField::LED_KNOB_3,
		DaisyField::LED_KNOB_4,
		DaisyField::LED_KNOB_5,
		DaisyField::LED_KNOB_6,
		DaisyField::LED_KNOB_7,
		DaisyField::LED_KNOB_8};
	// every 100 microseconds update the keyboard lights and knob led brigthness - fun and good for debugging feedback
	if(System::GetNow()+100>led_time){
		led_time=System::GetNow()+100;
		for(uint8_t i=0;i<8;i++){
			// set led brightness - float from 0 to 1
			field.led_driver.SetLed(led_knob[i], float(knob_vals[i]));
			// set key led brightness - you could have some fun here with setting the key brightness to the voice intensity
			field.led_driver.SetLed(led_grp[i], float(key_vals[i]));
			field.led_driver.SetLed(led_grp[i+8], float(key_vals[i+8]));
		}
	}
	// tell the field to update the key leds
	field.led_driver.SwapBuffersAndTransmit();
}

// Update the ADSR global parameters based on knobs 1 2 3 4 (indices 0, 1, 2, 3) - should use DaisyField::KNOB_1 etc.!!!
void UpdateADSR(){
	// update every 100 microseconds
	if(System::GetNow()+100>adsr_time){
		adsr_time=System::GetNow()+100;
		// should modify this so each voice gets a unique setting - maybe using switch 1 or 2?
		for(uint8_t i=0;i<num_voices;i++){
			v[i].env_.SetTime(ADSR_SEG_ATTACK,  knob_vals[0]*0.01f);
			v[i].env_.SetTime(ADSR_SEG_DECAY,   knob_vals[1]*0.041f);
			v[i].env_.SetSustainLevel(knob_vals[2]);
			v[i].env_.SetTime(ADSR_SEG_RELEASE, knob_vals[3]*1.f);
		}
	}
}

// Give the user the ability to change waveforms on the fly using the location of knob[7] - again DaisyField::KNOB_8
void UpdateWaveForm(){
	// WAVE_SIN,
	// WAVE_TRI,
	// WAVE_SAW,
	// WAVE_RAMP,
	// WAVE_SQUARE,
	// WAVE_POLYBLEP_TRI,
	// WAVE_POLYBLEP_SAW,
	// WAVE_POLYBLEP_SQUARE,
	if(System::GetNow()+100>wf_time){
		wf_time=System::GetNow()+100;
		for(uint8_t i=0;i<num_voices;i++){
			v[i].osc_.SetWaveform(int(knob_vals[7]*8));
		}
	}
}

// Let's change the frequency that the each row of voices starts at using konb[5] and knob[6] 
// I know I know - DaisyField::KNOB_6 and DaisyField::KNOB_7
void UpdateOctave(){
	if(System::GetNow()+100>octave_time){
		octave_time=System::GetNow()+100;
		int start_note1=int(knob_vals[5]*100+12);
		int start_note2=int(knob_vals[6]*100+12);
	        for(unsigned int i=0;i<8;i++){
			// mtof is a helper function that translates MIDI# to frequency in Hz - SetFreq want's Hz
			// see https://en.wikipedia.org/wiki/Piano_key_frequencies
			v[i].osc_.SetFreq(mtof(start_note1+i));
			v[i+8].osc_.SetFreq(mtof(start_note2+i));
		}
	}
}


// Main -- Initialize and call routines for updating
int main(void)
{
	// Init
	field.Init();
	// Get the sample rate 
	float sample_rate=field.AudioSampleRate();
	// Initialize each voice
	for(unsigned int i=0;i<num_voices;i++)v[i].Init(sample_rate,mtof(i+69));


	// Initialize the ADC - this gives the program the ability to read the knobs 
	// Don't skip this you'll be wasting a time wondering why your knobs are not updating (*blush*)
	field.StartAdc();
	// Start the AudioCallback! 
	field.StartAudio(AudioCallback);
	// OK - push buttons - turn knobs - make sounds
	for(;;) {
		UpdateDisplay();
		UpdateKeyboardLeds();
		UpdateADSR();
		UpdateWaveForm();
		UpdateOctave();
	}
}

