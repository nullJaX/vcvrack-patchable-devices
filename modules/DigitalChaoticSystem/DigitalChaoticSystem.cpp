// Copyright (C) 2023 Jacek Lewański
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
#include "../plugin.hpp"

using dsp::TRCFilter;
using dsp::TSchmittTrigger;
using simd::float_4;

struct DigitalChaoticSystem : Module {
	enum ParamId {
		ENUMS(RATE, 2),
		ENUMS(CV_ATT, 4),
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(CV, 4),
		VOCT1_INPUT,
		DATA_INPUT,
		CLOCK_INPUT,
		VOCT2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(VCOS, 4),
		STEPPED_OUTPUT,
		PULSED_OUTPUT,
		SMOOTHED_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	DigitalChaoticSystem() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		std::string vco[2] = {"Clock", "Data"};
		std::string waveforms[2] = {"Triangle", "Square"};
		for (unsigned char i = 0; i < 2; i++) {
			std::string vcoName = vco[i] + " Oscillator";
			configParam(i, -5.f, 15.f, 5.f, vcoName + " Frequency", "Hz", 2.f);
			configInput(i ? VOCT2_INPUT : VOCT1_INPUT, vcoName + " V/Oct");
			for (unsigned char j = 0; j < 2; j++) {
				float min = -1.f * (i ^ j);
				std::string att = min ? "Attenuverter" : "Attenuator";
				unsigned char twoJPlusI = (j << 1) + i;
				configInput(twoJPlusI, vcoName + " Frequency Modulation");
				configParam(2 + twoJPlusI, min, 1.f, 0.f, vcoName + " " + att);
				configOutput((i << 1) + j, vcoName + " " + waveforms[j]);
			}
		}
		configInput(CLOCK_INPUT, "Clock Trigger (normalized to Clock VCO Square)");
		configInput(DATA_INPUT, "Data Gate (normalized to Data VCO Square)");
		configOutput(STEPPED_OUTPUT, "Stepped");
		configOutput(PULSED_OUTPUT, "Pulsed");
		configOutput(SMOOTHED_OUTPUT, "Smooth");	
	}


	float_4 cvs;                        // Used for processing incoming CVs for both VCOs
	float_4 pitches;                    // Intermediate information for VCOs tuning
	float_4 phases = float_4::zero();   // VCOs phase state
	float_4 output;                     // VCOs output states (TRIANGLE_A, SQUARE_A, TRIANGLE_B, SQUARE_B)

	float clockInput;                   // Clock input value
	TSchmittTrigger<float> clock;       // Clock trigger input processing
	bool dataInput, xored;              // Data input state and XOR(data, shift_register(8))
	unsigned char shiftRegister;        // Shift register state
	float stepped;                      // Stepped function state (last 3 bits from shift register as 8 state analog value)
	TRCFilter<float> smooth;            // Used for generating smooth version of stepped signal

	void process(const ProcessArgs& args) override {
		// CV = k * X
		cvs = float_4(inputs[0].getVoltage(), inputs[1].getVoltage(), inputs[2].getVoltage(), inputs[3].getVoltage());
		cvs *= float_4(params[2].getValue(), params[3].getValue(), params[4].getValue(), params[5].getValue());
		// Convert voltages to pitches
		pitches[0] = params[0].getValue() + cvs[0] + cvs[2] + inputs[VOCT1_INPUT].getVoltage();
		pitches[1] = params[1].getValue() + cvs[1] + cvs[3] + inputs[VOCT2_INPUT].getVoltage();
		// To Hertz
		pitches = pow(2, clamp(pitches, -5.f, 15.f));
		// Accumulate phases
		phases += pitches * args.sampleTime;
		// Reset phases if needed
		phases += ifelse(phases >= 0.5f, -1.f, 0.f);
		// Assign to output values and generate waveforms
		output = float_4(abs(phases[0]), phases[0], abs(phases[1]), phases[1]);
		output -= float_4(0.25f, 0.f, 0.25f, 0.f);
		output = clamp(output * float_4(20.f, 1e5f, 20.f, 1e5f), -gateOn, gateOn);
		// Read clock and data inputs
		dataInput = ((inputs[DATA_INPUT].isConnected()) ? inputs[DATA_INPUT].getVoltage() : output[3]) > triggerThresholdLevel;
		clockInput = (inputs[CLOCK_INPUT].isConnected()) ? inputs[CLOCK_INPUT].getVoltage() : output[1];
		// Calculate XOR(data, shift_register(8))
		xored = dataInput ^ (shiftRegister & 0x01);
		// Update shift register on clock rising edge
		if (clock.process(clockInput, triggerThresholdLevel, triggerThresholdLevel)) {
			shiftRegister >>= 1;
			shiftRegister |= (xored << 7);
		}
		// Calculate stepped function
		stepped = .125f * gateOn * (shiftRegister & 0x07);
		// Calculate smoothed version of stepped function
		smooth.setCutoffFreq(20.f * args.sampleTime);
		smooth.process(stepped);
		// Output VCOs values
		for (unsigned char i = 0; i < 4; i++) {
			unsigned char twoI = (i << 1);
			outputs[twoI].setVoltage(output[twoI]);
			outputs[twoI + 1].setVoltage(output[twoI + 1]);
		}
		outputs[PULSED_OUTPUT].setVoltage(gateOn * xored);      // Pulsed is the XOR result
		outputs[STEPPED_OUTPUT].setVoltage(stepped);            // Output STEPPED
		outputs[SMOOTHED_OUTPUT].setVoltage(smooth.lowpass());  // Output SMOOTH
	}
};

struct DigitalChaoticSystemWidget : ModuleWidget {
	DigitalChaoticSystemWidget(DigitalChaoticSystem* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "modules/DigitalChaoticSystem/DigitalChaoticSystem.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		float xs[4] = {xCoords(0), xCoords(2), xCoords(1), xCoords(3)};
		for (unsigned char i = 0; i < 4; i++) {
			float x = xs[i];
			if (i < 2) {
				unsigned char outputX = 3 * i;
				unsigned char outputIdx = (i << 1);
				addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xs[outputX], yCoords(0))), module, outputIdx));
				addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xs[outputX], yCoords(1))), module, outputIdx + 1));
				addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x + xOffset, yCoords(5))), module, i));
			}
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(2))), module, i + 4));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(3))), module, i));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(4))), module, i + 2));
		}
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xs[1], yCoords(0))), module, DigitalChaoticSystem::SMOOTHED_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xs[2], yCoords(0))), module, DigitalChaoticSystem::STEPPED_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(0.5f * (xs[1] + xs[2]), yCoords(1))), module, DigitalChaoticSystem::PULSED_OUTPUT));
	}
};
Model* modelDigitalChaoticSystem = createModel<DigitalChaoticSystem, DigitalChaoticSystemWidget>("DigitalChaoticSystem");