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
using simd::float_4;

struct VoltageSequencer : Module {
	enum ParamId {
		ENUMS(A_PARAM, 8),
		ENUMS(B_PARAM, 8),
		ENUMS(MAN_PARAM, 8),
		CLOCK_EN_PARAM,
		VCLOCK_EN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(GATEIN_INPUT, 8),
		RESET_INPUT,
		PRESET_INPUT,
		HOLD_INPUT,
		DIRECTION_INPUT,
		CLOCK_IN_INPUT,
		VCLOCK_IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(GATEOUT_OUTPUT, 8),
		ALLGATES_OUTPUT,
		A_OUT_OUTPUT,
		B_OUT_OUTPUT,
		A_B_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		STAGE_OUTPUT,
		AB_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(LED_LIGHT, 8),
		ENUMS(LEDSEL, 2),
		LIGHTS_LEN
	};

	float_4 signals;                            // For priority trigger inputs: DIRECTION, VERTICAL_CLOCK, RESET
	TSchmittTrigger<float_4> sigTriggers;       // Processing priority triggers
	unsigned char direction = 0;                // Sequencer direction: 0 (to the right), 1 (to the left)
	unsigned char vStage = 1;                   // Sequencer vertical stage: 0 (Row A), 1 (Row B)
	unsigned char changeInfo = 0x00;            // Signal Bitmask (alligned to right): Reset, Preset, Clock, Stage Selected
	TSchmittTrigger<float> presetTrig, clock;   // Additional triggers
	signed char stage = 0, newStage = 0;        // Sequencer stage and new stage
	unsigned char preset = 0;                   // Sequencer preset stage
	float stageVoltageFactor = 1.f / 6.f;       // Whole tone step for STAGE output

