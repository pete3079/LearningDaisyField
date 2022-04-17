#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField field;
const unsigned int num_voices = 16;
class Voice
{
	public:
		Voice() {}
		~Voice() {}
		void Init(float sample_rate,float freq)
		{
			osc_.Init(sample_rate);
			osc_.SetAmp(1.f);
			osc_.SetWaveform(Oscillator::WAVE_SIN);
			osc_.SetFreq(freq);
			env_.Init(sample_rate);
			env_.SetSustainLevel(0.5f);
			env_.SetTime(ADSR_SEG_ATTACK,  0.005f);
			env_.SetTime(ADSR_SEG_DECAY,   0.010f);
			env_.SetTime(ADSR_SEG_RELEASE, 1.0f);
			filt_.Init(sample_rate);
			filt_.SetFreq(6000.f);
			filt_.SetRes(0.6f);
			filt_.SetDrive(0.8f);
		}

		float Process()
		{
			amp_ = env_.Process(env_gate_);
			sig_ = osc_.Process();
			return sig_*amp_;
		}

		///	private:
		Oscillator osc_;
		Svf        filt_;
		Adsr       env_;
		float      note_, velocity_, sig_, amp_;
		bool       active_;
		bool       env_gate_;
};

Voice v[num_voices];
float bright=0;
// Use two side buttons to change octaves.
float knob_vals[8];
float key_vals[16];


void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
		AudioHandle::InterleavingOutputBuffer out,
		size_t                                size) {
	field.ProcessDigitalControls();
	field.ProcessAnalogControls();
	for(size_t i = 0; i < 8; i++)
	{
		knob_vals[i] = field.GetKnobValue(i);
		key_vals[i]  = field.KeyboardState(i);
		key_vals[i+8]= field.KeyboardState(i+8);
	}
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

uint32_t display_time=0, led_time=0, adsr_time=0, wf_time=0, octave_time=0;
void UpdateDisplay(){
	//display
	char line1[32];
	if(System::GetNow()+1000>display_time){
		display_time=System::GetNow()+1000;
		snprintf(line1,32,"%d %d %d %d", int(100*knob_vals[0]), int(100*knob_vals[1]), int(100*knob_vals[2]), int(100*knob_vals[3]));
		field.display.SetCursor(0,0);
		field.display.WriteString(line1, Font_11x18 , true);
		snprintf(line1,32,"%d %d %d %d", int(100*knob_vals[4]), int(100*knob_vals[5]), int(88*knob_vals[6]), int(8*knob_vals[7]));
		field.display.SetCursor(0,20);
		field.display.WriteString(line1, Font_11x18 , true);
	}
	field.display.Update();
}

void UpdateKeyboardLeds(){
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
	uint8_t led_knob[] = {
		DaisyField::LED_KNOB_1,
		DaisyField::LED_KNOB_2,
		DaisyField::LED_KNOB_3,
		DaisyField::LED_KNOB_4,
		DaisyField::LED_KNOB_5,
		DaisyField::LED_KNOB_6,
		DaisyField::LED_KNOB_7,
		DaisyField::LED_KNOB_8};
	if(System::GetNow()+100>led_time){
		led_time=System::GetNow()+100;
		for(uint8_t i=0;i<8;i++){
			field.led_driver.SetLed(led_knob[i], float(knob_vals[i]));
			field.led_driver.SetLed(led_grp[i], float(key_vals[i]));
			field.led_driver.SetLed(led_grp[i+8], float(key_vals[i+8]));
		}
	}
	field.led_driver.SwapBuffersAndTransmit();
}

void UpdateADSR(){
	if(System::GetNow()+100>adsr_time){
		adsr_time=System::GetNow()+100;
		for(uint8_t i=0;i<num_voices;i++){
			v[i].env_.SetTime(ADSR_SEG_ATTACK,  knob_vals[0]*0.01f);
			v[i].env_.SetTime(ADSR_SEG_DECAY,   knob_vals[1]*0.041f);
			v[i].env_.SetSustainLevel(knob_vals[2]);
			v[i].env_.SetTime(ADSR_SEG_RELEASE, knob_vals[3]*1.f);
		}
	}
}


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

void UpdateOctave(){
	if(System::GetNow()+100>octave_time){
		octave_time=System::GetNow()+100;
		int start_note1=int(knob_vals[5]*100+12);
		int start_note2=int(knob_vals[6]*100+12);
	        for(unsigned int i=0;i<8;i++){
			v[i].osc_.SetFreq(mtof(start_note1+i));
			v[i+8].osc_.SetFreq(mtof(start_note2+i));
		}
	}
}


// Main -- Init, and Midi Handling
int main(void)
{
	// Init
	field.Init();
	float sample_rate=field.AudioSampleRate();
	for(unsigned int i=0;i<num_voices;i++)v[i].Init(sample_rate,mtof(i+69));


	// Start stuff.
	field.StartAdc();
	field.StartAudio(AudioCallback);
	for(;;) {
		UpdateDisplay();
		UpdateKeyboardLeds();
		UpdateADSR();
		UpdateWaveForm();
		UpdateOctave();
	}
}

