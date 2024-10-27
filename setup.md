NS-3 IoT Simulation Setup Guide
===============================

This guide will help you set up your environment, download the necessary tools, and run the IoT network simulation using the provided `IOT.cc` source code.

---

## Prerequisites

**1. NS-3 Installation:**  
   Ensure that NS-3 is installed on your system. You can download NS-3 from the official website. Use the following commands to install NS-3:

    ```
    sudo apt-get update
    sudo apt-get install g++ python3 cmake libc6-dev libc6-dev-i386 libclang-dev llvm-dev
    sudo apt-get install qtbase5-dev
    wget https://www.nsnam.org/release/ns-allinone-3.xx.tar.bz2
    tar xjf ns-allinone-3.xx.tar.bz2
    cd ns-allinone-3.xx/ns-3.xx
    ./build.py --enable-examples --enable-tests
    ```

   For version-specific instructions, refer to the NS-3 Installation Guide.

**2. Git Installation:**  
   Install Git to clone and manage this repository:

    ```
    sudo apt-get install git
    ```

**3. NetAnim Installation:**  
   To visualize the simulation, install NetAnim using the following commands:

    ```
    sudo apt-get install qt5-qmake qtbase5-dev
    wget https://www.nsnam.org/release/netanim-3.108.tar.bz2
    tar xjf netanim-3.108.tar.bz2
    cd netanim-3.108
    qmake NetAnim.pro
    make
    ```

---

## Cloning the Repository

Clone this repository, which contains the `IOT.cc` source code:
   git clone https://github.com/richiekrich/IOT_Simulation.git 
   cd NS3-IoT-Simulation
---

## Setting Up the Project

1. **Navigate to the NS-3 scratch folder:**

    ```
    cd ~/ns-3/scratch
    ```

2. **Copy the provided `IOT.cc` file into the scratch folder:**

    ```
    cp ../NS3-IoT-Simulation/IOT.cc .
    ```

   Ensure that the `IOT.cc` file is located in the `~/ns-3/scratch` directory.

---

## Building and Running the Simulation

1. **Build NS-3**  
   Build the project in NS-3 after adding the `IOT.cc` file:

    ```
    cd ~/ns-3
    ./ns3 build
    ```

2. **Run the Simulation**  
   Once the build is complete, run the simulation:

    ```
    ./ns3 run scratch/IOT
    ```

---

## Visualizing the Simulation

To visualize the simulation, ensure that XML output is generated in your simulation file. After the simulation runs, open the animation file using NetAnim:

./NetAnim iot-animation.xml


---

## Simulation Results

- During the simulation, the UDP Echo Client-Server sends and receives packets between the sensor and cloud nodes.
- The simulation outputs logs with packet send/receive timestamps.

---

## Troubleshooting

- **Compilation Issues:**  
  - Ensure all dependencies are installed as per the prerequisites.
  - If the build fails, refer to the NS-3 documentation for troubleshooting common issues.

- **NetAnim Visualizer Not Working:**  
  - Verify that NetAnim is installed correctly.
  - For missing library errors, ensure dependencies like `qt5-qmake` are installed.

- **Network Behavior:**  
  - Confirm the correct setup of nodes, links, and IP addresses as defined in the simulation code.

