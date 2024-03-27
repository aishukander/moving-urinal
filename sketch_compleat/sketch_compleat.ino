//導入函式庫
#include <Stepper.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <TridentTD_LineNotify.h>

//定義接腳與所需參數
const String restroom = "現代大樓右側三樓半";
const String number = "5號";
const char* ssid     =  "s23u";
const char* password =  "ttest113";
const int stepsPerRevolution = 2048;
int led_pin = 2;
int TRIG_PIN = 4;
int ECHO_PIN = 16;
int PLAYE = 23;
int water_sensor=36;
int msg = 0;
long duration, cm;
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17
#define LINE_TOKEN "OhciMy4vl44Q8DfMhZXEcsJ1pEeQFjIKy52MOyA44nI"

WiFiClient client; //宣告網路連接物件
TaskHandle_t Task1;  //宣告任務Task1

Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);  //初始化步進馬達

void setup() {
  Serial.begin(115200);  //設定鮑率
  myStepper.setSpeed(10); //設定步進馬達速度
  //設定腳位狀態
  pinMode(water_sensor,INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PLAYE, OUTPUT);
  pinMode(led_pin, OUTPUT);
  //初始化網路
  Network();

  //初始化多核心使用
  xTaskCreatePinnedToCore(
    Task1_senddata, /*任務實際對應的Function*/
    "Task1",        /*任務名稱*/
    10000,          /*堆疊空間*/
    NULL,           /*無輸入值*/
    0,                 /*優先序0*/
    &Task1,       /*對應的任務變數位址*/
    0);                /*指定在核心0執行 */
}

//第一主迴圈
void loop() {
  Ultrasound();
  if (cm < 100) {
    Program();
  }
}

//第二主迴圈
void Task1_senddata(void * pvParameters ){
  for (;;) {
    int val=analogRead(water_sensor);
    if(val > 800){  //當水位大於800時通過line通報並且開始閃爍esp32本身的LED
      water_high();
    }
    //如果故障處理完成就讓下次水位過量時可使用line通報
    if(val < 800 && msg ==1){
      msg = 0;
    }
    delay(1);
  }
}

void Program() {
  //重複三次要求使用者靠近
  for(int i = 0; i < 3 && cm >= 40 && cm <= 50; i++){      
    digitalWrite(PLAYE, HIGH);
    delay(500);
    digitalWrite(PLAYE, LOW);      
    delay(1000);
    Ultrasound();
  }
  delay(100);
  //三次後未靠近則將小便斗前移
  if (cm >= 40 && cm <= 50) {
    advance();
    Ultrasound();
    if (cm > 60) {  //如距離現已大於50cm則移回小便斗
      retreat();
    } else {
      while (cm <= 60) {  // 等待距離大於50cm就移回小便斗
        delay(100);
        Ultrasound();
      }
      retreat();
    }
  }
}

//小便斗前推函式
void advance() {
  Serial.println("靠近使用者");
  myStepper.step(stepsPerRevolution);
}

//小便斗後移函式
void retreat() {
  Serial.println("離開使用者");
  myStepper.step(-stepsPerRevolution);
}

//距離感測函式
void Ultrasound(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  cm = (duration / 2) / 29.1;  //將獲取之距離單位轉換為cm
  Serial.print("距離 :");
  Serial.println(cm);
}

//網路初始化函式
void Network(){
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.print("已連接至 " + String(ssid));
  LINE.setToken(LINE_TOKEN);
}

//高水位反應函式
void water_high(){
  if(msg == 0){  //假設此次故障還未通報就使用line通報
    LINE.notify(restroom + " 內的 " + number + " 小便斗出現故障");
    msg = 1;
    Serial.println("已通報");
  }
  LEDFLASH();
}

//LED閃爍函式
void LEDFLASH(){
  digitalWrite(led_pin, HIGH);
  delay(500);
  digitalWrite(led_pin, LOW);
  delay(500);
}
