/* System sterowania siewnikami rolniczymi
 *  Założenia
 *  - dowolny typ siewnika - zbożowy, kukurydzy, warzywny...
 *  - dowolny producent
 *  - współpraca z różnymi układami automatyki (elektrozawory, siłowniki elektryczne/pneumatyczne/hydrauliczne)
 *    możliwe wykożystanie istniejacych czujników, przy założeniu że ograniczymy sygnał na wejściu - np zener 5V1
 *  - w pełni konfigurowalny
 *    parametry siewnika
 *    sekwencje sterowania urzytkownika
 *  - automatyczne wykrywanie podłączonych czujników poziomu ziarna
 *  - automatyczne wykrywanie połączenia z urządzeniem Android
 *  - automatyczne wykrywanie typu sterownika przez system Android
 *  - może pracować samodzielnie jako czujnik poziomu ziarna - info wizualne i dźwiękowe
 *    Automatycznie ograniczona funkcjonalność
 *  - konsola sterująca urządzenie systemem Android
 *  - komunikacja za pomocą interf Bluetooth
 *  - baza danych na systemie Android ze statystykami (user + geo + admdata + stat data)
 *  - wykożystanie bazy danych do synchronizacji pracy z innymi sterownikami - np opryskiwacz, lub samej nawigacji
 *  - ustawienia konfiguracji siewnika zapisane na sterowniku EEPROM (synchronizacja po podłączeniu urządzenia Android)
 *  - możliwość podłączenia różnych urządzeń Android do tego samego sterownika
 *  - wykozystanie akcelerometru do rozróżniania awaryjnego cofania ze zmianą ścieżki
 *  - zasilanie steownika z instalacji 12V
 * 
 * Czujniki sterownika:
 * 1-8 GPIO x poziomu ziarna liniowe optyczne (własny pomysł) 0/5V
 * 1 GPIO x czujnik obrotów (na koło lub wałek wysiewający) - indukcyjny 0/5V
 * 1 GPIO x czujnik podniesienia siewnika - indukcyjny - 0/5V
 * 2 GPIO x czuknik położenia znaczników - indukcyjny - 0/5V
 *  
 * Czujniki urządzenie Android:
 * GPS
 * akcelerometr
 * * dodatkowo transmisja danych, aktualne mapy
 * 
 * Wyjścia sterownika:
 * 3 GPIO x PWM RGB LED
 * 1 GPIO x SOUND ALARM
 * 2 GPIO x znaczniki
 * 1 GPIO x ścieżki technologiczne
 * 2 GPIO x komunikacja Bluetooth
 * 
 * System Android
 *   Wyliczanie statystyk
 *   - liczenie przejazdów: na pole dzienny/roczny/calkowity
 *   - liczenie ścieżek technologicznych: na pole
 *   - liczenie czasu spedzonego na siewie: dzienny/roczny/calkowity
 *   - liczenie czasu spedzonego na uwrociach: dzienny/roczny/calkowity
 *   - liczenie prędkości wysiewu: chwilowej, sredniej dziennej/rocznej/calkowitej
 *   - liczenie zasianych ha: dzienne/rocznne/całkowite, typu zasiewu dzienne/roczne/całkowite
 *   - liczenie wysianej dawki l/ha: dzienne/roczne/całkowite, typu zasiewu dzienne/roczne/całkowite
 *   - przewidywanie ilosci przejazdów oraz ilosci pola do zasiania, aktualnej ilosci nasion l
 *   - zapisywanie daty i rodzaju wysiewu dla danego pola
 *   
 *   Zapisywanie danych GEO
 *   - definiowanie pola
 *     powierzchnia, lista działek
 *     data utworzenia pola
 *   - edycja pola
 *   - kasowanie pola
 *   
 *   Sterowanie
 *   - przycisk PAUZA/KONTYNUACJA zatrzymujacy i wznawiający dzialanie sterownika (liczy statystyki)
 *   - przycisk START/KONIEC - zatrzymuje działanie systemu (nie liczy statystyk)
 *   - opcja wznów po akcji KONIEC
 *   - ręczne włączenie ścieżek technologicznych
 *   - ręczne przełączenie znaczników
 *   - ręczne złożenie znaczników
 *   - kalibracja czujników poziomu ziarna
 *   - definiowanie sekwencji sterowania siewnikiem
 *   
 *   Komunikacja
 *   Dane przesyłamy między urządzeniem Android a sterownikiem asynchronicznie.
 *   Dana w postaci String z autozabezpieczeniem MD5 lub zwykłym liczniiem znakow
 *   Format
 *   Ramka arduino -> android
 *   #typ_danych$a1;a2;a3;a4;a5;a6;a7;a8;d1;d2;d3;d4;d5;d6;d7;d8@suma_kontrolna%
 *    
 * *Przyjmujemy że pole to cześć lub suma kilku działek administracyjnych.
 */

