# Sensing Subsystem Summary (NYNSOM001)

## Objective
Detect low-height predators such as honey badgers using IR beam and PIR sensors, capture an image, and trigger the deterrent and notification subsystems.

## Components Used
- ESP32-CAM (microcontroller + camera)
- PIR motion sensor
- IR break-beam sensors
- MP1584 buck converter
- LM317 voltage regulator
- Perspex enclosure

## Trigger Logic
A predator is detected when:
- Top IR = HIGH
- Bottom IR = LOW
- PIR = HIGH  
This logic avoids false triggers from humans or flying birds.

## Output
- Activates GPIO12 and GPIO15 HIGH for 10 seconds
- Captures image and makes it accessible via HTTP GET
