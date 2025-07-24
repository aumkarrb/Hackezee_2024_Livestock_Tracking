#include <WiFi.h>
#include <esp_now.h>

// Access Point credentials
const char* ssid = "Cattle-Tracker-AP";
const char* password = "123456789";


WiFiServer server(80);


typedef struct struct_message {
    int ID;
    double GPS_N;
    double GPS_W;
    float Temperature;
    float Humidity;
    float Acc_x;
    float Acc_y;
    float Acc_z;
    float Air_Quality;
    float Battery;
} struct_message;


struct_message myData[3];
int deviceCount = 0; 

// esp now get data
void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
    struct_message receivedData;
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    
    if (receivedData.ID >= 1 && receivedData.ID <= 3) {
        myData[receivedData.ID - 1] = receivedData;
        Serial.printf("Received from Device %d: GPS_N=%lf, GPS_W=%lf, Temp=%f, Humidity=%f, Acc_x=%f, Acc_y=%f, Acc_z=%f, Air_Quality=%f, Battery=%f\n",
                      receivedData.ID, receivedData.GPS_N, receivedData.GPS_W,
                      receivedData.Temperature, receivedData.Humidity, receivedData.Acc_x, 
                      receivedData.Acc_y, receivedData.Acc_z, receivedData.Air_Quality, receivedData.Battery);
        deviceCount = (receivedData.ID > deviceCount) ? receivedData.ID : deviceCount; // Update device count
    }
}

// connection handling
void handleClient(WiFiClient client) {
    String currentLine = "";

    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            Serial.write(c);  

            // If you get a newline, end of the HTTP request
            if (c == '\n') {
                // Empty line indicates the end of the client's request
                if (currentLine.length() == 0) {
                    // HTTP response
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();
                    client.println("<!DOCTYPE html><html lang='en'>");
                    client.println("<head>");
                    client.println("<meta charset='UTF-8'>");
                    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
                    client.println("<title>Cattle Tracker</title>");
                    client.println("<style>");
                    client.println("body { font-family: 'Roboto', sans-serif; background-color: #1a1a2e; color: #fff; margin: 0; padding: 0; }");
                    client.println("header { background-color: #0f3460; color: #fff; padding: 20px; text-align: center; font-size: 32px; font-weight: bold; letter-spacing: 2px; }");
                    client.println(".tagline { text-align: center; margin: 10px; font-size: 18px; color: #e0e0e0; }");
                    client.println(".container { display: flex; flex-wrap: wrap; justify-content: center; margin: 20px; gap: 20px; }");
                    client.println(".widget { background: linear-gradient(135deg, #6a11cb 0%, #2575fc 100%); box-shadow: 0 4px 12px rgba(0, 0, 0, 0.4); border-radius: 10px; width: 260px; padding: 20px; text-align: center; cursor: pointer; transition: transform 0.3s ease, background-color 0.3s ease; color: white; font-weight: bold; }");
                    client.println(".widget:hover { transform: scale(1.05); background: linear-gradient(135deg, #fc4a1a 0%, #f7b733 100%); }");
                    client.println(".widget h3 { margin: 0; color: #fff; font-size: 22px; }");
                    client.println(".widget p { margin: 10px 0; color: #e0e0e0; font-size: 16px; }");
                    client.println(".widget-details { margin: 10px 0; background-color: rgba(0, 0, 0, 0.2); padding: 15px; border-radius: 5px; }");
                    client.println(".warning { color: red; font-weight: bold; }");
                    client.println(".map-link { display: inline-block; margin-top: 5px; color: #00aaff; text-decoration: none; }");
                    client.println(".map-link:hover { text-decoration: underline; }");
                    client.println("</style>");
                    client.println("</head>");
                    client.println("<body>");
                    client.println("<header>Cattle Tracking & Monitoring System</header>");
                    client.println("<p class='tagline'>Real-time Monitoring of Temperature, Humidity, Battery, and More</p>");
                    client.println("<div class='container' id='deviceContainer'>");
                    client.println("<!-- Widgets will be dynamically inserted here -->");
                    client.println("</div>");
                    client.println("<script>");
                    client.println("const ACCELERATION_THRESHOLD = 15.0;"); // Threshold for acceleration warning

                    
                    client.println("function loadDevices() {");
                    client.println("fetch('/data').then(response => response.json()).then(devices => {");
                    client.println("const container = document.getElementById('deviceContainer');");
                    client.println("container.innerHTML = ''; // Clear existing widgets");
                    client.println("devices.forEach(device => {");
                    client.println("const widget = document.createElement('div');");
                    client.println("widget.className = 'widget';");
                    client.println("let warningMessage = '';");

                    // acceleration warning
                    client.println("if (Math.abs(device.Acc_x) > ACCELERATION_THRESHOLD || Math.abs(device.Acc_y) > ACCELERATION_THRESHOLD || Math.abs(device.Acc_z) > ACCELERATION_THRESHOLD) {");
                    client.println("warningMessage = '<p class=\"warning\">Warning: High Acceleration Detected!</p>'; }");

                    // google maps link
                    client.println("const mapLink = `https://www.google.com/maps?q=${device.GPS_N},${device.GPS_W}`;");
                    client.println("widget.innerHTML = `<h3>${device.ID}</h3><div class='widget-details'><p>Battery: ${device.Battery}%</p><p>Temperature: ${device.Temperature}°C</p><p>Humidity: ${device.Humidity}%</p><p>Air Quality: ${device.Air_Quality}</p><p>GPS: (${device.GPS_N.toFixed(2)}, ${device.GPS_W.toFixed(2)})</p><a class='map-link' href='${mapLink}' target='_blank'>View on Map</a><p>Acc_x: ${device.Acc_x}</p><p>Acc_y: ${device.Acc_y}</p><p>Acc_z: ${device.Acc_z}</p>${warningMessage}</div>`;");
                    client.println("container.appendChild(widget);");
                    client.println("});");
                    client.println("});");
                    client.println("}");

                    
                    client.println("window.onload = () => { loadDevices(); setInterval(loadDevices, 2000); };");
                    client.println("</script>");
                    client.println("</body>");
                    client.println("</html>");
                    client.println();
                    break;  // Break out of the while loop to end the response
                } else {
                    currentLine = "";  // Clear the current line
                }
            } else if (c != '\r') {
                currentLine += c;  // Collect the characters in a line
            }
        }
    }
    client.stop();  // Close the connection
}

