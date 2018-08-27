#include <Rotary.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "DRV8825.h"

#define MOTOR_STEPS 200 // obsługa krokowca
#define DIR 10
#define STEP 9
#define ENBL 11

 
 DRV8825 stepper(MOTOR_STEPS, DIR, STEP);
 
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);   // obsługa wyświetlacza


Rotary r = Rotary(2, 3);          // obsługa enkodera


const int pinWyjsciaNaDiodeIR = 7;        // numer pinu do którego będzie podłączona anoda diody podczerwieni
const int pinWejsciaPrzycisku1 = A2;      // numer pinu do którego będzie podłączony przycisk wyzwalający pojedyńcze zdjęcie
const int pinWejsciaPrzycisku2 = A1;      // numer pinu do którego będzie podłączony przycisk trybu ciągłego wyzwalania zdjęć

boolean stanLogicznyWyzwalania = 0;                  // zmienna przechowująca aktualny stan logiczny wyzwalania w trybie ciągłego robienia zdjęć
boolean stanLogicznyPrzycisku1 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean poprzedniStanLogicznyPrzycisku1 = 1;         // zmienna przechowująca poprzedni stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean stanLogicznyPrzycisku2 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku trybu ciągłego wyzwalania zdjęć
boolean poprzedniStanLogicznyPrzycisku2 = 1;

int stanMenu;                                     // zmienna przechowująca poprzedni stan logiczny przycisku trybu ciągłego wyzwalania zdjęć
int trybPredkosci;                                // zmienna trybu predkosc w wideo
int obrotManualny = 0;                            // zmienna liczaca kroki dla trybu mmanualnego
int obrotManualnyStan=0;                          // stan zmiennej
int interwalCzasuMiedzyZdjeciami = 1500;          // czas między każdym zdjęciem w trybie ciągłego wyzwalania zdjęć (w milisekundach)
const int czasSwieceniaDiody1Zdjecie = 100;       // czas świecenia diody po wciśnięciu przycisku wyzwalającego pojedyńcze zdjęcie (w milisekundach)
const int czasOscylacji = 40;                     // czas opóźnienia dla ustabilizowania sygnału po wcisnięciu przycisków (w milisekundach)

unsigned long obecnyCzas = 0;                     // zmienna przechowująca aktualny pomiar czasu jaki upłynął od momentu uruchomienia mikrokontrolera
unsigned long poprzedniCzas = 0;                  // zmienna przechowująca poprzedni pomiar czasu jaki upłynął od momentu uruchomienia mikrokontrolera

                                                  // cczasy do silnika krokowego w mikrosekundach
const int predkoscSilnikaMax = 1550;               //minimalna wartosc przerwy miedzy impulsami kroku dla trybu auto
const int predkoscSilnikaMin = 2050;               //maksymalna wartosc przerwy miedzy impulsami kroku dla trybu auto
int dlugoscObrotu = 25210;                        // 25210 wynosi pełen obrót na przekładni zębatej 5:1 i mikrokroku 1/8
const int dlugoscSoft = (predkoscSilnikaMin-predkoscSilnikaMax);               
const int czasImpulsu = 350;
int iloscZdjec = 1; 
const int czasOczekiwania = 2000;
int licznik = 1;

boolean wracacDoMenu = 0;         //jeśli 0 nie bedzie wracac do menu, jesli 1 bedzie


                                  // FUNKCJA DOSTOSOWANA DO UNO, TRZEBA ZMIENIC PORT JAK DO INNEGO ARDUINO!!!
