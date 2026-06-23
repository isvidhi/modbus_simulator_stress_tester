# modbus_simulator_stress_tester
A Modbus Simulator/Stress testing utility.
This project provides a robust, multi-threaded **Modbus Simulator** written in C++ using `libmodbus`. It is designed to act as a virtual PLC (Programmable Logic Controller) for testing SCADA systems, HMI applications, or any Modbus Master that needs to communicate over TCP or RTU protocols.

---

# Modbus Simulator Project

## Overview

This application serves as a flexible, dynamic Modbus Slave simulator. It allows developers to test their Master applications without requiring physical hardware. It supports both **Modbus TCP** and **Modbus RTU** and provides real-time introspection of read/write traffic.

## Key Features

* **Multi-Protocol Support:** Easily switch between TCP and RTU by changing the `ModbusConfig`.
* **Dynamic Data Simulation:** An internal thread-safe engine updates register and coil values based on user-defined strategies.
* **Traffic Introspection:** Real-time logging of both Read (FC 01-04) and Write (FC 05, 06, 15, 16) requests, including address tracking and delta-monitoring for changes.
* **Master Reconnection Logic:** Automatically handles client disconnections and re-binds without requiring a restart.

---

## Data Simulation Strategies

The simulator engine applies behaviors to your Modbus mapping at 500ms intervals. Users can toggle these modes dynamically:

* **Random Strategy:** Adds a small random jitter (e.g., ±5) to current values, simulating natural sensor noise.
* **Incremental Strategy:** Cycles values continuously (e.g., 0 → 1000) to simulate counters or time-based progression.
* **Manual Strategy:** Keeps data static until the Modbus Master writes a new value, or you programmatically update a register via the simulator API.

---

## Architecture & Thread Safety

The simulator separates the **Network Loop** (which handles Modbus protocol handshakes) from the **Simulation Thread** (which updates data).

* **Thread Safety:** A `std::mutex` is used to prevent data races between the `modbus_reply` network calls and the background generator threads.
* **Lifecycle Management:** The `running` flag (atomic) ensures graceful shutdowns, preventing memory leaks and dangling threads.

---

## Technical Implementation Details

* **Introspection:** Every incoming packet is parsed to extract the **Function Code**, **Starting Address**, and **Quantity**.
* **Delta Check:** For write operations, the simulator compares the current state with a "previous state snapshot," printing detailed logs only when a specific value changes.
* **Bitwise Reassembly:** The simulator uses bitwise shifting to reconstruct 16-bit addresses and quantities from the big-endian byte array (`query`) received from the Modbus Master.

---

## Getting Started

To integrate this into your project, define a `ModbusConfig` object and initialize the simulator:

```cpp
ModbusConfig config;
config.type = ModbusConfig::Type::TCP;
config.ip = "127.0.0.1";
config.port = 5020;
config.map = {100, 100, 100, 100}; // Coils, Input Bits, Holding, Input Registers

ModbusSimulator sim(config);
sim.run();

```

---

*This project is designed to be a "living" test environment. This documentation was written with help from Gemini*
