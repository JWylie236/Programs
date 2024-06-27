#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal.h>


// These 4 varibales are the only variables you need to change to adjust the fan curve. The rest of the code below doesnt need to be worried about.
uint8_t Fan_Min_Speed = 30; // Sets the Minimum speed the Fan can operate at in Percent.
uint8_t Fan_max_Speed = 75; // Sets the maximum Speed the fan can reach in Percent.
uint16_t Fan_On_Min_Temp = 30; // Sets the minimum Temperature needed to Turn the Cooling Fan on. 
uint16_t Fan_Max_Speed_Temp = 60; // Sets the Temperature at which the Fan will be turned up to its max speed in percentage as set by Fan_max_Speed.


// 2004 Smartcontroller display Pins on Arduino___________________________________________________________
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Thermistor Pins + Fan Pin________________________________________________________________

// Each Thermistor must be connected to its respective Pin through a jumper placed between the Thermistor and the 4.7k Resistor as shown in the Thermistor Wiring Diagram.
// Example: The Thermistor labelled "CPU" needs to be connected to the CPU_GPU_Temp_Pin.

const uint8_t CPU_GPU_Temp_Pin = A0;
const uint8_t Memory_Temp_Pin = A1;
const uint8_t Strg_Temp_Pin = A2;
const uint8_t PSU_Temp_Pin = A3;
const uint8_t Amb_Temp_Pin = A4;
const uint8_t Fan_Pin = 6;

// DO NOT CHANGE ANY OF THE CODE FROM THIS POINT FORWARD UNLESS YOU KNOW WHAT YOU ARE DOING.

// Thermistor Temp/resistance Arrays
//________________________________________________________________________________________________________
const int temptable_5_ADC[] {17, 20, 23, 27, 31,37,43,51,61,73,87,106,128,155,189,230,278,336,402,476,554,635,713,784,846,897,937,966,1000,1010};

const int temptable_5_Temps[] {300 ,290 ,280 ,270 ,260 ,250 ,240 ,230 ,220 ,210 ,200 ,190 ,180 ,170 ,160 ,150 ,140 ,130 ,120 ,110 ,100 ,90 ,80 ,70 ,60 ,50 ,40 ,30 ,20 ,10 , 0};

// End of Thermistor Resistance arrays._____________________________________________________________________

const uint8_t CPU_Display_Row = 4;
const uint8_t CPU_Collumn = 1;
const uint8_t Memory_Display_Row = 14;
const uint8_t Memory_Collumn = 1;
const uint8_t Strg_Display_Row = 4;
const uint8_t Strg_Collumn = 2;
const uint8_t PSU_Display_Row = 14;
const uint8_t PSU_Collumn = 2;
const uint8_t Amb_Display_Row = 4;
const uint8_t Amb_Collumn = 3;
const uint8_t Fan_Row = 14;
const uint8_t Fan_Collumn = 3;

uint16_t CPU_GPU_Temp_Pin_Val = 0;
uint16_t Memory_Temp_Pin_Val = 0;
uint16_t Strg_Temp_Pin_Val = 0;
uint16_t PSU_Temp_Pin_Val = 0;
uint16_t Amb_Temp_Pin_Val = 0;

const uint8_t CPU_GPU_Temp = 0;
const uint8_t Memory_Temp = 1;
const uint8_t Strg_Temp = 2;
const uint8_t PSU_Temp = 3;
const uint8_t Amb_Temp = 4;
uint8_t Highest_Temp = 0;

uint8_t Temp_Sens_Pins_Array[] {CPU_GPU_Temp_Pin, Memory_Temp_Pin, Strg_Temp_Pin, PSU_Temp_Pin, Amb_Temp_Pin};
uint16_t Temp_Sens_Result_Array[] {CPU_GPU_Temp_Pin_Val, Memory_Temp_Pin_Val, Strg_Temp_Pin_Val, PSU_Temp_Pin_Val, Amb_Temp_Pin_Val};
float Temp_Array[5];

uint8_t Display_Row_Array[] {CPU_Display_Row, Memory_Display_Row, Strg_Display_Row, PSU_Display_Row, Amb_Display_Row};
uint8_t Display_Collumn_Array [] {CPU_Collumn, Memory_Collumn, Strg_Collumn, PSU_Collumn, Amb_Collumn};


uint8_t Fan_Speed_Range = Fan_max_Speed - Fan_Min_Speed;
uint8_t Fan_Temp_Range = Fan_Max_Speed_Temp - Fan_On_Min_Temp;
float Fan_Step_Per_Degree = ((float) Fan_Speed_Range / Fan_Temp_Range);
float Fan_Speed = 0;
uint8_t Fan_PWM = 0;
const uint8_t PWM_Max = 255;
float Fan_Speed_Add = 0;
uint8_t Fan_Speed_Percentage = 0;