void zrobZdjecie()              // definicja funkcji generującej dwa impulsy o częstotliwości ok. 32kHz
{ 
  for(int i=0;i<17;++i)         // wykonanie 17 razy pętli spowoduje wygenerowanie sygnału trwającego 510 mikrosekund [17 x (15 + 15)]
  {
    PORTD = B10000000;         // przypisanie 7-mu bitowi portu B wartość 1, co spowoduje podanie wysokiego sygnału na pinie 13 mirkokontrolera (na płytce Arduino Micro)
   _delay_us(15);            // opóznienie generujące pierwszą połowę cyklu
    PORTD = B00000000;         // przypisanie 7-mu bitowi portu B wartość 0, co spowoduje podanie niskiego sygnału na pinie 13 mirkokontrolera (na płytce Arduino Micro)
   _delay_us(15);            // opóznienie generujące drugą połowę cyklu
  }
  _delay_ms(7);                 // opóźnienie między impulsami
   for(int i=0;i<17;i++)
  {
    PORTD = B10000000;
    _delay_us(15);
    PORTD = B00000000;
    _delay_us(15);
   }
} 

  

void procentobrotu()                               //funkcja obracająca o 1% koła 
{
  digitalWrite(ENBL, LOW);
  
 for(int i =0; i<(dlugoscObrotu/100); i++)
 {  
   
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
        _delay_us(450);

}

}


void obrot()                                   //funkcja obracająca o część koła wynikająca z liczby zdjęć z łagodnym rozruchem i hamowaniem 

{

digitalWrite(ENBL, LOW);                      // ustawianie włączenie sterownika silnika       
                   // ustawianie kierunku 

 for(int i =0; i<(dlugoscObrotu/iloscZdjec); i++)
 {
   if (i<dlugoscSoft)
   {
    digitalWrite(STEP, HIGH);
   _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
   delayMicroseconds(predkoscSilnikaMin-i);
   }
  else if (i>(dlugoscObrotu/iloscZdjec)-dlugoscSoft)
   {
     digitalWrite(STEP, HIGH);
   _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
    delayMicroseconds(predkoscSilnikaMax+(i-dlugoscObrotu/iloscZdjec+dlugoscSoft));
   }
   else   
   {  
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
        _delay_us(predkoscSilnikaMax);
   }


}

}

void automatyczny()                                                                          // AUTOMATYCZNY
{
  licznik = iloscZdjec;
boolean stanLogicznyPrzycisku1 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean poprzedniStanLogicznyPrzycisku1 = 1;         // zmienna przechowująca poprzedni stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean stanLogicznyPrzycisku2 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku trybu ciągłego wyzwalania zdjęć
boolean poprzedniStanLogicznyPrzycisku2 = 1;


  lcd.clear();  
 do
 {
  int x = 0;                       // czyszczenie ekranu dla zmiany z 10 na 9

    if (licznik==9 || x==1)
  {
    lcd.setCursor(11, 1);
      lcd.print(char(143));

    x=0;                            // koniec czyszczenia
  }
  lcd.setCursor(0,0);
  lcd.print("Podaj ilosc zdjec:");
  lcd.setCursor(10, 1);
  lcd.print(licznik);
  lcd.setCursor(0, 3);
  lcd.print("cofnij");
  lcd.setCursor(11, 3);
  lcd.print("zatwierdz");
  
  if (licznik==10)                //sprawdzenie czy pojawiło się 10 
  { 
    x=1;
  }

   stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);    
   stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);                  // COFANIE
   delay(czasOscylacji);

 
 if (poprzedniStanLogicznyPrzycisku2 != stanLogicznyPrzycisku2)    
  {
    if(stanLogicznyPrzycisku2 == 1)                                
    {
        lcd.clear();  
        return;
    }
   
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
    poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
  } 

      
stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);                      // Zatwierdzanie wyboru
     
  delay(czasOscylacji);
     
 if (poprzedniStanLogicznyPrzycisku1 != stanLogicznyPrzycisku1)             //petla wykonujaca wybrana liczbe zdjec i obrotow
  {

  lcd.clear();  
  lcd.setCursor(0,0);
  lcd.print("Prosze czekac");

     iloscZdjec=licznik;
     
     for(int i=0; i<iloscZdjec; i++)
     {
      
      delay(300);
      digitalWrite(ENBL, HIGH);     
      delay(1000); 
   //   digitalWrite(ENBL, LOW);     
    //  delay(1000); 
     //digitalWrite(ENBL, HIGH); 

      
  lcd.setCursor(0,0);
  lcd.print("Prosze czekac");
  lcd.setCursor(0,2);
  lcd.print("Robie zdjecie");
  noInterrupts();
  
      zrobZdjecie();
      
   interrupts();

  delay(czasOczekiwania);
  lcd.clear();  
  lcd.setCursor(0,0);
  lcd.print("Prosze czekac");
  lcd.setCursor(0,3);
  lcd.print("Zrobiono zdjec:");
  lcd.setCursor(15,3);
  lcd.print(i+1);
  
         digitalWrite(ENBL, LOW); 
         digitalWrite(DIR, LOW);   
         obrot();
   
    } 
    
    stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);  
    poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;
    delay(700);
    digitalWrite(ENBL, HIGH); 
    lcd.clear();
      
      }  
              
  } while (stanLogicznyPrzycisku1 == poprzedniStanLogicznyPrzycisku1 && stanLogicznyPrzycisku2 == poprzedniStanLogicznyPrzycisku2);
        poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;
 
    lcd.clear();  

}
  

  
void manualny()                                                                            // MANUALNY
{

  lcd.clear();
  int x=0;
 obrotManualny=0;
 obrotManualnyStan=0;
boolean stanLogicznyPrzycisku1 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean poprzedniStanLogicznyPrzycisku1 = 1;         // zmienna przechowująca poprzedni stan logiczny przycisku wyzwalającego pojedyńcze zdjęcie
boolean stanLogicznyPrzycisku2 = 1;                  // zmienna przechowująca aktualny stan logiczny przycisku trybu ciągłego wyzwalania zdjęć
boolean poprzedniStanLogicznyPrzycisku2 = 1;

  do {

delay(czasOscylacji);
  lcd.setCursor(8,0);
  lcd.print("STOP ");
  lcd.setCursor(0, 3);
  lcd.print("cofnij");
  lcd.setCursor(16,2);
  lcd.print("zrob");
  lcd.setCursor(13,3);
  lcd.print("zdjecie");


                   
 while(obrotManualny!=obrotManualnyStan)
 {
  lcd.setCursor(0,1);
  lcd.print("Czekaj...");
  
   if (obrotManualny>obrotManualnyStan)                    // PRAWO
   {
    lcd.setCursor(8,0);
    lcd.print("START");
    digitalWrite(DIR, HIGH);
    procentobrotu();
    obrotManualnyStan=obrotManualnyStan+1;
      
   }
   if (obrotManualny<obrotManualnyStan)                   // LEWO
   {
    lcd.setCursor(8,0);
    lcd.print("START");
    digitalWrite(DIR, LOW);
    procentobrotu(); 
    obrotManualnyStan=obrotManualnyStan-1;
   }
   
  if (obrotManualny==9 && obrotManualny == -9 || x==1)          //czyszcznie przy zmianie z 10 na 9
  {
    lcd.setCursor(17, 0);
      lcd.print(char(143));
       lcd.print(char(143));
    
    x=0; 
  }
    
  if (obrotManualny==99 && obrotManualny ==-99 || x==1)          //czyszczenie przy zmieniae z 100 na 99
  {
    lcd.setCursor(18, 0);
      lcd.print(char(143));
    x=0; 
  }
    if (obrotManualny==0 || x==2)                                 // usuwanie minusa przy zmianie z -1 na 0
  {
    lcd.setCursor(15, 0);
      lcd.print(char(143));
    x=0; 
  }
      if (obrotManualny==101 || obrotManualny==-101)              //Zmiana z 100% na 0%
  {
   obrotManualny=0;
   obrotManualnyStan=0;
   lcd.setCursor(15, 0);
   lcd.print(char(143));
   lcd.print(char(143));  
   lcd.print(char(143));
   lcd.print(char(143));
  }
  if(obrotManualny<0)                                           //Wyświetlanie ujemnych procentów
  {
    lcd.setCursor(15,0);
    lcd.print(obrotManualny);
    if(obrotManualny>-10) 
    {
      lcd.setCursor(17,0);
      lcd.print(" ");
    }
  }
  else                                                           // Wyświetlanie dodatnich procetnów
  {
   lcd.setCursor(16,0);
  lcd.print(obrotManualny);
      if(obrotManualny<10) 
    {
      lcd.setCursor(17,0);
      lcd.print(" ");
    }
  }
  lcd.setCursor(19,0);
  lcd.print("%");
  if(obrotManualny==100 || obrotManualny==-100) x=1;                 //sprawdzanie czy pojawiło się 100
  if (obrotManualny==10 || obrotManualny==-10) x=1;                  //sprawdzenie czy pojawiło się 10 
  if (obrotManualny==-1) x=2;                                       //sprawdzanie czy pojawiła się ujemna liczba


  lcd.setCursor(0,1);
  lcd.print("           ");
  
 }

 digitalWrite(ENBL, HIGH);
  
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);                // petla cofania do menu
 
if (poprzedniStanLogicznyPrzycisku2 != stanLogicznyPrzycisku2)    
  {
    if(stanLogicznyPrzycisku2 == 1)                                
    {
        lcd.clear();  
        obrotManualny=0;
        obrotManualnyStan=0;
        return;
    }
   
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
    poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
  } 

    stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);                // petla robienia zdjecia pod przycisk
 
