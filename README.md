# ESP32-Automated-Shooting-System
An automated shooting system powered by ESP32 and ESP32 Camera, integrated with AI-based object detection. This system features a self-tuning algorithm that enhances targeting accuracy by continuously adjusting the alignment based on real-time detection data.

![2c0a8128-3319-469d-997f-152faff4bb6c](https://github.com/user-attachments/assets/7ce565c9-da6d-4a2b-9a20-f933df69a18a)

Software and Hardware Design & Development
1. Mechanical Design
The mechanical components of the system, including the metal frame and servo placements, were designed using SOLIDWORKS. Key design elements include:

-Servo-Driven Pitch and Yaw System: This system enables precise targeting by adjusting the camera’s position in both horizontal and vertical axes.
-Gear Systems: These gears are used to control rotational and vertical movements, providing smooth and accurate motion.
-High-Torque 3D-Printed and Laser-Cut Gears: These components are designed for durability and strength, ensuring that the system can operate in outdoor environments and handle high-torque applications.
2. Electronic System
The electronic system is powered by the ESP32 and ESP32-CAM, which are programmed using Arduino IDE. Key components of the system include:

-PIR Sensors: Used for motion detection, the PIR sensors continuously monitor the environment for any signs of movement.
-Servo Motors: These motors adjust the system’s targeting mechanism based on motion detection and AI processing.
-AI Object Detection Model: The system utilizes a trained AI model that detects monkeys, enabling the system to respond with precision. This model was trained using Edge Impulse and optimized for deployment on the ESP32-CAM, ensuring real-time object detection.
3. AI Model
The AI object detection model was trained using Edge Impulse on a custom dataset of monkey images, collected under varying environmental conditions. The dataset includes images of monkeys in different poses, lighting, and backgrounds to ensure robust object recognition. The model was fine-tuned to balance accuracy and computational efficiency, enabling it to run effectively on the ESP32-CAM with limited resources. The trained model is seamlessly integrated into the Arduino IDE to process real-time video feeds and identify monkeys.

4. Coding & Flow
The program is structured with a clear flow to ensure accurate and timely responses. The sequence of operations is as follows:

-Motion Detection: The PIR sensors continuously monitor the environment. Upon detecting motion, the system proceeds to the next steps.
-Image Capture: The ESP32 camera captures an image of the detected area for further analysis.
-AI Processing: The captured image is processed by the trained AI model, which identifies whether the detected object is a monkey.
-Targeting Adjustment: If a monkey is detected, the system's self-tuning algorithm refines the alignment of the targeting system. This algorithm uses the detected object’s coordinates to adjust the servo motors for accurate targeting.
-Shooting System Activation: Once the targeting system is aligned, the gel blaster is triggered to repel the monkey, ensuring the system operates in a humane manner.
