#pragma once

#include "motor_regulator.h"


#define TICKS_PER_REV 372
#define WHEEL_D 67
#define WHEEL_BASE 175
long rStart_ticks;
long lStart_ticks;
extern Regulator left_regulator;
extern Regulator right_regulator;

//Команды с ROS ноды
enum Commands : uint8_t {
  SET_MOTORS = 0x10,
  GET_DATA = 0x11,
  TURN_ROBOT = 0x12,
};

struct Data {
  int16_t left_encoder_delta;
  int16_t right_encoder_delta;
};

template <typename T> void serial_read(T& dest) {
  wait_bytes(sizeof(dest));
  Serial.readBytes((uint8_t* ) &dest, sizeof(T));
}

template <typename T> void serial_send(T& src) {
  Serial.write((uint8_t *) &src, sizeof(T));
}

void wait_bytes(uint8_t b) {
  while (Serial.available() < b) {
    ; // Ожидание прибытия всего пакета
  }
}

void set_motors(){
  wait_bytes(sizeof(int8_t) + sizeof(int8_t));

  int8_t left_motor_speed = Serial.read();
  int8_t right_motor_speed = Serial.read();
  left_regulator.set_delta(left_motor_speed);
  right_regulator.set_delta(right_motor_speed);
}

void get_data(){
  wait_bytes(sizeof(int8_t));
  left_regulator.encoder.calc_delta();
  right_regulator.encoder.calc_delta();
  Data ret{left_regulator.encoder.speed, right_regulator.encoder.speed};
  Serial.write((uint8_t*)&ret, sizeof(ret));  
}

int turnAngle() {
  wait_bytes(sizeof(int8_t) + sizeof(int8_t));
  int8_t angle = Serial.read();
  int8_t speed = Serial.read();
  
  float angle_rad = angle * (PI / 180);
  float arc_distance = (WHEEL_BASE * angle_rad) / 2;
  float ticks = (arc_distance * TICKS_PER_REV) / (WHEEL_D * PI);
  if (angle > 0) {
    left_regulator.set_delta(speed);
    right_regulator.set_delta(-speed);
  }else {
    left_regulator.set_delta(-speed);
    right_regulator.set_delta(speed);
  }
  rStart_ticks = right_regulator.encoder.ticks;
  lStart_ticks = left_regulator.encoder.ticks;
  return (int)ticks;
}

int ticks = 0;
bool turn_flag = 0;
void command_spin(){
    //Обработчик комманд
  if (Serial.available() > 1) {
    uint8_t code = Serial.read();
    switch (code) {
    case SET_MOTORS: set_motors(); break;
    case GET_DATA: get_data(); break; 
    case TURN_ROBOT: ticks = turnAngle(); turn_flag=1; break;
    }
  }
  if ((abs(left_regulator.encoder.ticks - lStart_ticks) >= abs(ticks)) and (turn_flag)) {
    for (int i = 0,  s = 255; i <= 15; i++, s = -s){
      left_regulator.motor.set_pwmdir(s);
      right_regulator.motor.set_pwmdir(s);
      delay(5);
    }
    left_regulator.set_delta(0);
    right_regulator.set_delta(0);
    left_regulator.motor.set_pwmdir(0);
    right_regulator.motor.set_pwmdir(0);
    Serial.write(1);
    turn_flag = 0;
  }
}