uint8_t Temp_Table_Index = 0;
uint8_t Sens_Pin_Index = 0;
uint8_t Display_Array_Index = 0;
const uint8_t Num_Sens = 5;

uint16_t Temp_Table_Val = 0;
uint16_t Prev_Temp_Table_Val = 0;

bool Read_Temp_Sens_Flg = 0;
bool Read_Current_Sens_Flg = 0;
bool Temptable_Flg = 0;
bool ADC_Table_Set_Flg = 0;
bool Update_Temp_Flg = 0;
bool Update_Display_Flg = 0;
bool Clear_Display_Flg = 0;

float Temp = 0;
const uint16_t Max_Temp = 300;
float Base_Temp = 0;
float Temp_Increment = 10;
float Temp_Calc_ADC = 0;
float Add = 0;
const uint16_t Max_ADC_Val = 1024;
uint16_t Pin_ADC_Val = 0;
uint16_t ADC_Val_Difference = 0;
float ADC_Pin_Left_Over = 0;
unsigned long Temp_Update_Interval = 250000;
unsigned long Display_Update_Interval = 1500000;
unsigned long Prev_Temp_Update_Time = 0;
unsigned long Prev_Display_Update_Time = 0;
unsigned long Current_Time = 0;

void Display_Setup() 
{
  lcd.setCursor(4, 0);
  lcd.print("TEMPS IN C");
  lcd.setCursor(0, 1);
  lcd.print("CPU 0.0");
  lcd.setCursor(10, 1);
  lcd.print("MEM 0.0");
  lcd.setCursor(0, 2);
  lcd.print("STR 0.0");
  lcd.setCursor(10, 2);
  lcd.print("PSU 0.0");
  lcd.setCursor(0, 3);
  lcd.print("AMB 0.0");
  lcd.setCursor(10, 3);
  lcd.print("FAN 0%");
}

void Read_Temp_Sensors() 
{
  // Temp calculation formula R = 10K / (1023/ADC - 1) 
    if (Read_Current_Sens_Flg <= 0)
    {
      Temp_Sens_Result_Array[Sens_Pin_Index] = analogRead(Temp_Sens_Pins_Array[Sens_Pin_Index]);
      Pin_ADC_Val = Max_ADC_Val - Temp_Sens_Result_Array[Sens_Pin_Index];
      //Read_Current_Sens_Flg = 1;
    }

    if (ADC_Table_Set_Flg <= 0)
    {
      if (Pin_ADC_Val > Temp_Table_Val)
    {
      Prev_Temp_Table_Val = Temp_Table_Val;
      Temp_Table_Val = temptable_5_ADC[Temp_Table_Index++];
    }
    
    if (Temp_Table_Val > Pin_ADC_Val)
    {
      ADC_Table_Set_Flg = 1;
    }
    }

      if (ADC_Table_Set_Flg >= 1)
  {
    ADC_Val_Difference = Temp_Table_Val - Prev_Temp_Table_Val;
    Base_Temp = temptable_5_Temps[Temp_Table_Index];
    ADC_Pin_Left_Over = Temp_Table_Val - Pin_ADC_Val;
    Temp_Calc_ADC = (ADC_Pin_Left_Over / ADC_Val_Difference) * 100;
    Add = (Temp_Increment / 100) * Temp_Calc_ADC;
    Temp_Array[Sens_Pin_Index] = (Base_Temp + Add) + 10;
    if (Temp_Sens_Result_Array[Sens_Pin_Index] <= 0)
    {
      Temp_Array[Sens_Pin_Index] = 0;
    }
    
    Temp_Table_Val = 0;
    Prev_Temp_Table_Val = 0;
    Temp_Table_Index = 0;
    ADC_Table_Set_Flg = 0;
    //Read_Current_Sens_Flg = 0;
    Sens_Pin_Index++;
    }
}

