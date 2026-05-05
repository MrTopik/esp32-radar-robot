#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

#define ECHO 14
#define TRIG 27

Servo myServo;
float duration, distance;
int angle = 0;
bool servo_way = true;
unsigned long lastServoMove = 0;
const unsigned long servoInterval = 50;

const char* ssid = "ESP32";
const char* password = "gordon1998";

WebServer server(80);

void setup() {
  // put your setup code here, to run once:
  myServo.attach(18);
  myServo.write(angle);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  server.on("/", handle_OnConnect);
  server.on("/readData", handle_GetData);
  server.onNotFound(handle_NotFound);
  server.begin();
  delay(5000);
  Serial.println(WiFi.softAPIP());
}

void handle_GetData() {String data = String(angle) + "," + String(distance); server.send(200, "text/plain", data);}
void handle_OnConnect() {server.send(200, "text/html", createHTML());}
void handle_NotFound() {server.send(404, "text/plain", "Not found");}

String createHTML() {
  String str = "<!DOCTYPE html><html><head>";
  str += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  str += "<style>body{background:#000; color:#0f0; text-align:center; font-family:monospace;} canvas{background:#111; border:2px solid #0f0; border-radius:50%;}</style>";
  str += "</head><body>";
  str += "<canvas id='radar' width='400' height='400'></canvas>";
  str += "<div id='info'>Angle: 0 | Dist: 0cm</div>";

  str += "<script>";
  str += "var canvas = document.getElementById('radar'); var ctx = canvas.getContext('2d');";
  str += "var angle = 0; var distance = 0;";

  str += "function drawRadar() {";
  str += "  ctx.clearRect(0, 0, 400, 400);";
  str += "  var cx = 200, cy = 200, r = 180;";
  // Draw circles
  str += "  ctx.strokeStyle='#040'; ctx.beginPath();";
  for(int i=1; i<=3; i++) { str += "ctx.arc(cx, cy, (r/3)*"+String(i)+", 0, 2*Math.PI);"; }
  str += "  ctx.stroke();";
  // Draw Sweep Line
  str += "  var rad = (angle - 90) * Math.PI / 180;"; // Offset by 90 to face "up"
  str += "  ctx.strokeStyle='#0f0'; ctx.lineWidth=3; ctx.beginPath(); ctx.moveTo(cx,cy);";
  str += "  ctx.lineTo(cx + r * Math.cos(rad), cy + r * Math.sin(rad)); ctx.stroke();";
  // Draw Target
  str += "  if(distance < 50 && distance > 0) {"; // Only draw if within 50cm
  str += "    var tr = (distance/50) * r;";
  str += "    ctx.fillStyle='red'; ctx.beginPath();";
  str += "    ctx.arc(cx + tr * Math.cos(rad), cy + tr * Math.sin(rad), 5, 0, 2*Math.PI); ctx.fill();";
  str += "  }";
  str += "}";

  str += "setInterval(function() {";
  str += "  fetch('/readData').then(r=>r.text()).then(d=>{";
  str += "    var v = d.split(','); angle=v[0]; distance=v[1];";
  str += "    document.getElementById('info').innerHTML = 'Angle: '+angle+' | Dist: '+distance+'cm';";
  str += "    drawRadar();";
  str += "  });";
  str += "}, 50);"; 
  str += "</script></body></html>";
  return str;
}

void distance_cal() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH, 30000);
  if (duration > 0) {
    distance = (duration * 0.0343) / 2;
  }
}

void servo_move(){
  if (millis() - lastServoMove >= servoInterval) {
    lastServoMove = millis();
    if (servo_way) {
      angle++;
      if(angle>=180){angle=180;servo_way = false;}
    }
    else {
      angle--;
      if(angle<=0) {angle=0;servo_way = true;}
    }
    myServo.write(angle);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  servo_move();
  distance_cal();
}
