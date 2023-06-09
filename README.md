<div align="center">
  <h1>Auto-Billing-Trolley</h1>
  <img src="./Software/public/images/icon-192.png" alt="Logo">
</div>

## An IoT based self billing trolley project


The project consists of a hardware system that includes an ESP8266 microcontroller, an EM-18 RFID reader, and an OLED display. The software component involves a Node.js server implemented with Node-RED, enabling communication between the hardware and software components through the WebSocket protocol over the internet.

The system is designed to streamline the billing process by utilizing RFID technology. Each product in the shopping mall is equipped with a unique RFID code. When a customer adds a product to the trolley, the trolley's RFID reader scans the product's RFID tag. The microcontroller, specifically the ESP8266, sends the scanned RFID details to the Node.js server.

The Node.js server stores the product details along with its corresponding RFID code. The server's user interface, accessible through a WPA app, displays the current cart's product list and total amount based on the data received from the trolley. The OLED display on the trolley also shows the total amount and the total count of products in the cart.

To remove an item from the trolley, the hardware part of the trolley incorporates a button. When the user presses the button, the OLED display shows a "remove" text prompt. The user can then rescan the RFID tag of the item they wish to remove. Upon scanning, the microcontroller sends a removal request to the server, and the server updates the user's cart by removing the corresponding item.

By implementing this system, customers can easily add products to their cart by scanning the RFID tags, view the product list and total amount in real-time through the WPA app, and conveniently remove items with the help of the hardware button and OLED display. This auto-billing trolley project improves the overall shopping experience by automating the billing process, reducing manual effort, and providing accurate real-time information to the customers.

### Schematic
<img src="./Hardware/Schematic_Auto-Billing-Trolley_2023-05-17.png" alt="Schematic">

### Component Layout
<img src="./Hardware/Component Layout.png" alt="Component Layout">


## How to start the server?

1. Clone or [download](https://github.com/e-labInnovations/Auto-Billing-Trolley/archive/refs/heads/main.zip) this repository.
2. Install [Node.js](https://nodejs.org/en) on your PC.
3. Navigate to the `Software` folder.
4. Open Command Prompt or Windows Terminal in that folder.
5. Install the required packages by running the `npm install` command. (First time only)
6. Start the server by running the `npm start` command.
7. Visit `http://localhost:1880` or `http://127.0.0.1:1880` in your browser.
