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
using dsp::TSlewLimiter;
using simd::float_4;

// A helper struct based on TSchmittTrigger, but
// it accepts two floats and return a bitmask'ed results
struct BitMaskSchmittTrigger {
	unsigned char state;
	unsigned char _mask = 0x03;
	BitMaskSchmittTrigger() { reset(); }
	void reset() { state = 0xff; }
	unsigned char isHigh() { return state; }
	unsigned char process(float in1, float in2, float lowThreshold = 0.f, float highThreshold = 1.f) {
		unsigned char on = (in1 >= highThreshold) | ((in2 >= highThreshold) << 1);
		unsigned char off = (in1 <= highThreshold) | ((in2 <= highThreshold) << 1);
		unsigned char triggered = (~state & on) & _mask;
		state = (on | (state & ~off)) & _mask;
		return triggered;
	}
};

struct DualIntegrator : Module {
	enum ParamId {
		ENUMS(SH_PARAM, 2),
		ENUMS(CV1_ATTV_PARAM, 2),
		ENUMS(RATE_PARAM, 2),
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(IN_INPUT, 2),
		ENUMS(GATE_INPUT, 2),
		ENUMS(INF_INPUT, 2),
		ENUMS(CV1_INPUT, 2),
		ENUMS(CV2_INPUT, 2),
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(SLEW_OUTPUT, 2),
		ENUMS(END_OUTPUT, 2),
		CMP_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(OUT_LED_LIGHT, 4),
		ENUMS(SH_LED_LIGHT, 2),
		LIGHTS_LEN
	};

	DualIntegrator() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		std::string cvStr = "CV";
		for (unsigned char i = 0; i < 2; i++) {
			configSwitch(SH_PARAM + i, 0.f, 1.f, 0.f, "Mode", {"Track & Hold", "Sample & Hold"});
			configParam(CV1_ATTV_PARAM + i, -1.f, 1.f, 0.f, "CV Attenuverter");
			configParam(RATE_PARAM + i, -5.f, 13.5f, 4.25f, "Rate", "Hz", 2.f);
			configInput(IN_INPUT + i, "Signal");
			configInput(GATE_INPUT + i, "Gate");
			configInput(INF_INPUT + i, "Sample/Track and Hold");
			configInput(CV1_INPUT + i, cvStr);
			configInput(CV2_INPUT + i, cvStr);
			configOutput(SLEW_OUTPUT + i, "Lag");
			configOutput(END_OUTPUT + i, "End");
		}
		configOutput(CMP_OUTPUT, "Comparator (L>R)");
	}

	float_4 gates, input, cv, output;               // Used for storing values, the names are straightforward
	unsigned char shToggle, shTrigger, infSlew;     // Bitmasks for storing S&H/T&H information
	BitMaskSchmittTrigger sh;                       // Used for bitmasks (above) calculation
	TSlewLimiter<float_4> slew;                     // Main cells, the core of the slew routine
	float endLow = -5.f, endHigh = 5.f;             // Threshold values for END Schmitt Trigger
	TSchmittTrigger<float_4> end;                   // END Schmitt Trigger

	void process(const ProcessArgs& args) override {
		// Gather S&H/T&H informations
		shToggle = (bool(params[0].getValue())) | (bool(params[1].getValue()) << 1);
		shTrigger = sh.process(inputs[4].getVoltage(), inputs[5].getVoltage(), triggerThresholdLevel, triggerThresholdLevel);
		// Calculate incoming CVs: y = (x * A) + B + C
		cv = float_4(inputs[6].getVoltage(), inputs[7].getVoltage(), 0.f, 0.f);
		cv *= float_4(params[2].getValue(), params[3].getValue(), 0.f, 0.f);
		cv += float_4(inputs[8].getVoltage(), inputs[9].getVoltage(), 0.f, 0.f);
		cv += float_4(params[4].getValue(), params[5].getValue(), 0.f, 0.f);
		// Convert to Hertz
		cv = pow(2.f, cv);
		// Determine whether infinite slew should be applied (a.k.a holding a value)
		infSlew = (shToggle & shTrigger) | ~(shToggle | sh.isHigh());
		// Multiply by 0 if holding a value, otherwise multiply by 20
		// Value 20 is chosen to match the frequency parameters: 2 * VoltagePeakToPeak
		cv *= float_4(20.f * (infSlew & 0x01), 20.f * ((infSlew >> 1) & 0x01), 0.f, 0.f);
		// Check whether gate inputs are active
		gates = float_4(inputs[2].getVoltage(), inputs[3].getVoltage(), 0.f, 0.f);
		// If gate inputs are active, assign 0 volts on input instead of the values
		input = ifelse(
			gates < triggerThresholdLevel,
			clamp(
				float_4(inputs[0].getVoltage(), inputs[1].getVoltage(), 0.f, 0.f),
				vMin,
				vMax
			),
			float_4::zero()
		);
		// Update slew rate
		slew.setRiseFall(cv, cv);
		// Perform slew
		output = slew.process(args.sampleTime, input);
		// Update END Schmitt Trigger
		end.process(output, endLow, endHigh);
		// Output the comparison between two slewing cells
		outputs[CMP_OUTPUT].setVoltage((output[0] > output[1] ? gateOn: -gateOn));
		for (unsigned char i = 0; i < 2; i++) {
			unsigned char twoI = (i << 1);
			// Update OUT and END
			outputs[SLEW_OUTPUT + i].setVoltage(output[i]);
			outputs[END_OUTPUT + i].setVoltage((end.isHigh()[i] ? -gateOn : gateOn));
			// Update LEDs
			lights[OUT_LED_LIGHT + twoI].setBrightness(std::max(0.f, .2f * output[i]));
			lights[OUT_LED_LIGHT + 1 + twoI].setBrightness(std::max(0.f, -.2f * output[i]));
			lights[SH_LED_LIGHT + i].setBrightness(((shToggle ^ sh.isHigh()) >> i) & 0x01);
		}
	}
};