void Set_Fan_Speed()
{

  if (Temp_Array[CPU_GPU_Temp] > Temp_Array[Memory_Temp] && Temp_Array[CPU_GPU_Temp] > Temp_Array[Strg_Temp] && Temp_Array[CPU_GPU_Temp] > Temp_Array[PSU_Temp])
  {
    Highest_Temp = Temp_Array[CPU_GPU_Temp];
  }
  
  if (Temp_Array[Memory_Temp] > Temp_Array[CPU_GPU_Temp] && Temp_Array[Memory_Temp] > Temp_Array[Strg_Temp] && Temp_Array[Memory_Temp] > Temp_Array[PSU_Temp])
  {
    Highest_Temp = Temp_Array[Memory_Temp];
  }
  
  if (Temp_Array[Strg_Temp] > Temp_Array[Memory_Temp] && Temp_Array[Strg_Temp] > Temp_Array[CPU_GPU_Temp] && Temp_Array[Strg_Temp] > Temp_Array[PSU_Temp])
  {
    Highest_Temp = Temp_Array[Strg_Temp];
  }

  if (Temp_Array[PSU_Temp] > Temp_Array[Memory_Temp] && Temp_Array[PSU_Temp] > Temp_Array[Strg_Temp] && Temp_Array[PSU_Temp] > Temp_Array[CPU_GPU_Temp])
  {
    Highest_Temp = Temp_Array[PSU_Temp];
  }

  if (Highest_Temp > Fan_On_Min_Temp)
  {
    if (Highest_Temp < Fan_Max_Speed_Temp)
    {
      Fan_Speed_Add = (Highest_Temp - Fan_On_Min_Temp) * Fan_Step_Per_Degree;
      Fan_Speed = Fan_Min_Speed + Fan_Speed_Add;
      Fan_Speed_Percentage = Fan_Speed;
      Fan_PWM = (PWM_Max / 100u) * Fan_Speed;
      analogWrite(Fan_Pin,Fan_PWM);
    }
    
    if (Highest_Temp > Fan_Max_Speed_Temp)
    {
      Fan_PWM = (PWM_Max / 100u) * Fan_max_Speed;
      analogWrite(Fan_Pin,Fan_PWM);
    }
  }

  if (Highest_Temp < Fan_On_Min_Temp)
  {
    Fan_Speed_Percentage = 0;
    Fan_PWM = 0;
    analogWrite(Fan_Pin,Fan_PWM);
  }
  
}

void Update_Display()
{
  if (Clear_Display_Flg >= 1)
  {

    for (Display_Array_Index = 0; Display_Array_Index < Num_Sens; Display_Array_Index++)
    {
      lcd.setCursor(Display_Row_Array[Display_Array_Index], Display_Collumn_Array[Display_Array_Index]);
      lcd.print("     ");
    }

  if (Display_Array_Index >= Num_Sens)
  {
    lcd.setCursor(Fan_Row,Fan_Collumn);
    lcd.print("     ");
    Clear_Display_Flg = 0;
    Display_Array_Index = 0;
  }

  }
  
  if (Clear_Display_Flg <= 0)
  {
    for (Display_Array_Index = 0; Display_Array_Index < Num_Sens; Display_Array_Index++)
    {
      lcd.setCursor(Display_Row_Array[Display_Array_Index], Display_Collumn_Array[Display_Array_Index]);
      lcd.print(Temp_Array[Display_Array_Index],1);
    }
    
  if (Display_Array_Index >= Num_Sens)
  {
    lcd.setCursor(Fan_Row,Fan_Collumn);
    lcd.print(Fan_Speed_Percentage);
    lcd.print("%");
  }
  
  if (Display_Array_Index >= Num_Sens && Clear_Display_Flg <= 0)
    {
      Update_Display_Flg = 0;
      Display_Array_Index = 0;
    }
  }
}

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  // Print a message to the LCD.
  Display_Setup();
  Serial.begin(9600);
  pinMode(CPU_GPU_Temp_Pin,INPUT);
  pinMode(Memory_Temp_Pin,INPUT);
  pinMode(Strg_Temp_Pin,INPUT);
  pinMode(PSU_Temp_Pin,INPUT);
  pinMode(Amb_Temp_Pin,INPUT);
  pinMode(Fan_Pin,OUTPUT);

}

void loop() {
  Current_Time = micros();
  if (Current_Time - Prev_Temp_Update_Time >= Temp_Update_Interval && Update_Temp_Flg <= 0)
  {
    Update_Temp_Flg = 1;
    Prev_Temp_Update_Time = Current_Time;
  }

  if (Current_Time - Prev_Display_Update_Time > Display_Update_Interval && Update_Display_Flg <= 0)
  {
    Update_Display_Flg = 1;
    Clear_Display_Flg = 1;
    Prev_Display_Update_Time = Current_Time;
  }

  if (Update_Temp_Flg >= 1)
  {
    Read_Temp_Sensors();

    if (Current_Time >= 4250000000u)
    {
      Current_Time = 0;
    }

    if (Sens_Pin_Index >= Num_Sens)
    {
      Set_Fan_Speed();
      Update_Temp_Flg = 0;
      Sens_Pin_Index = 0;
    }
    
  }

  if (Update_Display_Flg >= 1)
  {
    Update_Display();
  }
}