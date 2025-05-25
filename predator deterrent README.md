# Penguin Predator Deterrent System

## Overview
A modular system designed to detect and deter predators—particularly honey badgers—from approaching the fence of an African penguin colony. The system includes sensing, notification, and deterrent subsystems, built to operate in remote, low-power environments.

## Subsystems and Team Roles

- **Sensing Subsystem – NYNSOM001**  
  Detects predator presence using PIR and IR beam sensors, captures an image, and sends signals to trigger other subsystems.

- **Notification Subsystem – MRWDOU002**  
  Requests the captured image from the sensing module and sends alerts to the client.

- **Deterrent Subsystem – WLTRON002**  
  Activates non-harmful sensory stimuli (lights/sounds) to scare away predators.

## Features
- Image server hosted on ESP32-CAM
- Predator-triggered alert and deterrence
- Animal-safe, low-power, weather-resistant design
- Custom enclosure using Perspex for field deployment

## Directory Structure

