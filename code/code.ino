#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MAX31865.h>
#include <math.h>

const char* ssid = "GU 22";
const char* password = "12345678";

WebServer server(80);

Adafruit_MAX31865 thermo = Adafruit_MAX31865(5, 23, 19, 18);
#define RREF      430.0
#define RNOMINAL  100.0
float a =  2.60510743;
float B = -260.83901918;
float C = 0.0039083;
float D = -0.0000005775;
float R0 = 100;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
        background-color: pink;
    }
    h2 { font-size: 3.0rem; color: white; }
    p { font-size: 3.0rem; color: white; }
    .units { font-size: 1.2rem; color: white; }
    .dht-labels{
        font-size: 1.5rem;
        vertical-align:middle;
        padding-bottom: 10px;
        color: white;
    }
    .fa-thermometer-half, .fa-tint, .fa-microchip { color: white; }
    .container {
        display: flex;
        justify-content: center;
        align-items: flex-start;
    }
    .info {
        text-align: left;
        color: white;
        font-size: 0.1rem;
        padding: 5px;
        margin-left: 300px;
        background-color: pink;
    }
  </style>
</head>
<body>
  <h2>ĐIỀU KHIỂN QUÁ TRÌNH</h2>
  <div class="container">
    <div class="data">
      <p>
        <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
        <span class="dht-labels">NHIỆT ĐỘ:</span> 
        <span id="temperature">%TEMPERATURE%</span>
        <sup class="units">&deg;C</sup>
      </p>
      <p>
        <i class="fas fa-microchip" style="color:#00add6;"></i> 
        <span class="dht-labels">ĐIỆN TRỞ:</span>
        <span id="dientro">%DIENTRO%</span>
        <sup class="units">Ohm</sup>
      </p>
      <p>
        <i class="fas fa-tint" style="color:#00add6;"></i> 
        <span class="dht-labels">HỒI QUY TUYẾN TÍNH:</span>
        <span id="humidity">%HUMIDITY%</span>
        <sup class="units">&deg;C</sup>
      </p>
      <p>
        <span class="dht-labels">TÍNH TOÁN TRỰC TIẾP:</span>
        <span id="A">%A%</span>
        <sup class="units">&deg;C</sup>
      </p>
    </div>
    <div class="info">
      <p><strong>Thành viên:</strong></p>
      <p>Lê Anh Khoa<br>MSSV: 2000805</p>
      <p>Trần Lộc Đỉnh<br>MSSV: 2000187</p>
      <p><strong>Giảng viên hướng dẫn:</strong></p>
      <p>TS. Đỗ Vinh Quang</p>
    </div>
  </div>
  
  <canvas id="chart" width="400" height="200"></canvas>
  <script>
    var ctx = document.getElementById('chart').getContext('2d');
    var chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Nhiệt độ',
                    borderColor: 'rgb(255, 99, 132)',
                    data: []
                },
                {
                    label: 'Tính toán trực tiếp',
                    borderColor: 'rgb(75, 192, 192)',
                    data: []
                },
                {
                    label: 'Hồi quy tuyến tính',
                    borderColor: 'rgb(153, 102, 255)',
                    data: []
                }]
            },
            options: {}
        });

    setInterval(function() {
      fetch('/data').then(function(response) {
        return response.json();
      }).then(function(data) {
        chart.data.labels.push(new Date().toLocaleTimeString());
        chart.data.datasets[0].data.push(data.NHIETDO);
        chart.data.datasets[1].data.push(data.tinhtoantructiep);
        chart.data.datasets[2].data.push(data.hoiquytuyentinh);
        chart.update();
      });
    }, 500);
  </script>
</body>
</html>
)rawliteral";

const char data_json[] PROGMEM = R"rawliteral(
{
  "NHIETDO": %TEMPERATURE%,
  "tinhtoantructiep": %A%,
  "hoiquytuyentinh": %HUMIDITY%
}
)rawliteral";

void setup() {
  Serial.begin(115200);
  thermo.begin(MAX31865_3WIRE);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    String response = String(index_html);
    float nhietdo = thermo.temperature(RNOMINAL, RREF);
    float RTD_t = thermo.readRTD() * RREF / 32768.0;
    float tinhtoantructiep = (-C + sqrt(C * C - 4 * D * (1 - RTD_t / R0))) / (2 * D);
    float dientro = RTD_t;
    float hoiquytuyentinh = a * RTD_t + B;
    
    response.replace("%TEMPERATURE%", String(nhietdo));
    response.replace("%DIENTRO%", String(dientro));
    response.replace("%HUMIDITY%", String(hoiquytuyentinh));
    response.replace("%A%", String(tinhtoantructiep));
    
    server.send(200, "text/html", response);
  });

  server.on("/data", HTTP_GET, []() {
    String response = String(data_json);
    float nhietdo = thermo.temperature(RNOMINAL, RREF);
    float RTD_t = thermo.readRTD() * RREF / 32768.0;
    float tinhtoantructiep = (-C + sqrt(C * C - 4 * D * (1 - RTD_t / R0))) / (2 * D);
    float hoiquytuyentinh = a * RTD_t + B;
    
    response.replace("%TEMPERATURE%", String(nhietdo));
    response.replace("%A%", String(tinhtoantructiep));
    response.replace("%HUMIDITY%", String(hoiquytuyentinh));
    
    server.send(200, "application/json", response);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop(){
  server.handleClient();
}
