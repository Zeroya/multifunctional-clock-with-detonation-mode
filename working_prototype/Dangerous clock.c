/*****************************************************
This program was produced by the
CodeWizardAVR V2.04.4a Advanced

Project : Detonator Clock
Version : 1.0
Date    : 30.03.2013
Author  : Kizim Igor

Chip type               : ATmega8
Program type            : Application
AVR Core Clock frequency: 8,000000 MHz internal RC
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*****************************************************/

#include <mega8.h>
#include <delay.h>
#include <stdlib.h>
#include "buttons.h"    

/* the I2C bus is connected to PORTD */
/* the SDA signal is bit 3 */
/* the SCL signal is bit 2 */
#asm
    .equ __i2c_port=0x12
    .equ __sda_bit=3
    .equ __scl_bit=2
#endasm

#include <i2c.h>

//DS1307 Real Time Clock functions
#include <ds1307.h>

//***********************************************************************
#define SOUND_FREQ  2600  //Резонансная частота пьезогенератора (в Гц) 

//ИНДИКАЦИЯ                    
//Номера выводов, подключенных от регистра сдвига к сегментам индикатора 
#define SEG_A  0  //Q0
#define SEG_B  1  //Q1
#define SEG_C  2  //Q2
#define SEG_D  3  //Q3
#define SEG_E  4  //Q4
#define SEG_F  5  //Q5
#define SEG_G  6  //Q6              
#define SEG_DP 7  //Q7

