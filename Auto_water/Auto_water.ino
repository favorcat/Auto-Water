//=========================================================
// 자동 물공급장치
// 물통에 설치된 모터펌프를 가동하여 화분에 자동으로 물주는 시스템
// File name: Auto_water.ino
// Author   : 은영
// Date     : 2019.06.30
//=========================================================

// 사용보드 아두이노 Maga2560
// 모터드라이브 L298N
// LCD : I2C LCD (16*2) 모듈 사용을 위한  => LiquidCrystal_I2C 라이브러리 설치
//
// PWM핀  D0 ~ D13
// 핀20 (SDA)
// 핀 21 (SCL)

//LCD 라이브러리 사용하기
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display


// pin 번호 선언
int pinSw[2]     = {2, 3}; //스위치1,2 Pin
int pinTank      = 4;     //수위센서 Pin
int pinTank_led  = 5;     //저수위경보 LED
int pinTemp      = 6;     //온도/습도센서 Pin
int pinMoter[2][3] = {{7, 8, 9}, {10, 11, 12}}; //모터 A,B  (HIGH, LOW, PWM)

int R_time[2] = {0, 0};   //모터 남은 동작시간 (초)
int R_sec = 5;            //모터 동작시간 최소 5초       (사용자가 가변저항으로 변경가능 5~20초)
int M_speed = 250;        //모터 동작속도 기본 255(최대) (사용자가 가변저항으로 변경가능 100~250)

int pin_led1  = 14;         //화분1-LED
int pin_led2  = 15;         //화분2-LED

int pin_A0  = A0;       //토양수분센서1 Pin
int pin_A1  = A1;       //토양수분센서2 Pin
int pin_A2  = A2;       //조도센서 Pin
int pin_A3  = A3;       //모터 동작시간 가변 Pin
int pin_A4  = A4;       //모터 스피드 가변 Pin

// 전역 변수 선언 ///////////////////////////////
int r0, rr0 = 0;    //수분센서1 값 저장 변수
int r1, rr1 = 0;    //수분센서2 값 저장 변수
int r2, rr2 = 0;    //조도센서  값 저장 변수
int r3, rr3 = 0;    //모터 동작시간 가변 변수
int r4, rr4 = 0;    //모터 스피드 가변 변수
int cds = 0;        //조도센서값
int Sw[2];          //스위치1 상태 저장 변수
int Tank = 0;       //물탱크 센서 Low water level 0:저수위경보
int Low_soil = 50;  //Soil low water  화분 토양수분 최저경보
int Max_soil = 70;  //Soil Max water  화분 토양수분 수분수치가 70이상이면 물공급중지

int mc = 1;         //메세지 출력 위치 1~5
int mc_old = 0;     // 메세지를 첫번째줄에 출력을 위해 2번째줄에 출력한위치 보관


unsigned long _time1 = 0;      // 매 0.2 초마다 모터 동작버튼 읽기
unsigned long _time2 = 0;      // 매 1 초마다 센서값 읽기
unsigned long _time3 = 0;      // 매 1 초마다 relay 동작시간 체크 및 시간 1초 감소
unsigned long _time4 = 0;      // 매 2 초마다 LCD에 센서값 출력
unsigned long _time5 = 0;      // 매 1 초마다 시리얼 모니터에 출력하기
unsigned long autotime1 = 0;    // 화분1 자동으로 물 주는 시간
unsigned long autotime2 = 0;    // 화분2 자동으로 물 주는 시간
unsigned long autochecktime = 86400000  ;   // 24시간 미다 자동물주기  3600*24*1000

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