if (poprzedniStanLogicznyPrzycisku1 != stanLogicznyPrzycisku1)               
  {
    if(stanLogicznyPrzycisku1 == 1)                                
    {
  lcd.setCursor(0,1);                                                         // informacja o zdjeciu a nastepnie czyszczenie, czasy miedzy uruchomieniem a zatrzymaniem
  lcd.print("Prosze czekac");
  lcd.setCursor(0,2);
  lcd.print("Robie zdjecie");
  delay(500);
   zrobZdjecie();
   delay(300);
     lcd.setCursor(0,1);
  lcd.print("                  ");
  lcd.setCursor(0,2);
  lcd.print("                  ");
   
    }
   
    stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);  
    poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;
  } 




  
  
  
      

               
  } while (stanLogicznyPrzycisku2 == poprzedniStanLogicznyPrzycisku2);
        poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
   lcd.clear();

    
}

void wideo()
{


  trybPredkosci=3;                              // 3 jest środkowym trybm i oznacza zatrzymanie
 lcd.clear();
  do 
  {
     if (trybPredkosci == 3 )  
  {
    
  lcd.setCursor(8,0);
  lcd.print("STOP    ");  
  lcd.setCursor(1,1);
  lcd.print("Wybierz kierunek");
  lcd.setCursor(2,2);
  lcd.print("oraz predkosc");
  lcd.setCursor(0, 3);
  lcd.print("cofnij");
  digitalWrite(ENBL, HIGH);
  }


if (trybPredkosci > 3)                                          // obroty w Prawo, kolejne case to szybszy obrót
{
 lcd.clear();
  lcd.setCursor(8,0);
  lcd.print("W PRAWO");  
  lcd.setCursor(1,1);
  lcd.print("Predkosc:");

  digitalWrite(10, HIGH);               //obrot w LEWO (wynika z przekładni)
  digitalWrite(ENBL, LOW);                //uruchomienie silnika
  

    
  switch(trybPredkosci)                // wybór trybu
      
       {
   
    case 4:                                                                  // w każdy case analogicznie
    
         stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);         // sprawdzamy cofanie, niepotrzebne to ale na przyszlosc
         lcd.setCursor(2,2);                                                 // wypsiujem tempo
         lcd.print("WOLNO");
         
   for(int i =0; i < 450 ; i++)                                               // petla for jest rampą rozpędzania
   {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(czasImpulsu);
    digitalWrite(STEP, LOW);
    delayMicroseconds(1650-i);
   }
    do                                                                       // pętla pracy ciąglej
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(1150);
    }while (trybPredkosci==4);
   
     if(trybPredkosci==3)                                                    //jeśli tryb w dół to pętla for tu zwalnia
     {
      for(int i=0; i <450; i++)
      {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(1150+i);
      }
   }
        break; 



        case 5: 
          lcd.setCursor(2,2);
          lcd.print("SREDNIO");
          stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2); 

           
  for( int i=0; i < 450 ; i++)
   {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(1150-i);
   }
   do
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(700);
    }while (trybPredkosci==5);
         if(trybPredkosci==4)
     {
      for(int i=0; i <450; i++)
      {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(czasImpulsu);
      digitalWrite(STEP, LOW);
      delayMicroseconds(700+i);
      }
   }
     break;

        case 6:
       stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
       lcd.setCursor(2,2);
       lcd.print("SZYBKO");
       
   for(int i=0; i < 450 ; i++)
   {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(700-i);
   }
    do
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(250);
    }while (trybPredkosci==6);
         if(trybPredkosci==5)
     {
      for(int i=0; i <450; i++)
      {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(czasImpulsu);
      digitalWrite(STEP, LOW);
      delayMicroseconds(250+i);
      }
    }
   break;

   
  }

  
  stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);               

  
}



