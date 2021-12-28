#include <SoftwareSerial.h>
#include<LiquidCrystal.h>
#include <stdlib.h>
#define DEBUG true

//LED
int ledPin = 13;

//LM35 analog input
int lm35Pin = A0;

//Replace with your channel's thingspeak API key
String apiKey = "1YB61UH5YWZ9BS76";

//Connect 2 to TX of Serial USB
//Connect 3 to RX of Serial USB

SoftwareSerial ser (2, 3); //RX, TX
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

static byte temprCount = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode (ledPin, OUTPUT);
  lcd.begin(16, 2);
  Serial.println("Chopu");

  //Enable debug serial
  Serial.begin(9600);
  //Enable Software Serial
  ser.begin(9600); //9600

  //Reset ESP8266
//  ser.println("AT+RST");

  sendData("AT+RST\r\n", 5000, DEBUG); //Reset Module

  sendData("AT+CWMODE = 3\r\n", 1000, DEBUG); //Configure as access point as well as station
  sendData("AT+CWJAP =\"JioFi\",\"yaadnahi\"\r\n", 5000, DEBUG); //Connects your access point

  delay(3000);
  sendData("AT+CIFSR\r\n", 3000, DEBUG); //Get IP Address
  delay(1000);
  sendData("AT+CIPMUX = 0\r\n", 2000, DEBUG); //Configure for multiple connections
  delay(2000);

  //sendData("AT+CIPSERVER = 1, 80\r\n", 1000, DEBUG); //Turn on server on port 80
}


static byte ledControl = 0;
#define MAX_COUNT 1

void loop() {
  // put your main code here, to run repeatedly:

  lcd.home();

  if(temprCount < MAX_COUNT ){
    //Blink LED on Board
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
  
    //Read values from LM35
    //Read 10 values for averaging
    int val = 0;
    for (int i = 0; i < 10; i++) {
      val = val +analogRead(lm35Pin);
      delay(500);
    }
  
    //Convert to temp:
    //Temp value is in 0-1023 range
    //LM35 outputs 10mV/degree C. i.e 1 Volt => 100 degrees C
    //Sp Temp = (avg_value/1023)*5Volts*100degress/Volt
    float temp = val*50.0f/1023.0f;
  
    //Converty to string
    //char bug[16];
    //String strTemp = dtostrf(temp, 4, 1, buf);
    String strTemp = String(temp, 1);
    Serial.println(strTemp);

    lcd.display();
    lcd.print("Temp: ");
    lcd.setCursor(6, 0);
    lcd.print(strTemp);
  
    //TCP Connection
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += "184.106.153.149"; //api.thingspeak.com
    //cmd += "api.thingspeak.com";
    cmd += "\",80";
    ser.println(cmd);
  
    if(ser.find("Error")) {
      Serial.println("AT+CIPSTART error");
      return;
    }
  
    //prepare GET String
    String getStr = "GET /update?api_key=";
    getStr += apiKey;
    getStr += "&field1=";
    getStr += String(strTemp);
    getStr += "\r\n\r\n";
  
    //Send data length
    cmd = "AT+CIPSEND=";
    cmd += String(getStr.length());
    ser.println(cmd);
  
    if(ser.find(">")) {
     ser.print(getStr);
    }
    else {
      ser.println("AT+CIPCLOSE");
      //Alert user
      Serial.println("AT+CIPCLOSE");
    }
  
    //Thingspeak needs 15 seconds delay between updates
    delay(16000);

    temprCount ++;

    lcd.noDisplay();

    delay(500);
    //Turn on the display
  
    lcd.display();
    delay(500);
    lcd.print("5");
  }

  if(temprCount == MAX_COUNT)
  {
   Serial.println("Configure ESP as receiver");
   temprCount ++; 
   delay(16000);
  }

  if(temprCount > MAX_COUNT)
  { 
    if(Serial.available()){
      char ch = Serial.read();
      Serial.println(ch);
      if( isAlpha(ch))
      {
        if((ch == 'Y' || ch == 'y' ) && ledControl == 0){
          ledControl = 1;
          Setup_LED();
          delay(1000);
         
        }        
      }
    }
    else if(ledControl == 1){
      Loop_LED();   
    }
  }
}

void Setup_LED() {
  Serial.begin(9600);
  ser.begin(9600);
  delay(3000);
  Serial.println("Ready");
  
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);
  
  pinMode(4,OUTPUT);
  digitalWrite(4,LOW);
  
//  sendData("AT+RST\r\n",2000,DEBUG);
//  delay(3000);
//  
//  //sendData("AT+CWMODE CUR=3\r\n",1000,DEBUG);
//  sendData("AT+CWJAP=\"JioFi\",\"yaadnahi\"\r\n",5000,DEBUG);
  delay(3000);
  sendData("AT+CIFSR\r\n",3000,DEBUG);
  delay(3000);
  sendData("AT+CIPMUX=1\r\n",2000,DEBUG);
  sendData("AT+CIPSERVER=1,80\r\n",3000,DEBUG);

}

void Loop_LED() {
  if(ser.available()){
    
    lcd.setCursor(0, 1);
    lcd.print("Speed: ");
    lcd.setCursor(7, 1);
    
    if(ser.find("+IPD,")){
      delay(1000);
      int connectionId = ser.read() - 48;
      ser.find("pin=");
  
      int pinNumber = (ser.read()-48)*10;
      pinNumber+=(ser.read()-48);
  //    digitalWrite(pinNumber,!digitalRead(pinNumber));
      if (pinNumber == 11) {
        analogWrite(5, 255);
        digitalWrite(4, LOW);
        lcd.print("HIGH");
      }
  
      if (pinNumber == 12) {
        analogWrite(5, 180);
        digitalWrite(4, LOW);
        lcd.print("LOW ");
      }
  
      if (pinNumber == 13) {
        analogWrite(5, 0);
        digitalWrite(4, LOW);
        lcd.print("STOP");
      }
  
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand+=connectionId;
      closeCommand+="\r\n";
  
      sendData(closeCommand,100,DEBUG);
    }
  }
}

String sendData (String command, const int timeout, boolean debug)
  {
    String response = "";
    ser.print(command); //Send the read character to the ESP8266
    long int time = millis();

    while((time + timeout) > millis())
    {
      while(ser.available())
      {
        //The ESP has data to display its output to the serial window
        char c = ser.read(); //read the next character.
        response += c;
      }
    }

    if (debug)
    {
      Serial.print(response);
    }
    return response;
  }