void setup() {
  Serial.begin(9600);               // 시리얼 모니터 통신

  pinMode(pinSw[0], INPUT_PULLUP);  // 스위치1 풀업저항
  pinMode(pinSw[1], INPUT_PULLUP);  // 스위치2 풀업저항
  pinMode(pinTank, INPUT_PULLUP);    // 수위센서 스위치

  for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
      pinMode(pinMoter[i][j], OUTPUT);     // 모터 핀모드 설정
    }

  pinMode(pinTank_led, OUTPUT);       // 저수위경보 LED
  pinMode(pin_led1, OUTPUT);       // 저수위경보 LED
  pinMode(pin_led2, OUTPUT);       // 저수위경보 LED

  for (int i = 0; i < 2; i++) {
    digitalWrite(pinMoter[i][0], LOW);   // 모터 초기 LOW LOW설정
    digitalWrite(pinMoter[i][1], LOW);   // 모터 초기 LOW LOW설정
    analogWrite(pinMoter[i][2], M_speed); // 모터속도 250으로 설정 (0~255)
  }

  //LCD 초기화
  lcd.init();
  lcd.backlight();

}

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

void loop() {
  /*---------------------------------------------------------------------*/
  /*---------------------------------------------------------------------*/
  if (_time1 < millis() ) {    // 매 0.2 초마다 버튼 센서값 읽기

    _time1 = millis() + 200;  // Next time after 0.2초.

    //--------------------------------------------------------

    Sw[0] = digitalRead(pinSw[0]);
      
    if ((Sw[0] == 0) && (rr0< Max_soil))  // 동작스위치가 눌러지고 수분이 Max_soil 이하일때 모터동작
        moter(0, 0); //모터 동작버튼이 눌러지면

    Sw[1] = digitalRead(pinSw[1]);
      
    if ((Sw[1] == 0) && (rr1< Max_soil))  //
       moter(1, 0); //모터 동작버튼이 눌러지면



    //--------------------------------------------------------
  }
  /*---------------------------------------------------------------------*/
  /*---------------------------------------------------------------------*/
  if (_time2 < millis() ) {    // 매 1 초마다 센서값 읽기
    _time2 = millis() + 1000;  // Next time after 1초.

    r0 = analogRead(pin_A0);      //수분센서1
    r1 = analogRead(pin_A1);      //수분센서2
    r2 = 1023 - analogRead(pin_A2); //조도센서
    r3 = analogRead(pin_A3);      //모터 동작시간
    r4 = analogRead(pin_A4);      //모터 스피드

    rr0 = map(r0, 0, 1023, 0, 100 );  //수분센서1  0~100 %
    rr1 = map(r1, 0, 1023, 0, 100 );  //수분센서2  0~100 %
    rr2 = map(r2, 0, 1023, 0, 100 );  //조도센서  0~100 %
    rr3 = map(r3, 0, 1023, 5, 21 );   //모터 동작시간 5~20초
    rr4 = map(r4, 0, 1023, 100, 250 ); //모터 스피드 100~250

    Tank = digitalRead(pinTank);      //물탱크 센서 Low water level 0:저수위경보

    if (abs(cds - rr2) > 2) { //CDS 조도값이  3이상 변할때 적용
      lcd_print( 1, 3);  //LCD에 조도값 출력
      mc = 3;
    }
    cds = rr2;

    if (abs(M_speed - rr4) > 1) { //모터 Speed가 2이상 변할때 적용
      M_speed = rr4;
      analogWrite(pinMoter[0][2], M_speed); // 모터속도 재설정 (100~255)
      analogWrite(pinMoter[1][2], M_speed); // 모터속도 재설정 (100~255)
      lcd_print( 1, 5);  //LCD에 모터 Speed 이벤트 출력
      mc = 5;
    }

    if (abs(R_sec - rr3) > 1) {
      lcd_print( 1, 4);  //모터 동작시간이 2이상 변할때 LCD에 이벤트 출력
      mc = 4;
    }
    R_sec = rr3; //모터 동작시간 변경



    //---------- 화분 토양습도 Low_soil 이하  경보 LED ---------------------
    int led_1 = digitalRead(pin_led1);  //화분-1 토양습도 경보 LED
    if (rr0 < Low_soil) {  // 화분-1 토양습도 경보 LED 점멸
        if ((autotime1+autochecktime < millis()) || (autotime1 == 0)){
          moter(0, 1);  //자동으로 물주기
          autotime1 = millis();
        }
        
        lcd_print( 1, 16);
        if (led_1 == 0) digitalWrite(14, HIGH);
        else digitalWrite(pin_led1, LOW);
        
    } else if (led_1 == 1) digitalWrite(pin_led1, LOW); //저수위경보-1 LED Off


    int led_2 = digitalRead(pin_led2);  //화분-2 토양습도 경보 LED
    if (rr1 < Low_soil) {  // 화분-2 토양습도 경보 LED 점멸
        if ((autotime2+autochecktime < millis()) || (autotime2 == 0)){
          moter(1, 1);
          autotime2 = millis();
        }


      lcd_print( 1, 17);
      if (led_2 == 0) digitalWrite(15, HIGH);
      else digitalWrite(pin_led2, LOW);
    } else if (led_2 == 1) digitalWrite(pin_led2, LOW); //저수위경보-2 LED Off


    //---------- 저 수위경보 LED ----------------------------------------
    int Tank_led = digitalRead(pinTank_led);  //저수위경보 LED
    if (Tank == 0) { // 저수위시 LED 점멸
      lcd_print( 1, 15);
      if (Tank_led == 0) digitalWrite(pinTank_led, HIGH);
      else digitalWrite(pinTank_led, LOW);
    } else if (Tank_led == 1) //저수위경보 LED Off
      digitalWrite(pinTank_led, LOW);

  }


  /*---------------------------------------------------------------------*/
  /*---------------------------------------------------------------------*/
  if (_time3 < millis() ) {    // // 매 1 초마다 relay 동작시간 체크 및 시간 1초 감소
    _time3 = millis() + 1000;  // Next time after 1초.

    for (int i = 0; i < 2; i++) { // 매1초마다 동작시간 1초씩 감소
      if (R_time[i] > 0) R_time[i] = R_time[i] - 1; // 1초 빼기
      if (R_time[i] < 0) R_time[i] = 0;
      if (Tank == 0) R_time[i] = 0; //물탱크 저수위이면 모터중지

      int ok = digitalRead(pinMoter[i][0]); // 모터 동작상태 읽기   ok=1:동작중  0: OFF
      if (R_time[i] == 0 && ok == 1) {      // 모터가 동작중이고 정시시간이 되었을때
        analogWrite(pinMoter[i][0], LOW);   // 모터정지 명령
        Serial.print("모터 ");
        Serial.print(i + 1);
        Serial.println(" 정지 ... ");
        lcd_print( 1, 13 + i); // k=13  'Moter-1 :  Stop '
        // k=14  'Moter-2 :  Stop '
      }
    }
  }

  /*---------------------------------------------------------------------*/
  /*---------------------------------------------------------------------*/
  if (_time4 < millis() ) {    // 매 1 초마다 LCD에 센서값 출력

    _time4 = millis() + 2000;  // Next time after 2초.
    if (mc_old != 0) lcd_print( 0, mc_old); // 첫번째줄에 2번째줄에 출력한것 출력

    lcd_print( 1, mc);


  }
  /*---------------------------------------------------------------------*/
  /*---------------------------------------------------------------------*/
  if (_time5 < millis() ) {    // 매 1 초마다 시리얼 모니터에 출력하기
    _time5 = millis() + 1000;  // Next time after 1초.

    Serial.print("Tank:");   Serial.print(Tank);
    Serial.print(" 수분1:");  Serial.print(rr0);   Serial.print("% ");
    Serial.print("수분2:");   Serial.print(rr1);   Serial.print("% ");
    Serial.print("조도:");    Serial.print(rr2);   Serial.print("% ");
    Serial.print("동작:");    Serial.print(R_sec);
    Serial.print(" Speed:"); Serial.print(M_speed);
    Serial.print(" Sw=");    Serial.print(Sw[0]);     Serial.print(","); Serial.print(Sw[1]);
    Serial.print(" time=");  Serial.print(R_time[0]); Serial.print(","); Serial.print(R_time[1]);
    int r1 = digitalRead(pinMoter[0][0]);
    int r2 = digitalRead(pinMoter[1][0]);
    Serial.print(" M=");  Serial.print(r1);
    Serial.print(",");    Serial.println(r2);

  }
  /*---------------------------------------------------------------------*/
}

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/

