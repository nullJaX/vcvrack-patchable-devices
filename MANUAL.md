# User Manual
1. Modules (External files)
   1. [Comparing Counter](modules/ComparingCounter/ComparingCounter.md)
   2. [Digital Chaotic System](modules/DigitalChaoticSystem/DigitalChaoticSystem.md)
   3. [Dual Integrator](modules/DualIntegrator/DualIntegrator.md)
   4. [Nonlinear Integrator](modules/NonlinearIntegrator/NonlinearIntegrator.md)
   5. [Voltage Sequencer](modules/VoltageSequencer/VoltageSequencer.md)
   6. [Window Generators](modules/WindowGenerators/WindowGenerators.md)
2. [Panel standard](#panel-standard)
3. [Voltage standard](#voltage-standard)
## Panel standard
In order to improve user experience while using Patchable Devices plugin and achieve uniform look between the modules, several rules were established for the general panel design:
1. Components are (mostly) laid out in a grid (aligned to module center) with:
   - **6 rows** spaced evenly by **20 millimeters**
   - **4HP-wide (20.32mm) columns**
   - horizontal margin to module's edge equal to **10.16mm** (2HP for seamless aesthetics)
   - top margin reserved for module's name
2. **Output components** are positioned over **dark background** to easily navigate when patching
3. All components of the same type (potentiometers/buttons/switches, except LEDs) share **the same size and appearance**
4. All jacks contain **colored ring** underneath to indicate the **type of signal** to send or receive:
   - white - continuous voltage (analog)
   - red - digital (discrete voltage with two available states, usually 0V and 5V for outputs, high state for the input is detected when the value exceeds 1.8V)
5. Components that control the same parameter/behavior of a module are either:
   - linked with a line
   - share the same parameter label
6. There are three potentiometer types:
   - parameter offset - identified by the textual label nearby (usually at the bottom of the module). Affects the parameter
   - voltage attenuverter - identified by two textual labels `-` & `+` (usually just above parameter offset potentiometers). Multiplies incoming voltage by a number between -1 and 1
   - voltage attenuator - no labels (usually just above parameter offset potentiometers). Multiplies incoming voltage by a number between 0 and 1
7. Toggle switches are **ON** when in **upright position**
8. Bi-color LEDs display **green** when the associated voltage is **positive**, **red** when **negative**

**If in doubt, please hover over the module's components to see brief description or take a look into specific module's manual page.**
## Voltage standard
### Gates and Triggers (red ring; digital behavior):
- All **inputs** detect **high state** for any voltage **above 1.8V**
- Almost all outputs generate 5V for high state and 0V for low state (please refer to individual module's manual for more details)
### Audio and Control Voltage (white ring; continuous voltage):
- Any voltage is accepted, almost all inputs are equipped with attenuation options. The signal usually is restricted and conditioned collectively, after adding all voltages for particular input or parameter
- Outputs may generate signals from following voltage ranges (with few exceptions - please refer to individual module's manual for more details):
  - -5V to 5V
  - 0V to 5V
  - 0V to 10V