#if defined(ARDUINO_AVR_NANO)
  #define NANO 1
#else
  #define NANO 0
#endif

// Porty do pomiaru poziomu ziarna w zasobniku
// Dla wersji Arduino Nano definiujemy 8 kanałów
#define GET_SEED_1 A0
#define GET_SEED_2 A1
#define GET_SEED_3 A2
#define GET_SEED_4 A3
#define GET_SEED_5 A4
#define GET_SEED_6 A5
#define GET_SEED_7 A6
#define GET_SEED_8 A7

// Komunikacja szeregowa z modułem bluetooth HCxx lub podobnym
#define RX 12
#define TX 13

// Porty do komunikowania stanu zasypu zasobnika ziarna
#define SET_LED_R 11
#define SET_LED_G 10
#define SET_LED_B 9

// Porty do sterowania automatyką siewnika
#define SET_TECH_PATH 8
#define SET_L_POINTER 7
#define SET_R_POINTER 6
#define GET_L_POINTER 5
#define GET_R_POINTER 4
#define GET_ROLLER_RPM 3
#define GET_SEEDER_RPM 2

#define DEBUG 1

#include "settings.h"
#include "seeder.h"
#include "bluetooth.h"

void synchronisation()
{
  // jesli wykrywa urzadzenie to
  //    zestaw polaczenie
  //    odczytaj EEPROM
  //    wyslij konfiguracje
  //    przejdz do glownej petli
  // jesli nie ma polaczenia
  //    odczytaj konfiguracje 
}

void setup()
{
  if(DEBUG)
  {
    Serial.begin(9600);
  }
  
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  bt_setup();
  
 /*
  * zawszcze czytaj wszystkie czujniki
  * jesli stan wysoki znaczy ze nie ma podlaczonego czujnika
  * jesli stan < wysoki znaczy czujnik podlaczony 
  */
  pinMode(GET_SEED_1, INPUT_PULLUP);
  pinMode(GET_SEED_2, INPUT_PULLUP);
  pinMode(GET_SEED_3, INPUT_PULLUP);
  pinMode(GET_SEED_4, INPUT_PULLUP);
  pinMode(GET_SEED_5, INPUT_PULLUP);
  pinMode(GET_SEED_6, INPUT_PULLUP);
  if (NANO)
  {
    pinMode(GET_SEED_7, INPUT_PULLUP);  // nano only
    pinMode(GET_SEED_8, INPUT_PULLUP);  // nano only
  }

  pinMode(SET_LED_R, OUTPUT);
  pinMode(SET_LED_G, OUTPUT);
  pinMode(SET_LED_B, OUTPUT);
  digitalWrite(SET_LED_R, LOW);
  digitalWrite(SET_LED_G, LOW);
  digitalWrite(SET_LED_B, LOW);
  
  pinMode(SET_TECH_PATH, OUTPUT);           // 8
  pinMode(SET_L_POINTER, OUTPUT);           // 7
  pinMode(SET_R_POINTER, OUTPUT);           // 6
  pinMode(GET_L_POINTER, INPUT_PULLUP);     // 5
  pinMode(GET_R_POINTER, INPUT_PULLUP);     // 4
  digitalWrite(SET_TECH_PATH, LOW);
  digitalWrite(SET_L_POINTER, LOW);
  digitalWrite(SET_R_POINTER, LOW);

  // obsługa RPM
  pinMode(GET_SEEDER_RPM, INPUT_PULLUP);    // 2
  pinMode(GET_ROLLER_RPM, INPUT_PULLUP);    // 3
  //attachInterrupt(digitalPinToInterrupt(GET_SEEDER_RPM), read_rpm, FALLING);
  call_interrupt(GET_SEEDER_RPM, read_rpm_seeder);
  call_interrupt(GET_ROLLER_RPM, read_rpm_roller);
  
  synchronisation();
}

// seeder_command - is a global variable keeping enum data
bool bt_connection_status = true; // jesli polaczenie bedzie ok, obsluz dodatkowe funkcje

void loop()
{
  // czytaj czujnik nasion
  Serial.println(meassure_seed_level());
  // wyswietl status nasion na monitorze LED
  if(bt_connection_status)
  {
    get_seeder_speed();
    get_roller_speed();
  // czytaj czujnik obrotw - najwyzsze prio, bardzo czesto
  // czytaj czujnik podniesienia
  // czytaj czujniki znaczników
  // sendBtData();
  // receiveBtData();
  // zmien ustawienia jesli potrzba
  if(rpm_seeder_counter%2) seeder_command = set_marker_zero;
  control_seeder();
  }
}