void lcd_print(int y, int k) { //y 위치 0,1   k: 1~5, 11~17

  //       0123456789012345
  // k=1  'Soil Water-1:##%'
  // k=2  'Soil Water-2:##%'
  // k=3  'CDS Sensor  :##%'
  // k=4  'Run time : ##Sec'
  // k=5  'Moter Speed :###'

  //        0123456789012345
  // k=11  'Moter-1 : Start '
  // k=12  'Moter-2 : Start '
  // k=13  'Moter-1 :  Stop '
  // k=14  'Moter-2 :  Stop '
  // k=15  'Water tank low..'
  // k=16  'Soil-1 low water'
  // k=17  'Soil-2 low water'

  if (y == 1) mc_old = k; //2번째줄에 출력하면 다음에는 첫번째줄에 출력하기위해 보관

  if (k < 10) {
    mc = k + 1;
    if (mc > 5) mc = 1;
  }

  lcd.setCursor(0, y);
  switch (k)
  {
    case 1:
      {
        lcd.print("Soil Water-1:");
        lcd.setCursor(13, y);
        lcd.print(rr0);
        lcd.setCursor(15, y);
        lcd.print("%");
      } break;
    case 2:
      {
        lcd.print("Soil Water-2:");
        lcd.setCursor(13, y);
        lcd.print(rr1);
        lcd.setCursor(15, y);
        lcd.print("%");
      } break;
    case 3:
      { //   0123456789012345
        lcd.print("CDS Sensor  :");
        lcd.setCursor(13, y);
        lcd.print(rr2);
        lcd.setCursor(15, y);
        lcd.print("%");
      } break;
    case 4:
      { //   0123456789012345
        lcd.print("Run time : ");
        lcd.setCursor(11, y);
        lcd.print(R_sec);
        lcd.setCursor(13, y);
        lcd.print("Sec");
      } break;
    case 5:
      { //    0123456789012345
        lcd.print("Moter Speed :");
        lcd.setCursor(13, y);
        lcd.print(M_speed);
      } break;

    // 이벤트 발생시 출력 2번째줄에
    case 11: lcd.print("Moter-1 : Start "); break;
    case 12: lcd.print("Moter-2 : Start "); break;
    case 13: lcd.print("Moter-1 :  Stop "); break;
    case 14: lcd.print("Moter-2 :  Stop "); break;
    case 15: lcd.print("Water tank low.."); break;
    case 16: lcd.print("Soil-1 low water"); break;
    case 17: lcd.print("Soil-2 low water"); break;
  }

}

