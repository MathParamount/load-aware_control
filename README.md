# Overview

This project was developed to address limitations identified in a previous implementation, particularly concerning inrush current conditions. The system implements a load-aware control mechanism with current-based decision making to classify and measure dynamic power consumption, extending to larger circuits that were previously unaddressed.

The initial concept involved machine learning-based load control using decision trees, but this approach was deprioritized due to physical hardware constraints, economic considerations, and time limitations. Nevertheless, this project provided valuable learning opportunities in noise detection, accuracy improvement, and IoT project decision-making. Key learnings include snubber circuit, signal analysis, SSR types, and circuit impedance.

# System architecture

## Physical circuit
The circuit evolved from an initial prototype with over 15 jumper wires, three parallel capacitors, and additional components, to a simplified design using only 12 jumpers, a single 470 µF capacitor, and an LED. This reduction minimized oscillations and improved modularity. The image below shows the initial circuit implementation.

![circuit1](https://github.com/user-attachments/assets/75ad59d9-dd8d-4524-b632-9d516d7e35da)


Two Solid State Relays (SSRs) were implemented in series with test loads. Through experimentation, it was determined that placing the ACS712-5B current sensor in series with only the motor (rather than monitoring combined loads) significantly improved measurement accuracy and reduced noise interference.

## State machine
The system employs a finite state machine (FSM) for operational control. The initial six-state design was simplified to four core states for improved clarity and reliability:
1. IDLE: All loads deactivated; awaits user command

2. LAMP_ON: Lamp activated; can transition to motor start if commanded

3. MOTOR_STARTING: Validates inrush current against limits

4. MOTOR_ON: Motor running; can transition to lamp if commanded

[state_machine_project.pdf](https://github.com/user-attachments/files/24795252/state_machine_project.pdf)


The FSM ensures mutually exclusive activation of lamp and motor to prevent current saturation and component stress. Command clearing at each state transition prevents command accumulation and FSM instability.


# Key variables

- Inrush Detection: Implemented with inrush_limit to identify motor startup surges and define operational limits.

- Stability Monitoring: Uses stable_thresh and timing parameters for steady-state operation validation.

- Noise Filtering: Initial 0.15A minimum current threshold was removed after circuit optimization eliminated significant ACS712-5B noise.

# Implementation details

## Current measurement system

- ACS712-5B current sensor interfaced with ESP32 analog input (Pin 34)
- Dynamic offset calibration for noise compensation
- Inrush detection via 14.7ms sampling window with peak current analysis
- RMS current calculation at 500Hz sampling rate

## Stability monitoring

- Error calculation based on RMS current deviation
- Threshold-based stability determination using stable_thresh
- Continuous monitoring for steady-state operation

# Observations

## Technical optimization

Initial RC filtering (100kΩ + 10nF, 159Hz cutoff) attenuated both noise and legitimate load signals, causing false positives during 200ms peak detection windows. The optimal configuration was determined to be 660µF total capacitance (3×220µF parallel), which effectively suppressed oscillations without signal degradation.

![circuit_2](https://github.com/user-attachments/assets/92a98b4d-d17d-4887-bb82-62d2e6058d89)


## SSR implementation

Two SSRs were required for independent control of cooler and lamp loads. These components provided optical isolation and reliable AC/DC switching capabilities.

## Measuremente refinements

The first refinement was the peak current detection threshold adjusted from 0.1A to 1.1A based on empirical lamp startup characteristics.
The second improvement was the LED indicator implemented for visual system status feedback.

# Code structure

The implementation includes:
- Pin mapping definitions for sensors and SSRs
- Dynamic offset calibration routines
- Current reading with ADC conversion and reference scaling
- Inrush detection with configurable timing windows

<img width="839" height="479" alt="update_inrush" src="https://github.com/user-attachments/assets/cdf8a9bc-498c-4d6f-97da-8a97bba1b22c" />


- RMS calculation and stability monitoring functions
- Integrated system function calls

<img width="831" height="327" alt="loop" src="https://github.com/user-attachments/assets/18cd46d5-3f30-4efb-aed3-7dcae4670608" />


# Conclusion

This project successfully addressed initial technical challenges while providing substantial learning value. Although the machine learning component was deprioritized, the development process significantly enhanced critical thinking abilities, practical circuit implementation skills, and understanding of electrical phenomenals.
The work demonstrates effective problem-solving in IoT system design, combining ESP32 microcontroller programming with hardware optimization to create a robust current monitoring and control solution. The experience has substantially improved both programming proficiency and physical system analysis capabilities.
