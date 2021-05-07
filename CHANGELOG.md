# Legend
* FIX: A bug fix.  Relevant to users and developers.
* NEW: A new feature. Relevant to users and developers.
* SYS: A system-level change.  Usually only relevant to developers.
* vN.NN*: Only releases marked with an asterik are relevant to ER-102 users.

# v2.09*
* FIX: While in HOLD mode, pressing COMMIT when the COPY LED is flashing would cause corruption of the sequence and eventually a crash.
* FIX: There was no de-bouncing when sending button events to the ER-102.

# v2.08
* FIX: Dropped gate when clock precedes the reset signal.

# v2.07
* FIX: Random MATH operator had a minimum output value of 1.

# v2.06
* FIX: CV outputs overflow when clock divider > 1 and SMOOTH is enabled.
* FIX: While editing a MATH transform, the focus LED (orange) gets turned off when you push the RESET button or receive a gate on the RESET input.
* NEW: Show nP (number of pulses) when focus pressing the PATTERN or TRACK.

# v2.05p1
* FIX: Problem controlling the orange LEDs when using the ER-101 with the ER-102.

# v2.05
* NEW: Flash actual loop step number when setting loop points with PATTERN focused.
* FIX: Copying across snapshots in HOLD MODE has bugs.
* FIX: Math transform gets applied when pinning the math screen.
* Note to ER-102 users: Do not use this firmware for your ER-101, it has problems controlling the orange LEDs.

# v2.04
* NEW: On start-up, load last saved or *loaded* snapshot.
* NEW: Pattern-based looping (press LOOP buttons while PATTERN is focused to quickly loop patterns).
* NEW: Blank snapshot (displayed as '--') can be loaded to start a fresh snapshot.
* FIX: Crash if you turn knob during startup splash screen.
* NEW: Hold INSERT button, select SPLT with the RIGHT knob to split steps or patterns.
* NEW: Added help indicators to the track config screen.

# v2.03
* FIX: clock display was overwriting the MATH overlay
* NOT FIXED: During startup, ER-101 might display the message "14 bit" even though you have an older 12-bit device.  This has no impact on the operation of your module.

# v2.02*
* SYS: Improved stability of the ER-101/102 communciation.
* FIX: Lots of little user interface improvements.
* NEW: Simultaneous Clock Divide and Multiply.  Now you can divide the clock as well as multiply the clock differently for each track. "Focus press" the TRACK button to get to the track configuration screen and adjust the divider and multiplier amounts 

# v2.00*
* SYS: The main purpose of this release is to add support for the ER-102 Sequencer Controller.
* FIX: GATE output is set low when the corresponding track is deleted.
* FIX: Suppress voltage display when showing track configuration.
* FIX: BLUE and 22JT voltage tables were incorrectly swapped.
* FIX: Reset all timing counters to zero when PAUSE button is pushed.  
* NEW: added qt (quantize) operation to the MATH transform.

# v1.12p2
* FIX: more robust synchronization with external step sequencers via RESET (i.e. handle reset pulses that arrive soon before or after the clock edge)
* FIX: pausing the sequencer no longer affects the ER-101's estimate of clock period that is used in the clock multiplication algorithm.
* SYS: Plus some internal optimizations in preparation for the ER-102 Sequencer Controller.

# v1.10
* NEW: Track Configuration screen.
* NEW: Note Display mode. When note display is enabled for a particular track and table then the VOLTAGE window will show: {octave:0-8}.{note:A-G}.{percent of a whole tone:0-99}
* NEW: Repeatedly pressing the VOLTAGE focus button (or turning the RIGHT knob while holding this button) cycles through 3 levels of granularity.
* NEW: Trigger Mode. Once you have enabled triggers for a particular track (as above procedure) then all gates for this track become triggers with a duration equal to the steps's GATE value multiplied by 0.1ms. So if a particular step has GATE=10 then the trigger will last 10 x 0.1ms = 1ms. Particularly when pinging a filter, different trigger lengths will sound different. The range for triggers is 0.1ms to 9.9ms in 0.1ms increments.
* NEW: Clock Multiplication.  This one is pretty simple. Once you enable clock multplication for a given track, then that track will just behave as if it was given a faster clock. For example, if you configure a track's clock multiplier to 48 then for each single pulse of the global clock, the track in question will receive 48 pulses and thus run 48 times faster. In this track, a step with DURATION=48 will last for exactly one single cycle of the global clock. 
* NEW: One MATH Transform per TRACK.  Each track now has its own transform via the MATH button, rather than one global transform shared by all tracks.
* NEW: Last Saved Snapshot.  On power up, the ER-101 will load the last saved snapshot.
* NEW: User Reference Tables.  There are now 8 user reference tables (USr1 to USr8) that can be used to store your own customized voltage tables. They are accessed and copied the same way as other reference tables. You alter the contents of a user reference table by copying a track's voltage table (A or B) and pasting it onto the desired user table. These tables are not included in the snapshots and are not affected by loading of a new snapshot. These user tables are truly just like another reference table and are always available, unless overwritten by a firmware upgrade.

