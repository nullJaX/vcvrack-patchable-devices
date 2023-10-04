# Comparing Counter
***Summary:** Pulse counter / frequency divider with advanced comparator input*

***Width:** 8HP*
## Description
Comparing Counter consists of two (connected to each other) building blocks: a comparator and a pulse counter.
### Comparator
The input comparator subtracts two input signals (`A` and `B`; signal `A` can be attenuated) and compares the difference with the threshold voltage between -5V and 5V (set by a potentiometer labeled as `T`). If the differential voltage is higher than the threshold voltage, the comparator reaches HIGH state on its output (labeled as `A - B > T`) equal to 5V, otherwise the voltage is 0V.
### Pulse Counter
The pulse counter input is internally connected to the comparator output and detects state changes. Whenever the comparator output changes from 0V to 5V, it is identified as a pulse and the counter value is being updated (incremented) by 0.166V (whole tone steps when patched as a pitch signal). The current counter value is available at the output labeled as `VALUE`.

The pulse counter is equipped with the Counter Max (or Counter Limit) parameter that can be set by the potentiometer (labeled with `1` and `31` identifying the number of counter steps) and can be dynamically controlled by an external control voltage (`MAX CV` input and connected attenuverter). Whenever a current counter voltage reaches value higher than the Counter Max voltage (sum of potentiometer voltage and attenuverted `MAX CV` signal) the counter is reset to 0V. When the counter overflows (in other words - resets its value to 0) and the counter input reads HIGH state (5V) at the same time, the `END` output generates 5V, otherwise 0V.
## Connectivity
### Inputs
| Label | Description |
| --- | --- |
| A | First comparator input. Can be attenuated. |
| B | Second comparator input. |
| MAX CV | Control Voltage Input for Counter Limit. Can be attenuverted. |
### Outputs
| Label | Voltage range | Description |
| --- | --- | --- |
| A - B > T | 0V or 5V | Comparator output. Reaches high state (5V) when the incoming signals' difference is higher than the specified threshold. |
| VALUE | 0V - 5.17V | Current counter value. One counter step correspond to 0.166V (whole tone steps when patched as a pitch signal). |
| END | 0V or 5V | Counter END gate. Reaches high state (5V) when the counter overflows (reaches zero again) and the Comparator output (A - B > T) is also in high state. |
## Patching tips
- For wave shaping capabilities, try adjusting either signal `A` attenuator (if used) and/or threshold `T` parameter. This will result in outputs `A - B > T` and `END` producing pulse wave signals with variable pulse width.
- For any frequency division capabilities, try adjusting Counter Max parameter to choose N-th division (subharmonic) of the waveform produced by the comparator. Note that this will also affect the height of the staircase-shaped, saw wave available at `VALUE` output.
- Do not underestimate the comparator input and use it to implement various logic behavior.
- You can use `VALUE` output as pitch signal for your Oscillator and plug in external clock to create major scale (whole tone) arpeggios.
- You can create an Oscillator by self-patching `A - B > T` into input `B` and tuning the threshold `T` to negative. This way, the comparator will negate its state every sample forever (creating super fast square wave). Although the `A - B > T` output will produce fixed-frequency square wave, you can still use the Counter/Division part to obtain:
  - at `END` output - subharmonic of the oscillator controlled with Counter Max parameter
  - at `VALUE` output - saw-like wave that is louder for greater subdivisions
- If 32 subdivisions is not enough for you, you can always cascade it with another Comparing Counter.
- You can use 2 or more Comparing Counters with the same clock source as input to generate interesting polyrhythms or harmonies (depending on the clock speeds). Try to also patch different Comparing Counters between each other or automate the division changes for more creative results.