if (trybPredkosci < 3)                                       // W lewo

{
  lcd.clear();
  lcd.setCursor(8,0);
  lcd.print("W LEWO"); 
  lcd.setCursor(1,1);
  lcd.print("Predkosc:");

  digitalWrite(DIR, LOW); 
  digitalWrite(ENBL, LOW); 

  
  switch(trybPredkosci)
  {
    case 2:
    
    lcd.setCursor(2,2);
    lcd.print("WOLNO");
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
    
  for(int i=0; i < 450 ; i++)
   {
   digitalWrite(STEP, HIGH);
   delayMicroseconds(czasImpulsu);
   digitalWrite(STEP, LOW);
   delayMicroseconds(1550-i);
   } 
    do
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(1150);
    }while (trybPredkosci==2);
   
     if(trybPredkosci==3)
     {
      for(int i=0; i <450; i++)
      {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(1150+i);
      }
     digitalWrite(ENBL, HIGH);
   }
        break; 


        case 1:

     lcd.setCursor(2,2);
     lcd.print("SREDNIO");
     stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
     
     for(int i=0; i < 450 ; i++)
   {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(czasImpulsu);
    digitalWrite(STEP, LOW);
    delayMicroseconds(1150-i);
   } 
   
    do
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(700);
    }while (trybPredkosci==1);
     
     if(trybPredkosci==2)
     {
      for(int i =0; i <450; i++)
      {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(700+i);
      }
   }
        break;

        case 0:
         
       stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
       lcd.setCursor(2,2);
       lcd.print("SZYBKO");
     for(int i=0; i < 450 ; i++)
   {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(700-i);
   }
    do
    {
     digitalWrite(STEP, HIGH);
     _delay_us(czasImpulsu);
     digitalWrite(STEP, LOW);
     _delay_us(250);
    }while (trybPredkosci==0);
         if(trybPredkosci==1)
     {
      for(int i=0; i <450; i++)
      {
     digitalWrite(STEP, HIGH);
     delayMicroseconds(czasImpulsu);
     digitalWrite(STEP, LOW);
     delayMicroseconds(250+i);
      }
     
   }
   break;
  }
       

    
}
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
if (poprzedniStanLogicznyPrzycisku2 != stanLogicznyPrzycisku2)    
  {
    if(stanLogicznyPrzycisku2 == 1)                                
    {
        lcd.clear();  
         stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
    poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
        return;
    }
   
    stanLogicznyPrzycisku2 = digitalRead(pinWejsciaPrzycisku2);  
    poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
  } 

}while (stanLogicznyPrzycisku2 == poprzedniStanLogicznyPrzycisku2);
        poprzedniStanLogicznyPrzycisku2 = stanLogicznyPrzycisku2;
   lcd.clear();




  
 return;
}
  
