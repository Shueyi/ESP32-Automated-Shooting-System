# ESP32-Automated-Shooting-System
An automated shooting system powered by ESP32 and ESP32 Camera, integrated with AI-based object detection. This system features a self-tuning algorithm that enhances targeting accuracy by continuously adjusting the alignment based on real-time detection data.

![2c0a8128-3319-469d-997f-152faff4bb6c](https://github.com/user-attachments/assets/7ce565c9-da6d-4a2b-9a20-f933df69a18a)

https://github.com/user-attachments/assets/c7abd186-8823-44f6-9b8d-12a34a16942f

Software and Hardware Design & Development

1. Mechanical Design

The mechanical components of the system, including the metal frame and servo placements, were meticulously designed using SOLIDWORKS to ensure precision and durability. The design features a servo-driven pitch and yaw system that allows for accurate targeting adjustments. Additionally, gear systems were incorporated to control both rotational and vertical movements, ensuring smooth and precise motion. To handle the high-torque requirements of the system, 3D-printed and laser-cut gears were used, which offer strength and durability to withstand outdoor conditions and provide consistent operation.

2. Electronic System

The system's electronic components are powered by the ESP32 and ESP32-CAM, which are programmed using the Arduino IDE. The core of the electronic system includes PIR sensors for motion detection, which constantly monitor the surrounding area for movement. Upon detecting motion, the system triggers the servo motors to adjust the targeting position of the camera and mechanism. The AI object detection model, which was trained using Edge Impulse, plays a key role in analyzing the images captured by the camera and identifying if the detected object is a monkey. The integration of these components allows for real-time responsiveness and seamless operation of the system.

3. AI Model

The AI model used for object detection was developed with Edge Impulse, utilizing a custom dataset of monkey images under varying environmental conditions to ensure robustness and reliability. The training dataset included images of monkeys in different poses, lighting, and environmental factors, allowing the model to be highly adaptable to real-world scenarios. The model is optimized for deployment on the ESP32-CAM, balancing accuracy with computational efficiency to ensure fast object detection without overloading the system's resources. Once trained, the AI model was integrated into the Arduino IDE, enabling it to process real-time video feeds and identify monkeys, which is crucial for the system’s ability to respond quickly to movement.

4. Coding & Flow

The system’s programming follows a structured flow to ensure precise and timely actions. The process begins with the PIR sensors continuously monitoring for motion. Once motion is detected, the ESP32 camera captures an image for analysis. This image is processed by the trained AI model, which identifies whether the detected object is a monkey. If a monkey is detected, the system then activates a self-tuning algorithm that adjusts the positioning of the servo motors based on the coordinates of the detected object, ensuring accurate targeting. Finally, the system triggers the gel blaster to deter the monkey. The flow is designed to ensure that all components work cohesively and that the system can quickly and accurately identify and respond to potential threats.