// Serve the JSON data for AJAX requests
void handleDataRequest(WiFiClient client) {
    String json = "[";
    for (int i = 0; i < deviceCount; i++) {
        if (i > 0) json += ",";
        json += "{\"ID\": " + String(myData[i].ID) + ", \"GPS_N\": " + String(myData[i].GPS_N, 6) + 
                ", \"GPS_W\": " + String(myData[i].GPS_W, 6) + ", \"Temperature\": " + String(myData[i].Temperature) + 
                ", \"Humidity\": " + String(myData[i].Humidity) + ", \"Acc_x\": " + String(myData[i].Acc_x) + 
                ", \"Acc_y\": " + String(myData[i].Acc_y) + ", \"Acc_z\": " + String(myData[i].Acc_z) + 
                ", \"Air_Quality\": " + String(myData[i].Air_Quality) + ", \"Battery\": " + String(myData[i].Battery) + "}";
    }
    json += "]";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type: application/json");
    client.println();
    client.println(json);
}

void setup() {
    Serial.begin(115200);

    // Initialize WiFi
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.println("Access Point started: " + String(ssid));

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the callback function to handle received data
    esp_now_register_recv_cb(OnDataRecv);

    // Start the server
    server.begin();
    Serial.println("Server started");
}

void loop() {
    WiFiClient client = server.available();  // Check for incoming clients
    if (client) {
        String currentLine = client.readStringUntil('\r');  // Read the first line of the request
        Serial.println(currentLine);  // Output the request to the serial monitor
        if (currentLine.startsWith("GET /data")) {
            handleDataRequest(client);  // Serve JSON data
        } else {
            handleClient(client);  // Serve HTML
        }
    }
}