/*-------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------*/
void moter(int m, int a) { // m : 모터번호 , a : 1이면 자동, 0이면 수동
  int ok = digitalRead(pinMoter[m][0]);
  if ((ok != 1) && (Tank == 1)) { //물탱크에 물이 있고 모터가 동작중이 아니면   1:동작중 0:멈춤상태
    if (a==0)  {//수동동작

      
      R_time[m] = R_sec;  //모터 수동동작시간 (설정된시간  5~20초)
      int r4 = analogRead(pin_A4);             //모터 스피드확인(가변저항값)
      int moter = map(r4, 0, 1023, 100, 250 ); //모터 스피드 100~250
      analogWrite(pinMoter[m][2], moter);      // PWM조정 모터속도 (100~250) 
      
    }
    else       {//자동동작
      Serial.print("모터-"); Serial.print(m+1);  Serial.println(": 자동으로 물주기");  
      R_time[m] = 10;                    // 모터 자동동작시간 (자동 10초) 
      analogWrite(pinMoter[m][2], 200);  // PWM조정 모터속도 200
      
    }
      
    digitalWrite(pinMoter[m][0], HIGH);  // 모터동작

    lcd_print( 1, 11 + m); // k=11  'Moter-1 : Start '
    // k=12  'Moter-2 : Start '
  }

}