//Используя десятичные цифры от 0 до 9 в качестве индекса массива, легко выводить в порт нужные коды
unsigned char number[] = {
                          (1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(0<<SEG_G), //0
                          (0<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(0<<SEG_G), //1
                          (1<<SEG_A)|(1<<SEG_B)|(0<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(0<<SEG_F)|(1<<SEG_G), //2
                          (1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(1<<SEG_G), //3  
                          (0<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G), //4
                          (1<<SEG_A)|(0<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G), //5 
                          (1<<SEG_A)|(0<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(1<<SEG_G), //6
                          (1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(0<<SEG_D)|(0<<SEG_E)|(0<<SEG_F)|(0<<SEG_G), //7  
                          (1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(1<<SEG_E)|(1<<SEG_F)|(1<<SEG_G), //8
                          (1<<SEG_A)|(1<<SEG_B)|(1<<SEG_C)|(1<<SEG_D)|(0<<SEG_E)|(1<<SEG_F)|(1<<SEG_G)  //9   
                         };                                         
                                                            
//Номера выводов регистра, подключенные к индикаторам
#define NUM1 0
#define NUM2 1
#define NUM3 2
#define NUM4 3
  
#define DS1     PORTB.2   //вход данных
#define ST_CP1  PORTB.4   //вход «защёлкивающий» данные
#define SH_CP1  PORTB.5   //вход для тактовых импульсов    

#define DS2     PORTC.0   //вход данных
#define ST_CP2  PORTC.1   //вход «защёлкивающий» данные
#define SH_CP2  PORTC.2   //вход для тактовых импульсов

volatile char point, count_point; //Флаг разрешения включения точки и счётчик времени горения точки
volatile char blinking, count_blink; //Флаг разрешения мигания светодиода таймера и счётчик времени горения светодиода таймера 

int random_led;  //Переменная для сохранения псевдослучайного числа
char blink_led;  //Флаг разрешения имитации взрыва светодиодами 
//***********************************************************************
//СВЕТОДИОДЫ        
#define LED_1       PORTB.1
#define LED_2       PORTC.3
#define DET_LED     PORTC.5 // Вывод индикации включения таймера 
#define ALARM_LED   PORTD.4 // Вывод индикации включения будильника       
//***********************************************************************
//ФЛАГИ ДЛЯ УПРАВЛЕНИЯ ИНДИКАЦИЕЙ РАЗРЯДОВ  
char digit; 
#define DIGIT_1   0x01 
#define DIGIT_2   0x02 
#define DIGIT_3   0x04 
#define DIGIT_4   0x08 
//МАКРОСЫ ДЛЯ УПРАВЛЕНИЯ ИНДИКАЦИЕЙ РАЗРЯДОВ
#define DIGIT_ON(t)      digit|=(t)
#define DIGIT_OFF(t)     digit&=(~t)
#define DIGIT_IS_ON(t)   ((digit & (t)) != 0)
#define ALL_TUBE_ON      (digit|=(DIGIT_1|DIGIT_2|DIGIT_3|DIGIT_4))
#define ALL_TUBE_OFF     (digit&=~(DIGIT_1|DIGIT_2|DIGIT_3|DIGIT_4))
//***********************************************************************
//ВРЕМЯ И БУДИЛЬНИК
volatile unsigned char *digit_12, *digit_34;  //Указатели для вывода на индикатор                                            
unsigned char hour, minute, second;
unsigned char last_second;                                 
unsigned char hour_alarm, minute_alarm;   //Буферные переменные настройки будильника
eeprom unsigned char e_hour_alarm=0, e_minute_alarm=0;
unsigned char minute_timer, second_timer; //Буферные переменные настройки таймера                                   
eeprom unsigned char e_minute_timer=0, e_second_timer=10;

unsigned char alarm_on; //Переменные режима будильника 
eeprom unsigned char alarm_mode=0;       
unsigned char count;  //Счётчик для динамической индикации  
//***********************************************************************
#define ALARM PIND.0  //Кнопка 3
#define DET   PINC.4  //Кнопка 4
unsigned char switch_key; //Буферная переменная кода нажатой кнопки
unsigned char release;    //Флаг нажатия кнопки
char starting_countdown;  //Разрешение запуска обратного отсчёта
//*********************************************************************** 
#define WIRE_1   PINB.0 
#define WIRE_2   PINB.7
#define WIRE_3   PIND.5
#define WIRE_4   PIND.6 
char defuse, detonate; //Флаги обезвреживания и детонации бомбы
char defuse_bomb_flag; //Флаг укомплектованности разъёмов 4-мя проводами
//***********************************************************************    
//ПРОТОТИПЫ
void send_74HC595_seg (char byte);
void send_74HC595_dig (char byte);
void beep(unsigned int tone);
void check_wire (void);
void defuse_bomb (void);
void detonate_bomb (void);

//*******************************************************************************************************************                                   
//                              Timer1 overflow interrupt service routine
//*******************************************************************************************************************  
interrupt [TIM1_OVF] void timer1_ovf_isr(void)  //Прерывание каждые 3 мс
{        
 TCNT1H=0xFE;
 TCNT1L=0x89;        	
 
 if (count>=4) count=0;
     
  switch (count)
   { 
    case 0: if(DIGIT_IS_ON(DIGIT_1)) 
            {
             send_74HC595_dig(1<<NUM4); //Подключаю нужный индикатор
             send_74HC595_seg(number[*digit_12/10]); break;                 
            }             
    case 1: if(DIGIT_IS_ON(DIGIT_2)) 
            {
             send_74HC595_dig(1<<NUM3); 
             if(point) send_74HC595_seg((number[*digit_12%10])|(1<<SEG_DP)); //Зажечь точку
             else send_74HC595_seg(number[*digit_12%10]); break;                      
            }
    case 2: if(DIGIT_IS_ON(DIGIT_3))
            {
             send_74HC595_dig(1<<NUM2);                    
             send_74HC595_seg(number[*digit_34/10]); break;                 
            }             
    case 3: if(DIGIT_IS_ON(DIGIT_4)) 
            {
             send_74HC595_dig(1<<NUM1);
             send_74HC595_seg(number[*digit_34%10]); break;              
            }    
   } 
   count++; 
 
 //Опрос кнопок
 BUTTON_Scan();  

 //Выключение точки через определённый период времени
 if(point)  
 {
  if(++count_point==150)
  {
   point=0;  
   count_point=0;
  }  
 } 
 
 //Мигание светодиода таймера
 if(blinking)  
 {    
  if(++count_blink==50) { DET_LED^=1; count_blink=0; }      
 }
 
 //Имитация взрыва светодиодами
 if(blink_led)
 {
  if(++count_point==26)
  {
   count_point=0;
   random_led=rand();
   if(!(random_led%5)) LED_1=1; else LED_1=0;
   if(!(random_led%4)) LED_2=1; else LED_2=0;
   if(!(random_led%3)) DET_LED=1; else DET_LED=0;
   if(!(random_led%2)) ALARM_LED=1; else ALARM_LED=0;
  }
 }
 if((!blink_led) && (!starting_countdown)){ ALARM_LED=0; DET_LED=0; }    
}

//*******************************************************************************************************************  
//                          Timer1 output compare A interrupt service routine
//*******************************************************************************************************************  
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{  
 //Пониженная яркость индикации включения будильника в режиме отображения реального времени
 if((alarm_mode==1)&&(!starting_countdown))
 {
  ALARM_LED=1;
 } 
 
 if((alarm_mode==2)&&(!starting_countdown))
 {
  ALARM_LED=1; DET_LED=1;
 } 
}  

//*******************************************************************************************************************  
//                              Инициализация портов индикаторов и светодиодов
//*******************************************************************************************************************  
void IND_Init(void)  
{
 PORTB&=~((1<<1)|(1<<2)|(1<<4)|(1<<5));  
 DDRB|=((1<<1)|(1<<2)|(1<<4)|(1<<5));
 PORTC&=~((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<5));  
 DDRC|=((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<5)); 
 PORTD&=~(1<<4);
 DDRD|=(1<<4); 
} 
   
//*******************************************************************************************************************
//                                      Формирование цифры(сегментов)
//*******************************************************************************************************************  
void send_74HC595_seg (char byte)
{
 char x;
 for(x=0; x<8; x++)
 {
  if(byte&0x80) DS1=1;  
  else          DS1=0; 
  byte<<=1; 
  SH_CP1=1; 
  //#asm("nop");
  SH_CP1=0;
 }
 ST_CP1=1; ST_CP2=1;
 //#asm("nop");
 ST_CP1=0; ST_CP2=0;
 DS1=0; DS2=0;
}

//*******************************************************************************************************************  
//                                      Установка нужного разряда
//*******************************************************************************************************************  
void send_74HC595_dig (char byte)
{
 char x=0;
 for(x=0; x<8; x++)
 {
  if(byte&0x80) DS2=1;  
  else          DS2=0; 
  byte<<=1; 
  SH_CP2=1; 
  //#asm("nop");
  SH_CP2=0;
 }   
}

//*******************************************************************************************************************
//                               Режим СТС для формирования звукового сигнала
//*******************************************************************************************************************  
void beep(unsigned int tone)  
{
 if(tone==0) {TCCR2=0;}
 else {OCR2=(unsigned char)((8000000.0/(tone*2.0*128.0))-1.0); TCCR2=(1<<4)|(1<<3)|(1<<2)|(1<<0);}
}

//*******************************************************************************************************************
//                                  Обезвреживание "бомбы"
//*******************************************************************************************************************  
void check_wire (void)
{
 if(WIRE_1) { if(defuse==0) defuse_bomb(); if(detonate==0) detonate_bomb(); }  
 if(WIRE_2) { if(defuse==1) defuse_bomb(); if(detonate==1) detonate_bomb(); }
 if(WIRE_3) { if(defuse==2) defuse_bomb(); if(detonate==2) detonate_bomb(); }
 if(WIRE_4) { if(defuse==3) defuse_bomb(); if(detonate==3) detonate_bomb(); }
}

//*******************************************************************************************************************  
//                              Функция обезвреживания "бомбы"
//*******************************************************************************************************************  
void defuse_bomb (void)
{ 
 beep(0);
 blinking=0; 
 DET_LED=0;   
 delay_ms(4000); 
 starting_countdown=0;
 switch(alarm_mode)    //Обновить светодиоды будильника
 {
  case 0: ALARM_LED=0; DET_LED=0; break;  
  case 1: ALARM_LED=1; break;  
  case 2: ALARM_LED=1; DET_LED=1; break;  
 }
 defuse_bomb_flag=0;                                              
}

//*******************************************************************************************************************  
//                                  Функция детонации "бомбы"
//*******************************************************************************************************************  
void detonate_bomb (void)
{
 blinking=0; //Запрет мигания светодиода DET_LED
 DET_LED=0;
         
 blink_led=1; //Имитация светодиодами взрыва 
 beep(SOUND_FREQ); 
 ALL_TUBE_OFF; //Выключить индикаторы
 send_74HC595_dig(0x00); //Отключение всех индикаторов
 ST_CP2=1;  //Импульс "защёлкивания" регистром
 ST_CP2=0; 
 DS2=0;
         
 delay_ms(6000);           
 beep(0);
 blink_led=0; count_point=0;
 ALARM_LED=0; DET_LED=0; LED_1=0; LED_2=0;
 delay_ms(3000);
 starting_countdown=0; //Отключаем обратный отсчёт
 ALL_TUBE_ON; //Включить все индикаторы                          
        
 switch(alarm_mode)    //Обновить светодиоды будильника
 {
  case 0: ALARM_LED=0; DET_LED=0; break;  
  case 1: ALARM_LED=1; break;  
  case 2: ALARM_LED=1; DET_LED=1; break;  
 } 
 defuse_bomb_flag=0;                                                   
}

//*******************************************************************************************************************

void main(void)
{
// Declare your local variables here

// Input/Output Ports initialization
// Port B initialization
PORTB=0x00;  PORTB|=(1<<0)|(1<<7);
DDRB=0x00;   DDRB|=(1<<3); //Выход звукового сигнала

// Port C initialization
PORTC=0x00;  PORTC|=(1<<4);
DDRC=0x00;

// Port D initialization
PORTD=0x00;  PORTD|=(1<<0)|(1<<5)|(1<<6);
DDRD=0x00; 

// Timer/Counter 0 initialization
TCCR0=(1<<0)|(1<<2);
TCNT0=0x3D;

// Timer/Counter 1 initialization
TCCR1A=0x00;
TCCR1B=0x03;
TCNT1H=0xFE;
TCNT1L=0x89;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0xFF;
OCR1AL=0xF9;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization    
ASSR=0x00;
TCCR2=0x00;
TCNT2=0x00;
OCR2=1;

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
MCUCR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x14;

// Analog Comparator initialization
ACSR=0x80;
SFIOR=0x00;

//Обновление переменных будильника
hour_alarm=e_hour_alarm; minute_alarm=e_minute_alarm; 

//Обновить светодиоды будильника
switch(alarm_mode)    
{
 case 0: ALARM_LED=0; DET_LED=0; break;  
 case 1: ALARM_LED=1; break;  
 case 2: ALARM_LED=1; DET_LED=1; break;  
} 

// I2C Bus initialization
i2c_init();

// DS1307 Real Time Clock initialization
// Square wave output on pin SQW/OUT: On
// Square wave frequency: 4096Hz
rtc_init(0,0,0);       

//Инициализация кнопок
BUTTON_Init();

//Инициализация портов ввода-вывода и включение всех нндикаторов
IND_Init();
ALL_TUBE_ON; 
                            
// Global enable interrupts
#asm("sei")

while (1)
      {
       if(!starting_countdown) { digit_12=&hour; digit_34=&minute; } //Обычный режим отображения времени    
       
       switch_key=BUTTON_GetKey();  //Проверка нажатия кнопок, сохранил код нажатой кнопки в переменной switch_key
       
       if(switch_key)
       {
        //УСТАНОВКА ЧАСОВ
        if(switch_key==KEY_BUTTON_1) 
        {
         if(++hour==24) hour=0;
        }
        //УСТАНОВКА МИНУТ
        if(switch_key==KEY_BUTTON_2) 
        {    
         if(++minute==60) minute=0;
        }
       
        rtc_set_time(hour,minute,0);
       }
       
       //УСТАНОВКА БУДИЛЬНИКА
       if((!ALARM)&&(!starting_countdown)) 
       {
        delay_ms(20);
        hour_alarm=e_hour_alarm; minute_alarm=e_minute_alarm;
        digit_12=&hour_alarm; digit_34=&minute_alarm;         //Индикатор показывыает настройки будильника
        while(!ALARM) 
        {         
         switch_key=BUTTON_GetKey();
       
         if(switch_key==KEY_BUTTON_1) 
         {
          if(++hour_alarm==24) hour_alarm=0;                   
         }
       
         if(switch_key==KEY_BUTTON_2) 
         {
          if(++minute_alarm==60) minute_alarm=0;            
         }
         
         if((!DET)&&(!release)) { delay_ms(20); if(!DET) { if(++alarm_mode==3) alarm_mode=0; release=1;                                                              
                                                           switch(alarm_mode)               //Установки будильника
                                                           {
                                                            case 0: ALARM_LED=0; DET_LED=0; break;  //Отключен
                                                            case 1: ALARM_LED=1; break;  //Будильник сработает один раз
                                                            case 2: ALARM_LED=1; DET_LED=1; break;  //Будильник будет срабатывать каждый день в установленное время 
                                                           }                                                                      
                                                         }}
         if(DET) release=0;
        } 
        e_hour_alarm=hour_alarm;
        e_minute_alarm=minute_alarm;
       }
       
       //УСТАНОВКА ТАЙМЕРА
       if(!DET) 
       {
        delay_ms(20);  
        minute_timer=e_minute_timer; second_timer=e_second_timer;
        digit_12=&minute_timer; digit_34=&second_timer; //Индикатор показывыает настройки таймера  
        //РАЗРЕШЕНИЕ ОБРАТНОГО ОТСЧЁТА
        if(!starting_countdown) starting_countdown=1;   //Включение режима обратного отсчёта
        if((!WIRE_1)&&(!WIRE_2)&&(!WIRE_3)&&(!WIRE_4)) defuse_bomb_flag=1; //Проверить укомплектованность 4-х проводов
        else  defuse_bomb_flag=0;
        defuse=rand()%4;  //Установка флагов обезвреживания и детонации бомбы
        do { detonate=rand()%4; } while(defuse==detonate);
                             
        while(!DET) 
        {                   
         switch_key=BUTTON_GetKey();
       
         if(switch_key==KEY_BUTTON_1) 
         {
          if(++minute_timer==99) minute_timer=0;                      
         }
       
         if(switch_key==KEY_BUTTON_2) 
         {
          if(++second_timer==60) second_timer=0;            
         }
        }
        e_minute_timer=minute_timer;
        e_second_timer=second_timer;
       }           
       
       //СЧИТЫВАНИЕ ВРЕМЕНИ
       delay_ms(20);
       rtc_get_time(&hour,&minute,&second);
                      
       //Включение точки при изменении секунд
       if(!starting_countdown)
       {
        if(last_second!=second) 
        {
         last_second=second;      
         point=1; 
        }
       } 
       
       //"ОБЕЗВРЕЖИВАНИЕ БОМБЫ"           
       if(defuse_bomb_flag && starting_countdown) check_wire();
       
       //ОБРАТНЫЙ ОТСЧЁТ ВРЕМЕНИ
       if(starting_countdown)
       {
        blinking=1;  //Разрешение мигания светодиода DET_LED
        if(last_second!=second)  
       {
        last_second=second;
        ALARM_LED=0; 
        beep(SOUND_FREQ);
        if(second_timer==0) { second_timer=60; minute_timer--; }
        second_timer--;
        delay_ms(60); 
        beep(0);     
       
        if((!minute_timer)&&(!second_timer)) //Если достигли 0
        {          
         detonate_bomb();               
        }
       } 
       }
       
       //БУДИЛЬНИК
       if((hour==hour_alarm)&&(minute==minute_alarm)&&(second==0)&&alarm_mode&&!alarm_on) 
       {
        alarm_on=1;    //Будильник работает
        
        if((!WIRE_1)&&(!WIRE_2)&&(!WIRE_3)&&(!WIRE_4)) defuse_bomb_flag=1; //Проверить укомплектованность 4-х проводов
        else  defuse_bomb_flag=0;
        defuse=rand()%4; //Установка флагов обезвреживания и детонации бомбы
        do { detonate=rand()%4; } while(defuse==detonate);
        
        minute_timer=0; second_timer=10;
        digit_12=&minute_timer; digit_34=&second_timer;
        starting_countdown=1;
        
        //Начнётся обратный отсчёт с 10 секунд, после этого некоторое время будет звучать сигнал и мигать светодиоды
        if(alarm_mode==1)    
        {
         alarm_mode=0; //Отключили установку будильника
         alarm_on=0;               //Сбросить флаг работы будильника
        }        
        
        if(alarm_mode==2)    
        {
         alarm_on=0;               //Сбросить флаг работы будильника
        }
       } 
            
      };
}