# v1.09
* NEW: Transferring pulses between adjacent steps. You can now adjust a step boundary (thus keeping the pattern/track duration unchanged)  by just holding down the DURATION button and using the RIGHT knob to steal pulses from or give pulses to the next step (LOOP points are considered). While you are transferring pulses, the VOLTAGE display will temporarily show explicitly how the duration of the current and next step are changing. This method is especially useful for making rhythmic changes to your sequence without losing sync with other material.
* NEW: Note repeat (also known as ratcheting) for each step. If you press the GATE button while the GATE parameter is focused then the continuous trigger (i.e. ratchet) mode is toggled for that current step (indicated by the 2 dots lighting up in the GATE display). Ratcheting means that the gate will repeatedly fire for the duration of the step. The value of the GATE parameter still indicates the length of the gate signal. The step DURATION and GATE together determine how many times the gate output will trigger. For example, if the step DURATION is 16 and the GATE is 4 pulses (on the display shown as '0.4.') then the gate will go high for 4 pulses, low for 4 pulses, high again for 4 pulses and then low again for 4 pulses.
* NEW: In addition to the focus buttons, you can now use the LEFT knob to change the transform when setting a MATH operation.
* FIX: On some power supplies the ER-101 might leak a 366Hz tone on to the power rails. The reason was that the PWM frequency for the orange LEDs was mistakenly set to audio range (366Hz) and below the cutoff frequency of the ER-101's power filter. The fix was to set the PWM frequency to 200kHz, well above the cutoff frequency on the power filter for the ER-101.
* FIX: The ER-101 might freeze when you set LOOP points in a sequence with all steps having zero DURATION. This has been fixed now.

# v1.08
* FIX: Turning on smooth for a single step's CV-B causes the output voltage to increase for that step.
* NEW: Hold down the COPY button to select a range of patterns or steps for copying to the clipboard.

# v1.07
* NEW: Pressing DELETE while in MATH mode clears the current transformation.
* FIX: Copying a sequence ends with the first empty pattern encountered when it should copy the empty pattern and continue.
* FIX: If a loop start or end step is deleted the the loop point might appear randomly again when inserting a new step.
* FIX: Keep the focus track when loading a snapshot rather than setting it back to track #1.
* NEW: Add confirmation to SAVE snapshot button.
* NEW: Time quantization of RESET and COMMIT button pushes to next STEP or next PATTERN or next TRACK LOOP. Just hold down the STEP, PATTERN, or TRACK button when pressing either RESET or COMMIT.

# v1.06
* FIX: Even though the max number of patterns was increased to 400, only 255 could be used due to the use of a byte to address into the pattern memory pool. With this bug fix now the full 400 patterns are addressable and the max # of steps is increased to 2000.
* FIX: Sometimes the COPY (and a few others) button will stop responding.

# v1.05
* FIX: Copying and inserting steps on the edit cursor while a sequence is playing sometimes results in incorrect step display values for the play cursor.
* FIX: When copying tracks the user voltage tables are not being copied.
* FIX: SMOOTH flags are not copied when using COPY and INSERT buttons.
* FIX: Loop points are not copied when copying a track.
* FIX: Should TILT when trying to change step parameters in follow mode and unpaused.
* FIX: When copying patterns, if pattern memory is exhausted, new pattern creation will fail but the steps will still be copied into the current focused pattern. Steps should not be copied.