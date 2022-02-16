//***************************************************************************
//
//  Драйвер 2-х кнопочной клавиатуры
//
//
//***************************************************************************
#ifndef	BUTTONS_h
#define	BUTTONS_h

#pragma used+

//Порт, к которому подключены кнопки
#define PORT_BUTTON 	PORTD
#define PIN_BUTTON 	    PIND
#define DDRX_BUTTON 	DDRD

//Номера выводов, к которым подключены кнопки
#define BUTTON_1 	1 
#define BUTTON_2 	7 

//Коды, которые будут записываться в буфер
#define KEY_BUTTON_1    	1
#define KEY_BUTTON_2     	2

//Сколько циклов опроса кнопка должна удерживаться
#define THRESHOLD  20
#define THRESHOLD2 250

//ПРОТОТИПЫ
void BUTTON_Init(void);
void BUTTON_Scan(void);
unsigned char BUTTON_GetKey(void);

//Макроопределения 
#define MASK_BUTTONS 	(1<<BUTTON_1)|(1<<BUTTON_2)
#define BitIsClear(port, bit)     ((port & (1<<(bit))) == 0)
#define BitIsSet(port, bit)       ((port & (1<<(bit))) != 0)

volatile unsigned char pressedKey = 0; // Переменная для передачи кода нажатой кнопки
unsigned char comp = 0;		  // Переменная для создания антидребезговой выдержки времени

//*************************************************************
//		            Инициализация клавиатуры
//*************************************************************
void BUTTON_Init(void)
{
  DDRX_BUTTON &= ~(MASK_BUTTONS); // Установка выводов на вход
  PORT_BUTTON |= MASK_BUTTONS;    // Установка подтягивающих резисторов
}

//*************************************************************
//		             Функция опроса кнопок
//*************************************************************
void BUTTON_Scan(void)	  // Функция работает в прерывании таймера!
{
unsigned char key;	// Временная переменная для хранения кода нажатой кнопки

  // Последовательный опрос выводов мк
  if (BitIsClear(PIN_BUTTON, BUTTON_1))     
    key = KEY_BUTTON_1;
  else if (BitIsClear(PIN_BUTTON, BUTTON_2))    
    key = KEY_BUTTON_2;
  else  key = 0;

  // Если во временной переменной что-то есть
  if (key) 
 { // и если кнопка удерживается долго
   // записать ее код в буфер 
   if (comp > THRESHOLD2)
   {
     comp = THRESHOLD2 - 35;
     pressedKey = key;
     return;
   }
   else comp++;
    
   if (comp == THRESHOLD) 
   {
    pressedKey = key;
    return;
   }
 } 
  else comp=0;
}

//*************************************************************
//		       Функция возврата кода нажатой кнопки
//*************************************************************
unsigned char BUTTON_GetKey(void) // Функция работает в основной программе и возвращает код нажатой кнопки
{
  unsigned char key = pressedKey; // Сохранил код нажатой кнопки
  pressedKey = 0;
  return key;
}
//*************************************************************
    
#pragma used-

#endif
