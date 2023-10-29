# Nonlinear Integrator
***Summary:** State Variable Filter*

***Width:** 12HP*
## Description
Nonlinear Integrator is a general purpose filter with 4 different filtering outputs and one signal input that can accept both audio and sub-audio (control) voltage. The filter's frequency and resonance parameters can be controlled via external voltage (that can be pre-scaled by attenuverter potentiometers).

**WARNING: The resonance of this filter can be really high and, if not managed correctly, can cause output signal to be really loud (24 Volts peak-to-peak). It is recommended to gradually apply resonance with caution.**

The filter also contains a trigger input, that can be utilized for filter pinging technique. When a rising voltage is detected (above `1.8V`), the internal circuit generates a short pulse that is processed by the filter. That, in result, can generate various sounds that are similar to striking resonant objects. While the filter's frequency determine the base frequency of the sound, the resonance controls the damping effect of the resulting signal. Using filter pinging technique, one can generate a broad spectrum of sounds from clicks, through wood blocks, to bell tones.
## Connectivity
### Inputs
| Label | Description |
| --- | --- |
| IN | Filter input (for both Audio and CV signals) |
| PING | Trigger input for generating percussive sounds (filter pinging technique). Detects rising edges (above `1.8V`) |
| FREQ CV / EXP FM | Voltage control over the filter's frequency (both exponential to comply with `1V / octave` standard) |
| RES CV | Voltage control over filter's resonance (or Q) |
### Outputs
| Label | Voltage range | Description |
| --- | --- | --- |
| LOW | -12V - 12V | Low-pass filter output |
| BAND | -12V - 12V | Band-pass filter output |
| HIGH | -12V - 12V | High-pass filter output |
| NOTCH | -12V - 12V | Notch reject filter output |
## Patching tips
### Cycling (Quadrature Oscillator/Low Frequency Oscillator)
To cycle the filter, connect a `BAND`-pass output back to the filter's `IN` input and tweak both input gain and resonance until one of the outputs starts to produce a steady sine wave.

When cycling, the filter behaves like a quadrature oscillator. It means that all outputs of this filter will produce sine waves that are shifted in phase in relation to each other - exactly 90 degrees apart.

The filter's frequency inputs can be patched with external voltage to control the frequency of the sine wave. One can also use attenuvertable frequency input to introduce self frequency modulation by patching one of the outputs (for some interesting wave shaping capabilities).
### Envelope / Function Generator
Since this filter can also be adjusted to filter sub-audio signals, one can apply gates, triggers or other slowly changing voltages to the input and process the signal further, creating more complex envelopes or functions (depending on the output).

The resulting signal can be additionally modified by adding sine fluctuations resulting from increased resonance setting. This will add some interesting vibrato effect to the signal.
### Chaos Generator
While investigating state variable filters, one can notice that the architecture of the filter's circuit somewhat resembles the chaos generator circuit layout (for instance [here](https://ijfritz.byethost4.com/Chaos/ch_cir1.htm)). Such generator requires two integrators and a feedback path that introduces some level of nonlinearity.

The Nonlinear Integrator module (with any additional signal source of choice) can also be used for this purpose. Increase the resonance so that the sine wave fluctuations (nonlinearity source) appear in the output signal and tweak the filter's frequency to your taste. When patching two adjacent (ie. `LOW` and `BAND`) outputs to the scope in XY mode (one signal is displayed across X axis, the other across Y axis), one can see that the resulting picture is similar to the chaotic system with two attractors (for more information, please refer to the [Chaos Theory](https://en.wikipedia.org/wiki/Chaos_theory)). You can use the output signals as both audio or control voltage.