void menu()                                                     //MENU
{
    digitalWrite(ENBL, HIGH);
  lcd.setCursor(3,0);
  lcd.print("Wybierz tryb:");  
  lcd.setCursor(4,1);
  lcd.print("AUTOMATYCZNY");
  lcd.setCursor(6,2);
  lcd.print("MANUALNY");
  lcd.setCursor(7,3);
  lcd.print("WIDEO");
  
  
  switch(stanMenu)
  {

    
  case 0:
  
      do
           {

       lcd.setCursor(3,1);
       lcd.print(char(126));
       lcd.setCursor(5,2);
       lcd.print(char(143));
       lcd.setCursor(6,3);
       lcd.print(char(143));

       stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);      
       delay(czasOscylacji);                                          
    
  if(poprzedniStanLogicznyPrzycisku1 != stanLogicznyPrzycisku1)    
  {
    if(stanLogicznyPrzycisku1 == 1)                                 
    {
       poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;   
           licznik=1;
           automatyczny();
           stanMenu=1;
     }
    poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;   
   
  if(stanMenu==1)
    {
      stanMenu=0;
      break;
    }
   }  
  }  while (stanMenu==0);
   
   break;
     
 

  case 1:
  
while(stanMenu==1)
{
  lcd.setCursor(3,1);
  lcd.print(char(143));
  lcd.setCursor(5,2);
  lcd.print(char(126));
  lcd.setCursor(6,3);
  lcd.print(char(143));
  
  stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);      
  delay(czasOscylacji);    
  
  if(poprzedniStanLogicznyPrzycisku1 != stanLogicznyPrzycisku1)    
  {
    if(stanLogicznyPrzycisku1 == 1)                                 
    {
         manualny();      
         stanMenu=0;
    }
    poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;  
        
     if(stanMenu==0)
        {
            stanMenu=1;
            break;
        }
    }  
}
 
  break;

    case 2:
  
do
  {
  lcd.setCursor(3,1);
  lcd.print(char(143));
  lcd.setCursor(5,2);
  lcd.print(char(143));
  lcd.setCursor(6,3);
  lcd.print(char(126));

  stanLogicznyPrzycisku1 = digitalRead(pinWejsciaPrzycisku1);      
  delay(czasOscylacji);                                          
    
  if(poprzedniStanLogicznyPrzycisku1 != stanLogicznyPrzycisku1)    
  {
    if(stanLogicznyPrzycisku1 == 1)                                 
    {
           poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;   
           wideo();
           stanMenu=2;
     }
    poprzedniStanLogicznyPrzycisku1 = stanLogicznyPrzycisku1;   
   
      if(stanMenu==1)
          {
             stanMenu=0;
             break;
          }
       }  
  }  while (stanMenu==0);
    
   break;
   
    }
}


 

void setup()
{
 Serial.begin(9600);                                                    // wyświetlanie w monitorze szeregowym
      
  PCICR |= (1 << PCIE2);                                               // przerwania dla enkodera
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();  

  pinMode(pinWyjsciaNaDiodeIR, OUTPUT);                               // ustalenie pinu diody podczerwieni jako wyjście
  pinMode(pinWejsciaPrzycisku1, INPUT_PULLUP);                        // ustalenie pinu przycisku 1 jako wejście z włączonym rezystorem podciągającym
  pinMode(pinWejsciaPrzycisku2, INPUT_PULLUP);                        // ustalenie pinu przycisku 2 jako wejście z włączonym rezystorem podciągającym
  pinMode(STEP, OUTPUT); 
    
  lcd.begin(20,4);                                                    //włączanie wyświetlacza
  lcd.backlight();
                                                                 
}


void loop() 
{
   menu();
}


ISR(PCINT2_vect)
{                                       //przerwanie edytujace interwal czasu o krok 0,5 sekundy
  unsigned char result = r.process();
  if (result == DIR_NONE) {
 //nic nie rób
  }
  else if (result == DIR_CW) {
    
     if (stanMenu > 0) stanMenu = stanMenu-1;
     if (licznik > 1)  licznik = licznik-1;
     if (trybPredkosci > 0) trybPredkosci =  trybPredkosci-1;
     obrotManualny = obrotManualny-1;
  }
  else if (result == DIR_CCW) {
    
     if (stanMenu < 2) stanMenu = stanMenu+1;
     if (licznik < 99) licznik =  licznik+1;
     if (trybPredkosci < 6) trybPredkosci =  trybPredkosci+1;
     obrotManualny = obrotManualny+1;
     }
 }



