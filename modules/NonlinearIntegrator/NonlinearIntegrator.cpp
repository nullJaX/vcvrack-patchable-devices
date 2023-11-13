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

using dsp::PulseGenerator;
using dsp::TSchmittTrigger;
using simd::float_4;

struct NonlinearIntegrator : Module {
	enum ParamId {
		INPOT_PARAM,
		F_PARAM,
		Q_PARAM,
		FATTV_PARAM,
		QATTV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		VOCT_INPUT,
		FCV_INPUT,
		QCV_INPUT,
		IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LP_OUTPUT,
		BP_OUTPUT,
		HP_OUTPUT,
		NP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	NonlinearIntegrator() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		std::string inputLabels[4] = {"Trigger", "Frequency V/Oct", "Frequency CV", "Resonance CV"};
		std::string modes[4] = {"Low", "Band", "High", "Notch"};
		for (unsigned char i = 0; i < 4; i++) {
			configOutput(i, modes[i]);
			configInput(i, inputLabels[i]);
		}
		configInput(IN_INPUT, "Signal");
		configParam(INPOT_PARAM, 0.f, 1.f, 0.f, "Signal attenuator");
		configParam(F_PARAM, -4.f, 13.f, -4.f, "Frequency", "Hz", 2.f);
		configParam(FATTV_PARAM, -1.f, 1.f, 0.f, "Frequency CV attenuverter");
		configParam(Q_PARAM, 0.f, 12.f, 0.f, "Resonance", "", 0.f, 1.f/12.f);
		configParam(QATTV_PARAM, -2.f, 2.f, 0.f, "Resonance CV attenuverter", "", 0.f, 0.5f);
	}

	TSchmittTrigger<float> st;                              // Used for detecting rising edge on PING input
	PulseGenerator pg;                                      // Used for generating short pulse when filter is pinged
	float_4 inputSignals;                                   // Storage for incoming voltages
	float_4 clampMin = {-12.f, -4.f, 0.f, 0.f};             // Lower limit values for inputSignals
	float_4 clampMax = {12.f, 13.f, 12.f, 0.f};             // Upper limit values for inputSignals
	float f, q, qMultiplier = -.05f * 108900.f / 15330.f;   // Filter parameters
	float_4 states = float_4::zero();                       // Filter states (LOWPASS, BANDPASS, HIGHPASS, NOTCH)

	void process(const ProcessArgs& args) override {
		// If the filter is pinged, generate a short pulse on input
		if (st.process(inputs[TRIG_INPUT].getVoltage(), triggerThresholdLevel, triggerThresholdLevel)) pg.trigger();
		// Process all inputs: y = (x * a) + b
		inputSignals = {inputs[IN_INPUT].getVoltage(), inputs[FCV_INPUT].getVoltage(), inputs[QCV_INPUT].getVoltage(), 0.f};
		inputSignals *= {params[INPOT_PARAM].getValue(), params[FATTV_PARAM].getValue(), params[QATTV_PARAM].getValue(), 0.f};
		// Here we use random to enable self oscillation when BANDPASS is connected back to INPUT
		inputSignals += {1e-6f * (2.f * random::uniform() - 1.f), params[F_PARAM].getValue() + inputs[VOCT_INPUT].getVoltage(), params[Q_PARAM].getValue(), 0.f};
		// Inject PING
		inputSignals[0] += 6.f * pg.process(args.sampleTime);
		// Limit the values to acceptable range
		inputSignals = clamp(inputSignals, clampMin, clampMax);
		// Update filter parameters
		f = 2.f * sin(M_PI * args.sampleTime * pow(2.f, inputSignals[1]));
		q = pow(10, qMultiplier * inputSignals[2]);
		// Update filter states
		states[3] = (q * states[1] - inputSignals[0]);
		states[2] = (-(states[3] + states[0]));
		states[1] = (states[1] + f * states[2]);
		states[0] = (states[0] + (f * states[1]));
		// Clamp the values
		states = clamp(states, vMin, vMax);
		// Output
		for (unsigned char i = 0; i < 4; i++) outputs[i].setVoltage(states[i]);
	}
};

struct NonlinearIntegratorWidget : ModuleWidget {
	NonlinearIntegratorWidget(NonlinearIntegrator* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "modules/NonlinearIntegrator/NonlinearIntegrator.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		for (unsigned char i = 0; i < 2; i++) {
			float x = xCoords(i + 1);
			unsigned char twoI = (i << 1);
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(3))), module, i + 2));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(4))), module, i + 3));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(5))), module, i + 1));
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xCoords(twoI), 0.5f * (yCoords(0) + yCoords(1)))), module, twoI));
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xCoords(1), yCoords(i))), module, twoI + 1));
		}
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xCoords(0), yCoords(3))), module, NonlinearIntegrator::InputId::TRIG_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xCoords(0), yCoords(4))), module, NonlinearIntegrator::InputId::IN_INPUT));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(xCoords(0), yCoords(5))), module, NonlinearIntegrator::ParamId::INPOT_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xCoords(1), yCoords(2))), module, NonlinearIntegrator::InputId::VOCT_INPUT));
	}
};
Model* modelNonlinearIntegrator = createModel<NonlinearIntegrator, NonlinearIntegratorWidget>("NonlinearIntegrator");