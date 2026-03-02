# Real-Time Motor Condition Monitoring System using ESP32 & Frequency-Domain Analysis

## Overview
This project implements a **real-time embedded system** for monitoring DC motor conditions using the **ESP32 microcontroller**. The system captures high-speed current signals, performs **frequency-domain analysis** using **Fast Fourier Transform (FFT)**, and identifies potential motor faults in real-time.

Designed for predictive maintenance, this system can detect deviations in motor behavior such as overloading, bearing defects, Misalignment

---

## Key Features
- Real-time **motor fault detection** using current signal monitoring.
- High-speed ADC sampling at **2 kHz** for accurate signal acquisition.
- **Frequency-domain analysis** via FFT for spectral fault identification.
- **Automatic baseline calibration** to capture healthy motor operating frequencies.
- **Classification logic** to detect anomalies in dominant spectral components and RMS current.
- Simulation of real-world motor faults:
  - Overloading  
  - Bearing defects (frequency modulation)  
  - Misalignment
- Adaptive recalibration when motor is turned off and restarted.
- Integration of **PWM motor control**, real-time signal processing, and **LCD-based fault visualization**.

---

## System Architecture
```text
DC Motor → Current Sensor → ESP32 ADC → FFT & RMS Computation → Fault Classification → LCD Display
