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
#include "plugin.hpp"
#include "panel_schema.hpp"
#include "voltage_helpers.hpp"

using dsp::TSchmittTrigger;
using dsp::TSlewLimiter;
using simd::float_4;

struct WindowGenerators : Module {
	enum ParamId {
		ENUMS(P_POT, 5),
		ENUMS(A_POT, 5),
		SHAPE_PARAM,
		BUT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(V_IN, 5),
		GATE_INPUT,
		TRIG_INPUT,
		VALL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(G_OUT, 5),
		DADSR_OUTPUT,
		AHDSR_OUTPUT,
		DAHR_OUTPUT,
		ADASR_OUTPUT,
		G0_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float envMax = 10.f;                // Maximum envelope voltage
	float_4 triggerGate;                // Stores trigger and gate input values
	TSchmittTrigger<float_4> tg;        // Schmitt Trigger for processing trigger and gate
	float_4 times, cvs;                 // Store all CVs: {T1,T2,T3,T4};{Sustain,All,Shape,-}
	// Global envelope current stage
	// Value | Stage
	// ------|-------
	// 0     | T1
	// 1     | T2
	// 2     | T3
	// 3     | SUSTAIN
	// 4     | T4
	// 5     | END
	unsigned char stage = 5;
	float_4 envTargets;                 // Voltage targets for slew limiters
	float_4 rises, falls;               // Slew rates for slew limiters
	TSlewLimiter<float_4> envs;         // Slew limiters acting as envelope generators
	float_4 envOuts = float_4::zero();  // Current/last states of the envelope generators

	WindowGenerators() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		std::string labels[5] = {"T1", "T2", "T3", "Sustain", "T4"};
		for (unsigned char i = 0; i < 5; i++) {
			std::string label = labels[i];
			configOutput(i, label + " Gate");
			configInput(i, label + " CV");
			configParam(i + 5, -1.f, 1.f, 0.f, label + " CV Attenuverter");
			if (i != 3) configParam(i, -6, 8.f, 1.f, label + " Time", "s", 0.5f, 0.5f);
		}
		configParam(P_POT + 3, 0.f, envMax, 0.5f * envMax, labels[3] + " Level", "V");
		configParam(BUT_PARAM, 0.f, 1.f, 0.f, "Manual Gate");
		configParam(SHAPE_PARAM, -1.f, 1.f, 0.f, "Shape (LOG-LIN-EXP)");
		configInput(GATE_INPUT, "Gate");
		configInput(TRIG_INPUT, "Trigger");
		configInput(VALL_INPUT, "CV for all Tx parameters");
		configOutput(DADSR_OUTPUT, "Delay-Attack-Decay-Sustain-Release");
		configOutput(AHDSR_OUTPUT, "Attack-Hold-Decay-Sustain-Release");
		configOutput(DAHR_OUTPUT, "Delay-Attack-Hold-Release");
		configOutput(ADASR_OUTPUT, "Attack-Decay-Attack-Sustain-Release");
		configOutput(G0_OUTPUT, "End Gate");
	}

	// Updates the stage based on global envelope value (ADASR)
	// ADASR was chosen because the value is slewed always in timed stages
	// (both DADSR and AHDSR have holding timed stage). This way we can
	// always compare the value with the target and update the stage when it is reached.
	unsigned char updateStage() {
		if (movemask(triggerGate) && stage > 2) return 0;   // Retrigger only if in SUSTAIN stage
		if (stage == 3) return 3 + (!tg.isHigh()[1]);       // Upgrade to RELEASE only when the gate is LOW
		if (stage == 5) return 5;                           // Do not upgrade RELEASE stage is over, stay in state 5
		return stage + (envOuts[3] == envTargets[3]);       // Otherwise, upgrade only if the target is reached.
	}

