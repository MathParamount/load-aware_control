#include <Arduino.h>
#include <WiFi.h>
#include <TimeLib.h>

//Out pin to capacitors
#define ADC_PIN 34

// SSRs' pins
#define SSR_LAMP 25
#define SSR_MOTOR 26


#define ADC_Max 4095.0
#define volt_ref 3.3    // reference voltage
#define current_ref 5   //reference current number at datasheet
#define sensor_sensitivity 0.185      //specification of an ACS712-5B

#define INRUSH_LIMIT 1.5    //The motor maximum current

#define STABLE_THRESH 0.05  //10% of variation
#define STABLE_TIME_MS 300  //300 ms in stable condition

float adc_voltage = 0;
float aparent_potency = 0;

float offset_new = 0;
float peak_time = 0;
float peak_current = 0;

time_t start_time = 0;
float current_inst = 0;
float rms_current = 0;

float steady_current = 0.0;

//Motor inrush dectection
bool meas_inrush;
unsigned long inrush_start_time = 0;
unsigned long motor_start_time = 0;
unsigned long dt = 0;

//Used in permanent regime
bool stabilized = false;
float error = 0;
float last_rms = 0;

//FSM commands
bool cmd_lamp_on = false;
bool cmd_motor_on = false;
bool cmd_off = false;
bool cmd_reset = false;

//ALL possible state
enum StateMachine
{
  IDLE,
  LAMP_ON,
  MOTOR_STARTING,
  MOTOR_ON
};

StateMachine state = IDLE;

//StateMachine state = IDLE;

void setup() 
{
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  WiFi.mode(WIFI_OFF);

  //SSRs pin mode
  pinMode(SSR_LAMP, OUTPUT);
  pinMode(SSR_MOTOR, OUTPUT);

  digitalWrite(SSR_LAMP,LOW);
  digitalWrite(SSR_MOTOR,LOW);

  int adj_long = 3000;
  int sensor = 0;

  //Measuring the offset value
  for (int i = 0; i < adj_long; i++) {
    int adc = analogRead(ADC_PIN);

    sensor += adc;
  }

  offset_new = sensor / (float)adj_long;

  delayMicroseconds(1000);
}

void clear_command()
{
  cmd_lamp_on = false;
  cmd_motor_on = false;
  cmd_off = false;
  cmd_reset = false;
}

//System action definitions
void turn_on_motor()
{
  digitalWrite(SSR_MOTOR,HIGH);

  peak_current = 0;
  meas_inrush = true;
  inrush_start_time = micros();

  motor_start_time = millis();
  state = MOTOR_STARTING;
}

void turn_off_motor()
{
  digitalWrite(SSR_MOTOR,LOW);
}

void turn_on_lamp()
{
  digitalWrite(SSR_LAMP, HIGH);
}

void turn_off_lamp()
{
  digitalWrite(SSR_LAMP, LOW);
}

//System states
void state_machine()
{
  Serial.print("FSM running | state = ");
  Serial.println(state);

  switch (state)
  {
    case IDLE:
      turn_off_motor();
      turn_off_lamp();

      if(cmd_lamp_on)
      {
        turn_on_lamp();
        state = LAMP_ON;
      }

      else if(cmd_motor_on)
      {
        motor_start_time = millis();
        state = MOTOR_STARTING;
      }
      
      break;
    
    case LAMP_ON:
      
      turn_on_lamp();

      if(cmd_motor_on)
      {
        turn_off_lamp();
        turn_on_motor();
        motor_start_time = millis();
        state = MOTOR_STARTING; 
      }

      else if(cmd_off)
      {
          turn_off_lamp();
          state = IDLE;
      }
      break;

    case MOTOR_STARTING:

        if(peak_current > INRUSH_LIMIT)
        {
          turn_off_motor();
          state = IDLE;
        }
        else if(stabilized)
        {
          state = MOTOR_ON;
        }

      break;

    case MOTOR_ON:

      if(cmd_lamp_on)
      {
        turn_off_motor();
        turn_on_lamp();
        state = LAMP_ON;
      }

      else if (cmd_off) {
        state = IDLE;
        turn_off_motor();
      }

      break;
  }

  clear_command();
}

//This function below shouldn't exist 
/*void motor_on()
{
  if( state == MOTOR_STARTING)
  {
    if (dt > 200 && dt < 15000 && peak_current > INRUSH_LIMIT)
    {
      digitalWrite(SSR_MOTOR, LOW);
    }

    if( millis() - motor_start_time > 500)
    {
      state = MOTOR_ON;
    }
  }
}
*/


float read_current ()
{
  int adc = analogRead(ADC_PIN);

  adc_voltage = volt_ref / ADC_Max;

  float voltage = (adc - offset_new) * adc_voltage;

  current_inst = voltage / sensor_sensitivity;

  return current_inst;
}

void update_inrush()
{
  if(!meas_inrush) return;

  unsigned long dt_inrs = micros() - inrush_start_time;

  if (dt_inrs > 300 && dt_inrs < 15000)
  {
    float i = abs(current_inst);
      
      if( i > peak_current)
      {
        peak_current = i;
      }
  }

  //the system wait 15 micro seconds
  if (dt_inrs >= 15000) {
    meas_inrush = false;

    Serial.print("Inrush peak detected: ");
    Serial.println(peak_current);
  }
}

void update_rms()
{
  static float acc_sq = 0;
  static float sample_count = 0;

  static int RMS_SAMPLES = 500;   // ~10 ms @ 50 Hz

  acc_sq += current_inst * current_inst;
  sample_count++;

  if (sample_count >= RMS_SAMPLES) {
    rms_current = sqrt(acc_sq / sample_count);
    acc_sq = 0;
    sample_count = 0;
  }
}

//permanent regime
bool perm_regime()
{
  static float last = 0;
  static unsigned long t0 = 0;

  float error;

  if (last > 0.01)
    error = abs(rms_current - last) / last;
  else
    error = 1.0;

  if (error < STABLE_THRESH)
  {
    if (t0 == 0) t0 = millis();
    if (millis() - t0 > STABLE_TIME_MS)
      return true;
  }
  else
  {
    t0 = 0;
  }

  last = rms_current;
  return false;
}


// The functions

void loop() 
{
  /*
  read_current();

  update_inrush();
  update_rms();
  
  //Look into the peak_current and inrush current
  Serial.print("meas=");
  Serial.print(meas_inrush);
  Serial.print(" peak=");
  Serial.println(peak_current);

  stabilized = perm_regime();
  //state_machine();

*/

  //test
  digitalWrite(SSR_LAMP, HIGH);
  delay(2000);
  digitalWrite(SSR_LAMP, LOW);
  delay(2000);
}
