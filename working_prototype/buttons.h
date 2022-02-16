//***************************************************************************
//
//  ������� 2-� ��������� ����������
//
//
//***************************************************************************
#ifndef	BUTTONS_h
#define	BUTTONS_h

#pragma used+

//����, � �������� ���������� ������
#define PORT_BUTTON 	PORTD
#define PIN_BUTTON 	    PIND
#define DDRX_BUTTON 	DDRD

//������ �������, � ������� ���������� ������
#define BUTTON_1 	1 
#define BUTTON_2 	7 

//����, ������� ����� ������������ � �����
#define KEY_BUTTON_1    	1
#define KEY_BUTTON_2     	2

//������� ������ ������ ������ ������ ������������
#define THRESHOLD  20
#define THRESHOLD2 250

//���������
void BUTTON_Init(void);
void BUTTON_Scan(void);
unsigned char BUTTON_GetKey(void);

//���������������� 
#define MASK_BUTTONS 	(1<<BUTTON_1)|(1<<BUTTON_2)
#define BitIsClear(port, bit)     ((port & (1<<(bit))) == 0)
#define BitIsSet(port, bit)       ((port & (1<<(bit))) != 0)

volatile unsigned char pressedKey = 0; // ���������� ��� �������� ���� ������� ������
unsigned char comp = 0;		  // ���������� ��� �������� ��������������� �������� �������

//*************************************************************
//		            ������������� ����������
//*************************************************************
void BUTTON_Init(void)
{
  DDRX_BUTTON &= ~(MASK_BUTTONS); // ��������� ������� �� ����
  PORT_BUTTON |= MASK_BUTTONS;    // ��������� ������������� ����������
}

//*************************************************************
//		             ������� ������ ������
//*************************************************************
void BUTTON_Scan(void)	  // ������� �������� � ���������� �������!
{
unsigned char key;	// ��������� ���������� ��� �������� ���� ������� ������

  // ���������������� ����� ������� ��
  if (BitIsClear(PIN_BUTTON, BUTTON_1))     
    key = KEY_BUTTON_1;
  else if (BitIsClear(PIN_BUTTON, BUTTON_2))    
    key = KEY_BUTTON_2;
  else  key = 0;

  // ���� �� ��������� ���������� ���-�� ����
  if (key) 
 { // � ���� ������ ������������ �����
   // �������� �� ��� � ����� 
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
//		       ������� �������� ���� ������� ������
//*************************************************************
unsigned char BUTTON_GetKey(void) // ������� �������� � �������� ��������� � ���������� ��� ������� ������
{
  unsigned char key = pressedKey; // �������� ��� ������� ������
  pressedKey = 0;
  return key;
}
//*************************************************************
    
#pragma used-

#endif