	VoltageSequencer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (unsigned char i = 0; i < 8; i++) {
			std::string stageStr = "Stage " + std::to_string(i + 1);
			configParam(i, 0.f, 5.f, 0.f, stageStr + "A", "V");
			configParam(i + 8, 0.f, 5.f, 0.f, stageStr + "B", "V");
			configParam(i + 16, 0.f, 1.f, 0.f, stageStr + " Manual Select");
			configInput(i, stageStr + " Select Trigger");
			configOutput(i, stageStr + " Gate");
		}
		for (unsigned char i = 0; i < 2; i++) {
			std::string verticalStr = (i ? "Vertical " : "");
			std::string resetPreset = (i ? "Preset" : "Reset");
			configSwitch(CLOCK_EN_PARAM + i, 0.f, 1.f, 0.f, verticalStr + "Clock Enable", {"OFF", "ON"});
			configInput(CLOCK_IN_INPUT + i, verticalStr + "Clock");
			configInput(RESET_INPUT + i, resetPreset + " Trigger");
		}
		configInput(HOLD_INPUT, "Hold Gate");
		configInput(DIRECTION_INPUT, "Direction Change Trigger");
		configOutput(ALLGATES_OUTPUT, "All Gates");
		configOutput(A_OUT_OUTPUT, "A");
		configOutput(B_OUT_OUTPUT, "B");
		configOutput(A_B_OUTPUT, "A-B");
		configOutput(MIN_OUTPUT, "min(A,B)");
		configOutput(MAX_OUTPUT, "max(A,B)");
		configOutput(STAGE_OUTPUT, "Stage");
		configOutput(AB_OUTPUT, "A or B (Vertical Clock)");
		// Prepare state before processing
		changeState(0);
		changeVState();
	}

	void changeState(signed char newStage) {
		// Turn the current LED off and GATE output off
		lights[stage].setBrightness(ledOff);
		outputs[stage].setVoltage(gateOff);
		stage = newStage & 0x07;            // Update the stage (and limit the value to 0-7 range)
		lights[stage].setBrightness(ledOn); // Turn on the new LED
	}

	void changeVState() {
		lights[vStage + 8].setBrightness(ledOff);  // Turn off the current LED
		vStage = (vStage + 1) & 0x01;              // Update VERTICAL STAGE (limit to 0-1)
		lights[vStage + 8].setBrightness(ledOn);   // Turn new LED on
	}

	void process(const ProcessArgs& args) override {
		// Process incoming priority triggers
		signals = sigTriggers.process(
			float_4(
				inputs[DIRECTION_INPUT].getVoltage(),
				params[VCLOCK_EN_PARAM].getValue() * inputs[VCLOCK_IN_INPUT].getVoltage(),
				inputs[RESET_INPUT].getVoltage(),
				0.f
			),
			triggerThresholdLevel,
			triggerThresholdLevel
		);
		if (signals[0]) direction = (direction + 1) & 0x01; // Direction change
		if (signals[1]) changeVState();                     // Vertical stage change

		newStage = 0;                       // Default new stage (in case the change is needed)
		changeInfo = bool(signals[2]) << 3; // Fill the info with reset trigger value
		// If no triggers detected so far...
		if (!changeInfo) {
			// Check whether manual or voltage stage select is active
			for (unsigned char i = 0; i < 8; i++) {
				if (inputs[i].getVoltage() + (10.f * params[i + 16].getValue()) < triggerThresholdLevel) continue;
				preset = i;
				changeInfo |= 0x01;
				break;
			}
			// Or check whether PRESET also has been requested
			if (changeInfo || presetTrig.process(inputs[PRESET_INPUT].getVoltage(), triggerThresholdLevel, triggerThresholdLevel)) {
				newStage = preset;
				changeInfo |= 0x04;
			}
			// Otherwise, check if the CLOCK edge is detected and we are not HOLDing
			else if (
				inputs[HOLD_INPUT].getVoltage() < triggerThresholdLevel &&
				clock.process(params[CLOCK_EN_PARAM].getValue() * inputs[CLOCK_IN_INPUT].getVoltage(), triggerThresholdLevel, triggerThresholdLevel)
			) {
				newStage = stage + ((direction) ? -1 : 1);
				changeInfo |= 0x02;
			}
		}
		// Change sequencer state if any change was requested
		if (changeInfo) changeState(newStage);
		// Turn on the correct GATE output and ALL GATES
		// (if manual or voltage stage select was triggered)
		outputs[stage].setVoltage(gateOn);
		outputs[ALLGATES_OUTPUT].setVoltage(gateOn * (changeInfo & 0x01));
		// Get Row A & B values
		float a = params[stage].getValue();
		float b = params[stage + 8].getValue();
		// Assign correct values to outputs
		outputs[A_OUT_OUTPUT].setVoltage(a);
		outputs[B_OUT_OUTPUT].setVoltage(b);
		outputs[A_B_OUTPUT].setVoltage(a-b);
		outputs[MIN_OUTPUT].setVoltage(std::min(a, b));
		outputs[MAX_OUTPUT].setVoltage(std::max(a, b));
		outputs[STAGE_OUTPUT].setVoltage(stage * stageVoltageFactor);
		outputs[AB_OUTPUT].setVoltage((vStage) ? b : a);
	}
};

struct VoltageSequencerWidget : ModuleWidget {
	VoltageSequencerWidget(VoltageSequencer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VoltageSequencer.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		for (unsigned char i = 0; i < 8 ; i++) {
			float x = xCoords(i);
			if (i < 7) addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x + xOffset, yCoords(0))), module, i + 9));
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(1))), module, i));
			addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(x, 0.5f * (yCoords(1) + yCoords(2)))), module, i));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(2))), module, i));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(3))), module, i));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(4))), module, i + 8));
			addParam(createParamCentered<CKD6>(mm2px(Vec(x, yCoords(5))), module, i + 16));
		}
		for (unsigned char i = 0; i < 2; i++) {
			float x = xCoords(i + 8);
			addChild(createLightCentered<LargeLight<WhiteLight>>(mm2px(Vec(x - xOffset, yCoords(0))), module, i + 8));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(2))), module, i + 8));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(3))), module, i + 10));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(4))), module, i + 12));
			addParam(createParamCentered<NKK>(mm2px(Vec(x, yCoords(5))), module, i + 24));
		}
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xCoords(8), yCoords(1))), module, VoltageSequencer::ALLGATES_OUTPUT));
	}
};
Model* modelVoltageSequencer = createModel<VoltageSequencer, VoltageSequencerWidget>("PatchableDevicesVoltageSequencer");