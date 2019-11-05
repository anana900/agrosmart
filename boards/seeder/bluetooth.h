/*
 * Transmisja asynchroniczna z wykozystanie HC05
*/

#include <SoftwareSerial.h>
SoftwareSerial Blue(RX, TX);

void bt_setup()
{
  Blue.begin(9600);
  if(DEBUG)
  {
     Serial.print("Ustawienie BT ");
  }
}

int data_type = 0;
// 10 - dev to android regular
// 11 - dev to adnroid synchronisation
// 20 - adnroid to dev

String code_frame(int data_type, String message)
{
  /* 10
   * #typ_danych$a1;a2;a3;a4;a5;a6;a7;a8;d1;d2;d3;d4;d5;d6;d7;d8@suma_kontrolna%
   */
  String data = "#"+String(data_type)+"$"+message;
  return data+"@"+String(data.length())+"%";
}

int decode_frame()
{
  
}

void send_bt_data(int data_type, String message)
{
  String data = code_frame(data_type, message);
  Blue.print(data);
  if(DEBUG)
  {
     Serial.print("Wyslane ");Serial.println(data);
  }
}

String reveive_bt_data()
{
  String message;
  while(Blue.available())
  {
    if(Blue.available() > 0)
    {
      char part_message = Blue.read();
      message += part_message;
    }
  }
  
  if(DEBUG)
  {
     Serial.print("Odebrane ");Serial.println(message); 
  }
  return message;
}
