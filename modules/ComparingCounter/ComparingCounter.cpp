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

using dsp::TSchmittTrigger;
using simd::float_4;

struct ComparingCounter : Module {
	enum ParamId {
		REFERENCE_PARAM,
		COUNTER_LIMIT_PARAM,
		A_POT_PARAM,
		COUNT_CV_ATTV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		COUNT_CV_INPUT,
		B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		COMPARE_OUTPUT,
		END_OUTPUT,
		COUNTER_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	float increment = 1.f / 6.f;        // Whole tone voltage step
	float topMax = increment * 31.f;    // Maximum counter value in Volts
	float cmp;                          // Current comparator value
	float counter;                      // Counter value
	float_4 input;                      // Used for processing all inputs
	TSchmittTrigger<float> trigger;     // Used for updating the counter on compare

	ComparingCounter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(REFERENCE_PARAM, -5.f, 5.f, 0.f, "Threshold", "V");
		configParam(COUNTER_LIMIT_PARAM, 0.f, topMax, 0.f, "Counter Max", "V");
		configParam(A_POT_PARAM, 0.f, 1.f, 0.f, "Signal A Attenuator");
		configParam(COUNT_CV_ATTV_PARAM, -1.f, 1.f, 0.f, "Counter Max CV Attenuverter");
		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configInput(COUNT_CV_INPUT, "Counter Max CV");
		configOutput(COMPARE_OUTPUT, "Compare Gate");
		configOutput(COUNTER_OUTPUT, "Counter Value");
		configOutput(END_OUTPUT, "End Gate");
	}

	void process(const ProcessArgs& args) override {
		// y = (x * a) + b
		input = {inputs[A_INPUT].getVoltage(), inputs[B_INPUT].getVoltage(), inputs[COUNT_CV_INPUT].getVoltage(), 0.f};
		input *= {params[A_POT_PARAM].getValue(), 1.f, params[COUNT_CV_ATTV_PARAM].getValue(), 0.f};
		input += {0.f, params[REFERENCE_PARAM].getValue(), params[COUNTER_LIMIT_PARAM].getValue(), 0.f};
		// CMP = (k*A > B + THRESHOLD)
		cmp = gateOn * (input[0] > input[1]);
		// Update the counter
		counter += increment * trigger.process(cmp, triggerThresholdLevel, triggerThresholdLevel);
		// Reset counter if reached the limit
		if (counter >= clamp(input[2], 0.f, topMax)) counter = 0.f;
		// Output values
		outputs[COMPARE_OUTPUT].setVoltage(cmp);
		outputs[COUNTER_OUTPUT].setVoltage(counter);
		// END is only high when counter is 0 and CMP is high
		outputs[END_OUTPUT].setVoltage(gateOn * (trigger.isHigh() && !counter));
	}
};

struct ComparingCounterWidget : ModuleWidget {
	ComparingCounterWidget(ComparingCounter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "modules/ComparingCounter/ComparingCounter.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		for (unsigned char i = 0; i < 2; i++) {
			float x = xCoords(i);
			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(1))), module, i));
			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(3))), module, i));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(4))), module, i + 2));
			addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, yCoords(5))), module, i));
		}
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xCoords(0), yCoords(2))), module, ComparingCounter::B_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xCoords(1), yCoords(0))), module, ComparingCounter::COUNTER_OUTPUT));
	}
};
Model* modelComparingCounter = createModel<ComparingCounter, ComparingCounterWidget>("PatchableDevicesComparingCounter");