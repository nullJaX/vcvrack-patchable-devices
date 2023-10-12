# Digital Chaotic System
***Summary:** Digital 8-bit Shift Register driven by two independent VCOs*

***Width:** 16HP*
## Description
Digital Chaotic System consists of three sections: Clock Oscillator, Data Oscillator and 8-bit Shift Register.
### Clock and Data Oscillators
The oscillators on this module are identical and both have the same inputs, parameters and outputs. Each oscillator can produce a triangle (-5V to 5V) and a square (-5V and 5V) wave. All oscillator inputs affect the frequency exponentially to comply with `1V / octave` standard.
### 8-bit Shift Register
This building block is a core functionality of this module and has two digital inputs:
- Clock Trigger (Normalized to Clock `SQR` output)
- Data Gate (Normalized to Data `SQR` output)

For both inputs, a high state is read when an incoming signal exceeds `1.8V`. On each Clock pulse (state changes from LOW to HIGH), a Data Gate state is read and combined with the oldest state of the shift register (8th bit) using XOR operation. In short, XOR operation results in a high state when only one of the incoming signals is in a high state (see the table below):
| A | B | XOR(A, B) |
|---|---|---|
| LOW | LOW | LOW |
| HIGH | HIGH | LOW |
| HIGH | LOW | HIGH |
| LOW | HIGH | HIGH |

The result of XOR operation is then placed in the shift register (1st bit), shifting all previous states by one place. This configuration is an example of really primitive Linear Feedback Shift Register (LFSR), a simple digital implementation of a chaotic system (Chaos Theory). There are 3 outputs from the Shift Register that are available on the panel.

The `PULSED` output is the current result of the XOR operation mentioned above.

The `STEPPED` output is generated from the oldest 3 bits of the shift register (8 possible states) using digital-to-analog converter formula (6th bit is the most significant bit):
```
STEPPED = k * (
  4*register(6) +
  2*register(7) +
  1*register(8)
)

where:
  register(N) - Nth bit digital value read from the shift register (0 or 1)
  k = 5/8 = 0.625
```
To obtain the `SMOOTH` output, the `STEPPED` voltage is fed through a first order low pass filter (-3dB per octave) with a 20Hz cutoff frequency.
## Connectivity
### Inputs
| Label | Description |
| --- | --- |
| CLOCK | Shift Register Clock Trigger input. Normalized to Clock `SQR` output. |
| DATA | Shift Register Data Gate input. Normalized to Data `SQR` output. |
| EXP CV (and all white ring inputs) | Oscillators' frequency control (all with exponential characteristics to track `1V / octave` standard). |
### Outputs
| Label | Voltage range | Description |
| --- | --- | --- |
| TRI | -5V - 5V | Triangle wave output of the Clock or Data oscillator. |
| SQR | -5V or 5V | Square wave output of the Clock or Data oscillator. These outputs are internally connected to the shift register. |
| PULSED | 0V or 5V | Result of the XOR operation between data oscillator and the last bit of the shift register. |
| STEPPED | 0V - 4.38V | Analog representation of a binary number obtained from last 3 bits of the shift register (8 possible values). |
| SMOOTH | 0V - 4.38V | Smoothed version of the `STEPPED` output (first order low pass filter with 20Hz cutoff frequency). |
## Patching tips
### Self-oscillator patching
By patching `SQR` output back to adjustable frequency input (ie. attenuverted input), you will be able to:
- control the wave shape on the `TRI` output (from sawtooth, through triangle to reverse sawtooth)
- control the pulse width of the `SQR` output

By patching `TRI` output back to adjustable frequency input (ie. attenuverted input), you will be able to control the curvature/nonlinearity of the `TRI` output.

**Note 1: These changes will also affect the pitch of the oscillator, since these patches use self-frequency modulation.**

**Note 2: Try to experiment with these patches also when you are using the shift register.**
### Cross-oscillator patching (Cross Frequency Modulation)
Although this technique probably will not produce any musical results, you can try patching output(s) of one oscillator to adjustable input(s) of the other and control the amount of influence to achieve weird beats or timbres.
### Shift register tricks
- Keep in mind that the `PULSED` output is the result of the XOR operation between DATA gate and eighth (last) bit of the register, thus the frequency of the signal will usually be one eighth of the CLOCK or DATA input.
- When DATA oscillator has higher frequency than CLOCK oscillator:
  - `PULSED` output follows DATA oscillator with eventual interruptions resulting from the CLOCK pulses shifting bits that are XOR'ed with the DATA signal (irregular hard-sync like sound)
  - `STEPPED` and `SMOOTH` outputs produce changing patterns/timbres that eventually loop (the chaotic system behaves as if it was stuck in a steady state, oscillating between its attractors). The sound is not a random noise, rather, it contains a distinguishable frequency component derived from both CLOCK and DATA signals.
- When CLOCK oscillator has higher frequency than DATA oscillator:
  - `PULSED` output produces a square wave that is an 8th division of the CLOCK signal. The DATA signal (slower) will modify the pulse width of the generated waveform.
  - `STEPPED` and `SMOOTH` outputs will produce different repeatable waveforms (depending on the register contents) with frequency determined by the CLOCK input. The DATA signal determines the frequency of the waveform changes (wave shaping).
- You can simulate a digital noise generator by setting DATA oscillator to really high frequencies and changing the noise pitch with CLOCK oscillator. The noise sound will be available on the `STEPPED` output, while `SMOOTH` output will generate very quiet sound that is somewhat similar to the pink noise.
- Because the shift register contains a loop with XOR operation, you can leverage that (rows 1 & 4 presented in the table) and create constant 8 step pattern by plugging constant 0V signal into `DATA` gate input.