struct DualIntegratorWidget : ModuleWidget {
	DualIntegratorWidget(DualIntegrator* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "modules/DualIntegrator/DualIntegrator.svg")));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		float ys[5] = {yCoords(0), yCoords(2), yCoords(3), yCoords(4), yCoords(5)};
		for (unsigned char i = 0; i < 4; i++) {
			float x = xCoords(i);
			unsigned char halfI = (i >> 1);
			if (i == 0 || i == 3) {
				addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, ys[0])), module, DualIntegrator::SLEW_OUTPUT + halfI));
				addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(x, yCoords(1))), module, DualIntegrator::END_OUTPUT + halfI));
				addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, ys[1])), module, DualIntegrator::IN_INPUT + halfI));
				addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, ys[2])), module, DualIntegrator::CV1_INPUT + halfI));
				addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, ys[3])), module, DualIntegrator::CV1_ATTV_PARAM + halfI));
				addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(x, ys[4])), module, DualIntegrator::RATE_PARAM + halfI));
			} else {
				addChild(createLightCentered<LargeLight<GreenRedLight>>(mm2px(Vec(x, ys[0])), module, DualIntegrator::OUT_LED_LIGHT + (halfI << 1)));
				addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, ys[1])), module, DualIntegrator::GATE_INPUT + halfI));
				addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, ys[2])), module, DualIntegrator::CV2_INPUT + halfI));
				addInput(createInputCentered<PJ301MPort>(mm2px(Vec(x, ys[3])), module, DualIntegrator::INF_INPUT + halfI));
				addChild(createLightCentered<MediumLight<YellowLight>>(mm2px(Vec(x, 0.5f * (ys[3] + ys[4]))), module, DualIntegrator::SH_LED_LIGHT + halfI));
				addParam(createParamCentered<NKK>(mm2px(Vec(x, ys[4])), module, DualIntegrator::SH_PARAM + halfI));
			}
		}
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(0.5f * (xCoords(1) + xCoords(2)), yCoords(1))), module, DualIntegrator::CMP_OUTPUT));
	}
};
Model* modelDualIntegrator = createModel<DualIntegrator, DualIntegratorWidget>("PatchableDevicesDualIntegrator");