# Dual Integrator
***Summary:** Dual, multi-purpose slew limiter*

***Width:** 16HP*
## Description
Dual Integrator consists of two identical universal slew limiter cells. Each cell slows down the change rate of the applied signal on IN input and generates a slewed signal on OUT output. The slew rate is adjustable and controllable by external voltage exponentially, to comply with `1V / octave` standard.

Additionally, one can temporarily interrupt signal processing by applying positive gate signal (above `1.8V`) to the GATE input. When the gate is applied, the slew cell will see 0V on its input and will try to slew to that value.

Each cell is equipped with a mode trigger/gate input, its function is dependent on the mode toggle switch. When the cell is in `S&H` (Sample and Hold) mode, the cell will hold its value indefinitely regardless of the input signal (*slew rate = 0 Volts / second*) and slew one sample of the signal according to the settings when a rising edge of the trigger is detected. When the cell is in `T&H` (Track and Hold) mode, it will only hold its value as long as the gate signal is applied.

Moreover, each cell contains an inverting comparator with hysteresis (END output) that produces the voltage opposite to the current polarity of the signal. The END output generates -5V whenever the slewed signal reaches 5V or above. When the slewed signal reaches a threshold of -5V or below, the comparator will generate 5V.

The last building block of this module is comparator. Both slew limiters' signals are compared and the COMP output generates 5V whenever left cell has currently higher voltage then the right one. Otherwise, the comparator generates -5V.
## Connectivity
### Inputs
| Label | Description |
| --- | --- |
| IN | Slew limiter's input. |
| GATE | Mutes the incoming signal. When a positive gate (above `1.8V`) is applied, corresponding slew limiter cell will assume 0V on its input. |
| CV | Exponential (`1V / octave`) frequency/rate control for the slew limiter. The higher the voltage, the higher the frequency/rate. |
| `S&H` / `T&H` | This input is either a gate or a trigger, depending on the toggle switch setting below. When in `S&H` (Sample and Hold) mode, the slew limiter will only slew the signal on the positive trigger pulse (above `1.8V`) and hold the current voltage otherwise (*slew rate = 0 Volts / second*). When in `T&H` (Track and Hold) mode, the slew limiter will hold the current voltage only when a positive gate (above `1.8V`) is applied. |
### Outputs
| Label | Voltage range | Description |
| --- | --- | --- |
| OUT | *See description* | Slew limiter output. Follows the same voltage range as the corresponding input. The LED linked with a line indicates the current polarity and voltage of the generated signal. |
| END | -5V or 5V | Slew limiter's inverting comparator with hysteresis. When the slew limiter's voltage reaches 5V (and above), this output will produce -5V. When the slew limiter's voltage reaches -5V (and below), this output will produce 5V. |
| COMP | -5V or 5V | Comparator output. The internal comparator compares two slew limiters' voltages and produces positive signal (5V) when the limiter on the left has higher voltage than the one on the right, otherwise it generates negative signal (-5V). |
## Patching tips
### Cycling (Oscillator/Low Frequency Oscillator/Clock generator)
1. Set slew cell to `T&H` (Track and Hold) mode
2. Patch `END` output back to the `IN` input of the same slew cell
3. Control the frequency (both in audio and sub-audio range) via `CV` inputs and potentiometers.
4. This patch will generate (without additional patching, see next tips for wave shaping capabilities):
   1. Triangle wave on `OUT` output
   2. Square wave on `END` output
5. Additionally you can:
   - add [wave shaping tricks](#wave-shaping)
   - mute/freeze the signal using either `GATE` (mute to 0V) or `T&H` (freeze on current voltage) inputs
### Wave Shaping
**Note: Since this patch uses self frequency modulation technique, you will need to readjust the rate of the cell**
1. Patch either `END` or `OUT` output back to the `CV` input (ideally to the one with attenuverter for more control over the wave shape).
2. Patching `END` output will enable you to change:
   1. the shape of the `OUT` signal between sawtooth (faster fall time), triangle (equal times) and reverse sawtooth (faster rise time)
   2. the pulse width of the `END` signal
3. Patching `OUT` output will enable you to change the nonlinearity/curvature of the `OUT` signal (logarithmic - linear - exponential)
### Filtering (Slew limiter/Filter/Low-Pass Gate/Voltage Controlled Amplifier)
Since the slew limiter slows down the change rate of the signal, it behaves almost the same as low pass filter:
1. Make sure the slew cell is in `T&H` (Track and Hold) mode
2. Plug any voltage source (even in audio rate) to `IN` input and the result signal on `OUT` will be filtered
3. When the slew rate is significantly higher than the signal change rate, it will additionally start to attenuate it
4. You can automate this process by using `CV` inputs or gate the signal using `GATE` (mute to 0V) or `T&H` (freeze on current voltage) inputs.
### Envelope or Function Generator
This method is the same as the [filtering patch](#filtering-slew-limiterfilterlow-pass-gatevoltage-controlled-amplifier), except now you can provide sub-audio, trigger, gate or even constant signals on `IN` input. In addition, you can:
- utilize `T&H` gate input for holding the voltage
- utilize `GATE` input for interrupting the generator
- include [wave shaping tricks](#wave-shaping)
### Sample and Hold (Random Voltage Generator/Sample Rate Reducer/Bitcrusher/Staircase Oscillator)
Make sure the slew cell is in `S&H` (Sample and Hold) mode and adjust the slew rate to your taste (you can also combine it with other patching ideas mentioned above). The table below shows what can be achieved in this mode.
| Function | Patch details |
|---|---|
| S&H | Patch any signal to the `IN` input. Use any low frequency oscillator or arbitrary gate/trigger source for `S&H` input. |
| Random Voltage Generator | The same as S&H function, but use noise generator of your choice as the input signal. |
| Bitcrusher/Sample Rate Reducer | Patch any audio signal to the `IN` input. Apply a steady (probably also audio rate) clock to the `S&H` input. The rate of the clock source will determine the sample rate, while the number of voltage states will be determined by the slew rate parameter. |
| Staircase Oscillator | Patch `END` output back to the `IN` input. Apply a steady clock to the `S&H` input (you can even use the other cell in cycling mode). The waveforms on both `OUT` (staircase) and `END` (square wave) outputs will be a subdivision of the clock source. The frequency division can be managed by the cell slew rate parameter, while the base frequency can be adjusted on the clock source. |
### Comparator output
Don't underestimate the potential of `COMP` output, ever.

At a first glance, it is just a simple comparator between two slew cells, but when at least one cell oscillates in audio frequency range, the resulting waveform will sound like pulse width modulated square wave. The difference between cells' frequencies will determine the modulation speed.

You can also patch signal (either `OUT` or `END` output) from one of the cells to the second cell's `GATE` or `T&H` input to obtain square wave audio-rate wave shaping (hard sync like behavior).