# Retro-Logger

In today's hyper-connected world, nearly every device—from your phone to your fridge—is online. But long before the rise of the Internet of Things, the industrial world had already embraced digital communication through protocols like **RS-232 (1960s)** and **Modbus (1979)**. These standards formed the foundation of modern industrial automation and still power countless machines around the globe.

## 🏭 The Reality of Legacy Equipment

Here’s the thing:

- These old machines **still work**.
- They’re **precise**, **reliable**, and **battle-tested**.
- But replacing them? **Expensive and disruptive**.

More than **70% of global manufacturing** still relies on legacy equipment. Full system upgrades can cost **hundreds of thousands of dollars** and result in **weeks of downtime**.

### 💡 So… why not make the *old* smart instead of replacing it?

---

## 🔧 Introducing Retro-Logger

**Retro-Logger** is a secure, ESP32-based IoT data logger designed to bring vintage industrial machines into the cloud era. It reads data from UART (RS-232/RS-485/TTL) serial ports—standard on most legacy systems—and pushes it to platforms like **Google Firebase** in real-time.

### 🔩 What's Inside?

- ✅ **ESP32** microcontroller  
- ✅ **4 Status LEDs**:  
  - Power  
  - WiFi  
  - Data Receive  
  - Cloud Sync  
- ✅ **4-Pin UART Connector** for easy machine/PLC integration  
- ✅ **Secure Boot V2** support (enabled via [Thistle.tech](https://thistle.tech)) to prevent firmware tampering

All enclosed in a 3D-printed, tamper-resistant case.

---

## 🧪 Demo Scenario

We simulate a real-world production line using an **Arduino Uno** that:
- Randomly generates structured test data (winding, assembling, testing, errors)
- Sends this data via UART to the ESP32-based Retro-Logger
- Retro-Logger then parses and securely uploads this data to the cloud

---

## 🔐 Security Matters

Retro-Logger supports **Secure Boot V2**, which ensures:
- Only trusted firmware is executed
- No one can modify or flash unauthorized firmware
- Devices are **fused once for production** to lock them down securely  
(Thanks to our security partner: [Thistle.tech](https://thistle.tech))

---

## 🔌 Compatibility

With simple code tweaks, Retro-Logger can be adapted to communicate over:
- RS-232 / RS-485  
- Modbus RTU  
- MQTT  
- HTTP / REST APIs  

Whether you're retrofitting a 30-year-old lathe or a vintage PLC, Retro-Logger is your affordable bridge to the Industrial IoT.

---

## 🛠️ Get Started

1. Flash the ESP32 with firmware from `/data-logger`
2. Simulate machine data using Arduino sketch from `/Data-Generatore`
3. Connect the ESP32 to UART (TX/RX, GND, VCC)
4. View logs in Firebase!

---

## 📹 Watch the Full Video

[👉 YouTube Demo: Retro-Logger in Action (Coming Soon)](https://www.youtube.com/@makerbrains)

---

## 📣 Credits

**Project by:** [Mukesh Sankhla](https://github.com/MukeshSankhla)  
**Secure Boot Support by:** [Thistle.tech](https://thistle.tech)  
**Website:** [makerbrains.com](https://makerbrains.com)

---

## 🧠 License

This project is open source under the MIT License.  
Feel free to fork, contribute, and build upon it.
