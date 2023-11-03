# Voltage Sequencer
***Summary:** Programmable sequencer*

***Width:** 40HP*
## Description
Voltage Sequencer is an 8-stage step sequencer providing two basic, simultaneous voltage sequences and several control voltage outputs derived from different combinations of these basic sequences. Each stage of the sequencer is equipped with:
- two potentiometers (each for corresponding sequence)
- activation LED indicating active stage
- gate output that produces a gate signal whenever the stage is active
- stage select gate input and button (that immediately instructs sequencer to jump to particular stage)

Apart from the clock input (that advances the state), the sequencer provides 4 inputs to change or interrupt its operation:
- `DIRECTION` trigger input for controlling the direction of the sequencer steps
- `RESET`/`PRESET` (in conjunction with `Stage Select Gate Inputs and Buttons`) trigger inputs for jumping unconditionally to the desired stage
- `HOLD` gate input for pausing the operation of the sequencer

To add extra layer of sequencing, a vertical clock (labeled `V.CLOCK`) and `A or B` output were added. The user can use this feature to create a 16-step sequence (see [Patching Tips section](#patching-tips) for more information).

The sequencer can also be used as a simple controller/keyboard with 8 keys. While using `Stage Select Gates` or `Stage Select Buttons`, the output called `STAGE SELECTED` will hold high value (5V) as long as the stage is selected. Additionally, the output called `STAGE` generates a signal that can be used as a pitch (`1V / octave` standard) in whole tone steps (major scale). The user can also use regular sequence outputs to define own control voltage per step.
## Connectivity
### Inputs (with priority for inputs on simultaneous stage selection)
| Label | Priority | Description |
| --- | --- | --- |
| RESET | 1 | Highest priority input for stage selection. On a rising edge of the trigger (with threshold `1.8V`), the sequencer will unconditionally jump to stage 1. |
| Stage Select Gate Inputs and Buttons | 2 | While pushing one of the buttons or applying a signal above `1.8V` to one of the inputs, the sequencer will hold specific chosen stage regardless of other inputs mentioned below (including `CLOCK` input). Using these inputs and buttons will also update the preset stage that can be recalled by `PRESET` input (stage 1 after initialization by default). In a race condition situation (two or more inputs received signal), the leftmost stage is chosen (the lower the stage number, the higher priority it has). See other inputs above for more information when these inputs are overridden |
| PRESET | 2 | A rising edge of the trigger (with threshold `1.8V`) will recall and jump to last saved preset stage, updated by `Stage Select Gate Inputs and Buttons`. After initialization, the default preset stage is set to 1. See other inputs above for more information when this input is overridden |
| HOLD | 3 | When a gate signal (above `1.8V`) is applied, the sequencer will hold the current stage regardless of the `CLOCK` trigger input. See other inputs above for more information when this input is overridden |
| CLOCK | 4 | Trigger input (with threshold voltage `1.8V` on rising edge) that advances the stage of the sequencer. Can be disabled by attached toggle switch below. See other inputs above for more information when this input is overridden |
| DIRECTION | *N/A* | Changes sequencer direction (left or right) on a positive trigger (threshold `1.8V`). |
| V.CLOCK | *N/A* | Vertical Clock Trigger input (with threshold voltage `1.8V` on rising edge). On trigger, it will change from which row (`A` or `B`) the voltage will appear on the 'A or B' output (indicated by the LEDs). Can be disabled by attached toggle switch below |
### Outputs
| Label | Voltage range | Description |
| --- | --- | --- |
| A | 0V to 5V | Current stage voltage obtained from potentiometer row `A` |
| B | 0V to 5V | Current stage voltage obtained from potentiometer row `B` |
| A-B | -5V to 5V | Voltage difference between row `A` and `B` for current stage |
| MIN | 0V to 5V | Minimum voltage between row `A` and `B` for current stage  |
| MAX | 0V to 5V | Maximum voltage between row `A` and `B` for current stage |
| STAGE | 0V to 1.34V | Indicates the current stage of the sequencer. The voltage will increment in whole tone steps (major scale), thus this signal can be used as pitch/frequency control voltage |
| `A` or `B` output | 0V to 5V | Outputs voltage either from row `A` or row `B` (for given stage), depending on the vertical clock (indicated by two connected LEDs and controlled via `V.CLOCK` trigger input) |
| STAGE SELECTED | 0V or 5V | Generates 5V gate whenever any stage is selected (either via `Stage Select Gate Inputs` or by pushing the `Stage Select Buttons`) |
| Stage Gate Outputs | 0V or 5V | Placed above `Stage Select Gate Inputs`. Generate high state (5V) for the current stage. |
## Patching tips
### Creating shorter sequences (static)
1. Plug in a clock source to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Insert a cable between `RESET` trigger input and one of the stage gate outputs (**different from stage 1**, ie. 5th stage). This stage will serve as a sequence switch point
3. The sequence is immediately returning to the stage 1 after it reaches switch point.
4. One can use `Stage Select Buttons` or `Stage Select Gate Inputs` of other (unused) stages to create sequence mixins. After the sequencer returns back to the stage 1, it will continue looping the short sequence.

**Alternate version for point 2:** You can insert a cable between any `Stage Gate Output` and any `Stage Select Gate Input` to achieve the same result with different start and end points for the sequence.
### Creating shorter sequences (dynamic)
1. Plug in a clock source to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Insert a cable between `PRESET` trigger input and one of the stage gate outputs (ie. 5th stage). This stage will serve as a sequence switch point
3. The sequencer will advance to the stage with the switch point and jump immediately to the preset stage
4. The start of the sequence (preset stage) can be set by pushing one of the `Stage Select Buttons` or can be automated using `Stage Select Gate Inputs`
### Creating longer sequences (1 to 16 steps)
1. Plug in a clock source to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Make sure that the toggle switch under `V.CLOCK` trigger input is in upright position (ON)
3. Insert a cable between `V.CLOCK` trigger input and one of the stage gate outputs (ie. 8th stage). This stage will serve as a sequence switch point
4. The output at the top (with two LEDs connected) will generate a voltage sequence that is the combination of sequences A and B. The active LED indicates which row is used currently to output the voltage for a particular stage.
5. One can also use any `Stage Gate Output` and patch it to any `Stage Select Gate Input` or `RESET`/`PRESET` trigger input to obtain any even-step sequences between 2 and 14 (and 1 step sequence)
### Pendulum sequence
1. Plug in a clock source to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Insert a cable between `DIRECTION` trigger input and one of the stage gate outputs (ie. 5th stage). This stage will serve as a sequence switch point
3. When the sequence reaches the switch point, on the next clock pulse the sequence will start advancing in another direction. The sequencer will advance in that direction until the switch point is reached again
### Swing sequence
In any of the patches presented here, one can also sacrifice one of the control voltage outputs and send it to the clock source as a frequency modulation signal. This will completely destabilize the timing in relation to other rhythmic parts, but with careful fine tuning it can lead to very interesting musical results and create an illusion of a sequence being 'alive'.
### Clocking at audio rate (Wave Shaper/Frequency Divider)
1. Plug in any audio rate signal to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Each row of potentiometers allows for creating various stepped signal shapes available at the `A` and `B` related outputs
3. By default, the resulting signal is 8 times slower than the signal source, but this can be adjusted using one of the sequence length changing techniques mentioned above. Try to experiment especially with the `PRESET` trigger input
### One-shot sequence (Stepped Envelope/Function Generator)
1. Plug in a clock source to `CLOCK` trigger input and make sure the toggle switch below the input is in upright position (ON)
2. Select which stage should serve as the end point and connect its gate output to the `HOLD` gate input
3. To run a sequence, use `Stage Select Buttons` or `Stage Select Gate Inputs` to choose the start point of a sequence.
4. The sequencer will start from the chosen stage and advance with clock pulses until it stops at the end stage.
5. One can also sacrifice one potentiometer row to specify the time per step using [Swing sequence patch](#swing-sequence)