	// Updates slew voltage targets based on the current stage
	float_4 updateTargets() {
		if (stage < 2) {
			bool isDelayed = bool(stage);
			float delayed = envMax * isDelayed;
			return {delayed, envMax, delayed, envMax * !isDelayed};
		}
		if (stage < 4) {
			float sus = cvs[0];
			return {sus, sus, (stage - 2) ? 0.f : envMax, sus}; // Always sustain level except for DAHR
		}
		return float_4::zero();
	}

	// Converts voltage values (time-based) to frequency, regular (2**V) * 2 * VoltagePeakToPeak
	// This function also takes VC_ALL and SHAPE param into account and adds scaled envelopes' values
	float_4 voltageToTime(float_4 values) {
		return 2.f * envMax * pow(2.f, clamp(values + cvs[1] + (cvs[2] * envOuts), -6.f, 8.f));
	}

	void process(const ProcessArgs& args) override {
		// Process trigger and gate inputs
		triggerGate = tg.process(
			float_4(
				inputs[TRIG_INPUT].getVoltage(),
				inputs[GATE_INPUT].getVoltage() + gateOn * params[BUT_PARAM].getValue(),
				0.f, 0.f
			),
			triggerThresholdLevel, triggerThresholdLevel
		);
		// Calculate T1-T4 times, for now keep it in volts
		times = float_4(inputs[0].getVoltage(), inputs[1].getVoltage(), inputs[2].getVoltage(), inputs[4].getVoltage());
		times *= float_4(params[5].getValue(), params[6].getValue(), params[7].getValue(), params[9].getValue());
		times += float_4(params[0].getValue(), params[1].getValue(), params[2].getValue(), params[4].getValue());
		// Calculate SUSTAIN, VC_ALL and SHAPE
		cvs = float_4(inputs[3].getVoltage(), 0.f, 0.f, 0.f);
		cvs *= float_4(params[8].getValue(), 0.f, 0.f, 0.f);
		cvs += float_4(params[3].getValue(), inputs[VALL_INPUT].getVoltage(), params[SHAPE_PARAM].getValue(), 0.f);
		// Limit the SUSTAIN level
		cvs[0] = clamp(cvs[0], 0.f, envMax);
		// Update stage
		stage = updateStage();
		// Update voltage targets based on the current stage
		envTargets = updateTargets();
		// Update rise slew rate for slew limiters: {T2, T1, T2, T1 or T3}
		rises = voltageToTime(float_4(times[1], times[0], times[1], times[2 * (stage > 1)]));
		// Update fall slew rate for slew limiters: {T3 or T4, T3 or T4, T4, T2 or T4}
		bool inSustain = (stage > 2);
		float threeOrFour = times[2 + inSustain];
		falls = voltageToTime(float_4(threeOrFour, threeOrFour, times[3], times[1 + (2 * inSustain)]));
		// Update slew rates
		envs.setRiseFall(rises, falls);
		// Slew
		envOuts = clamp(envs.process(args.sampleTime, envTargets), 0.f, envMax);
		// Output
		for (unsigned char i = 0; i < 5; i++) {
			outputs[i].setVoltage(gateOn * (i == stage));      // Stage gate
			if (i < 4) outputs[i + 5].setVoltage(envOuts[i]);  // Envelopes' outputs
		}
		outputs[G0_OUTPUT].setVoltage(gateOn * (5 == stage));  // END gate
	}
};

struct WindowGeneratorsWidget : ModuleWidget {
	WindowGeneratorsWidget(WindowGenerators* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/WindowGenerators.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		for (unsigned char i = 0; i < 5; i++) {
			float x = xCoords(i);
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(0))), module, i + 5));
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(1))), module, i));
			if (i >= 1 && i <= 3) addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(2))), module, i + 4));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(3))), module, i));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(4))), module, i + 5));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(5))), module, i));
		}
		addParam(createParamCentered<CKD6>(mm2px(Vec(xCoords(0), yCoords(2))), module, WindowGenerators::BUT_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(xCoords(4), yCoords(2))), module, WindowGenerators::SHAPE_PARAM));
	}
};
Model* modelWindowGenerators = createModel<WindowGenerators, WindowGeneratorsWidget>("PatchableDevicesWindowGenerators");