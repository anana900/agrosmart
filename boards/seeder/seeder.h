
void call_interrupt(uint8_t PIN)
{
  detachInterrupt(digitalPinToInterrupt(PIN));
}

void call_interrupt(uint8_t PIN, void (*isr))                    // przesłanie funkcji jako parametru
{
  attachInterrupt(digitalPinToInterrupt(PIN), isr, FALLING);
}

void read_rpm_seeder()
{
  rpm_seeder_read_window_time_now = millis();
  if (rpm_seeder_read_window_time_now - rpm_seeder_read_window_time_past < RPM_SEEDER_READ_WINDOW)
  {
    return;
  }
  rpm_seeder_counter++;
  rpm_seeder_read_window_time_past = rpm_seeder_read_window_time_now;
}

void read_rpm_roller()
{
  rpm_roller_read_window_time_now = millis();
  if (rpm_roller_read_window_time_now - rpm_roller_read_window_time_past < RPM_ROLLER_READ_WINDOW)
  {
    return;
  }
  rpm_roller_counter++;
  rpm_roller_read_window_time_past = rpm_roller_read_window_time_now;
}

void get_seeder_speed()
{
  call_interrupt(GET_SEEDER_RPM);
  unsigned long time_diff = millis() - rpm_seeder_read_window_time_past;
  call_interrupt(GET_SEEDER_RPM, read_rpm_seeder);
  if(time_diff < 3000)
  {
    if(DEBUG)
    {
      Serial.print(time_diff);Serial.print(" ");Serial.println(rpm_seeder_counter);
    }
  }
}

void get_roller_speed()
{
  call_interrupt(GET_ROLLER_RPM);
  unsigned long time_diff = millis() - rpm_roller_read_window_time_past;
  call_interrupt(GET_ROLLER_RPM, read_rpm_roller);
  if(time_diff < 3000)
  {
    if(DEBUG)
    {
      Serial.print(time_diff);Serial.print(" ");Serial.println(rpm_roller_counter);
    }
  }
}

enum e_command
{
  do_nothing,            // nie wykonuj akcji
  do_stop,               // wymus zatrzymanie bierzacej akcji
  set_sound_alarm_on,
  set_sound_alarm_off,
  set_marker_zero,       // zlozenie znacznikow
  set_marker_left,       // rozlozenie tylko lewego znacznika
  set_marker_right,      // rozlozenie tylko prawego znacznika
  set_marker_left_right, // rozlozenie lewego i prawego znacznika
  set_seeder_up,         // podnies siewnik   
  set_seeder_down,       // opusc siewnik
  set_tpath_on,          // zablokuj wysiewanie na sciezkach
  set_tpath_off          // odblokuj wysiewanie na sciezkach
} seeder_command(do_nothing);

/*
 * marker types:
 * 1 - dwa siłowniki jednostronnego działania, lub jeden siłownik podwójny, sterowane pojedyńczym zaworem.
 *     Trójnik przełączający - podawanie ciśnienia składanie. Otwieranie zaworu, sprężyny rozkładają znaczniki.
 * 2 - dwa siłowniki jednostronnego działania, lub jeden siłownik podwójny, sterowane pojedyńczym zaworem.
 *     Bezpośrednie podawanie ciśnienia poprzez 2 osobne zawory sterujące.
 * 3 - siłowniki elektryczne z krańcówkami wbudowanymi. Czasowe rozkładanie. Sterowanie 4 portami.
 */
error_code set_seeder_marker_zero(int marker_type = 1)
{
  /*
   * Funkcja do pdnoszenia znaczników.
   */
  if (0 == SEEDER_ERROR &&
      !marker_is_busy)                                          // jesli obecnie nie ma zadnej akcji na znacznikach
  {
    marker_is_busy_timer = millis();                            // zresetuj licznik zabezpieczenia czasowego dla znacznikow
    marker_short_run_on_timer = millis();                       // zresetuj licznik zabezpieczenia czasowego dla detekcji l/r znaacznikow
  }
  
  if (set_marker_zero == seeder_command &&                      // jesli jest komenda
      (((digitalRead(GET_R_MARKER)  ||                          // jesli prawy znacznik NIE jest złożony lub
         digitalRead(GET_L_MARKER)) &&                          // jesli lewy znacznik NIE jest złożony oraz
      millis() - marker_is_busy_timer < MARKER_BUSY_WINDOW) ||  // jesli licznik zabezpieczenia czasowego nie jest przekroczony lub
      marker_force_command))                                    // jest wymuszenie komendy
  {
    // włącz składanie znaczników
    marker_is_busy = true;                                     // zajętość akcji na znacznikach
    switch(marker_type)
    {
      case 1:
        if(millis() - marker_short_run_on_timer < MARKER_SHORT_RUN_WINDOW)
        {
          digitalWrite(SET_L_MARKER, HIGH);                    // włącz siłownik znacznika na krotka chwile
          marker_short_run_off_timer = millis();
        }
        else
        {
          if(millis() - marker_short_run_off_timer < MARKER_SHORT_RUN_WINDOW/2)
          {
            digitalWrite(SET_L_MARKER, LOW);                   // wyłącz siłownik znacznika na krotka chwile
          }
          else
          {
            digitalWrite(SET_L_MARKER, HIGH);                  // włącz siłownik znacznika aż znaczniki zostaną ustawione
          }
        }
        break;
      case 2:
        digitalWrite(SET_L_MARKER, HIGH);                      // wyłącz siłownik znacznika
        digitalWrite(SET_R_MARKER, HIGH);                      // wyłącz siłownik znacznika
        break;
      default:
        break;
    }

    if(DEBUG)
    {
       Serial.println("set_seeder_marker_zero, składanie znacznikow");
    }
  }
  else if(set_marker_zero == seeder_command)
  {
    // wyłącz składanie znaczników 
    digitalWrite(SET_L_MARKER, LOW);
    digitalWrite(SET_R_MARKER, LOW);
    marker_is_busy = false;
    marker_is_busy_timer = 0;
    marker_short_run_on_timer = 0;
    marker_short_run_off_timer = 0;
    seeder_command = do_nothing;
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_zero, koniec");
    }
  }
  return NONE;
}

error_code set_seeder_marker_right_or_left(uint8_t marker_get, uint8_t marker_set, const e_command seder_command_expected, int marker_type = 1)
{
  /*
   * Funkcja do opuszczania wybranego znacznika - prawy lub lewy.
   * Poniewaz brakuje krancowek przy rozłożonym znaczniku uzywany jest timer.
   * Najpierw sprawdzany jest status znacznikow, potem przez pewien czas rozkladany jest znacznik, jesli spełnione są warunki.
   * Następnie sprawdzne jest czy poprawny znacznik został rozłożony. Jeśli nie, procedura jest powtarzana. 
   * Jeśli powtórne rozkładanie nie zadziała zwracany jest kod błędu.
   */ 
  if (0 == SEEDER_ERROR &&
      !marker_is_busy &&                                       // jesli obecnie nie ma zadnej akcji na znacznikach
      !digitalRead(marker_get))                                // jesli wybrany znacznik jest zamkniety            
  {
    marker_open_timer = millis();                              // zresetuj licznik zabezpieczenia czasowego dla otwierania znaacznikow
  }
  
  if (seder_command_expected == seeder_command &&              // jesli jest komenda
     (millis() - marker_open_timer < MARKER_OPEN_WINDOW ||     // jesli licznik zabezpieczenia czasowego nie jest przekroczony lub
      marker_force_command))                                   // jest wymuszenie komendy
  {
    // włącz rozkładanie wybranego znacznika
    marker_is_busy = true;                                     // zajętość akcji na znacznikach
    switch(marker_type)
    {
      case 1:
        digitalWrite(SET_L_MARKER, HIGH);                      // wyłącz ogólny siłownik znacznika
        break;
      case 2:
        digitalWrite(marker_set, HIGH);                        // wyłącz siłownik znacznika
        break;
      default:
        break;
    }

    if(DEBUG)
    {
       Serial.println("set_seeder_marker_right_or_left, rozkładanie znacznika");
    }
  }
  else if(seder_command_expected == seeder_command)
  {
    // wyłącz składanie znaczników 
    digitalWrite(SET_L_MARKER, LOW);
    digitalWrite(SET_R_MARKER, LOW);
    marker_is_busy = false;
    marker_open_timer = 0;
    
    if(!digitalRead(marker_get))                                // jesli wybrany znacznik nadal jest zamkniety
    {
      if(marker_procedure_repetation_counter)
      {
        // ERROR
        if(set_marker_left == seeder_command) SEEDER_ERROR = ERROR_SET_RIGHT_MARKER;
        if(set_marker_right == seeder_command) SEEDER_ERROR = ERROR_SET_LEFT_MARKER;
        return SEEDER_ERROR;
      }
      marker_procedure_repetation_counter++;
      set_seeder_marker_right_or_left(marker_get, marker_set, seder_command_expected, marker_type);  // wykonaj procedure rozłożenia wybranego znacznika ponownie
    }
    
    marker_procedure_repetation_counter = 0;
    seeder_command = do_nothing;
    
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_right_or_left, koniec");
    }
  }
  return NONE;
}

error_code set_seeder_marker_right_and_left(int marker_type = 1)
{
  /*
   * Funkcja do opuszczania prawego i lewego znacznika.
   */ 
  if (0 == SEEDER_ERROR &&
      !marker_is_busy &&                                       // jesli obecnie nie ma zadnej akcji na znacznikach
      (!digitalRead(GET_L_MARKER) ||
       !digitalRead(GET_R_MARKER)))                             // jesli choć jeden znacznik jest podniesiony            
  {
    marker_open_timer = millis();                              // zresetuj licznik zabezpieczenia czasowego dla otwierania znaacznikow
    marker_short_run_on_timer = millis();
  }
  
  if (set_marker_left_right == seeder_command &&              // jesli jest komenda
     (millis() - marker_open_timer < MARKER_OPEN_WINDOW ||     // jesli licznik zabezpieczenia czasowego nie jest przekroczony lub
      marker_force_command))                                   // jest wymuszenie komendy
  {
    // włącz rozkładanie znacznika prawego i lewego
    marker_is_busy = true;                                     // zajętość akcji na znacznikach
    switch(marker_type)
    {
      case 1:
        if(millis() - marker_short_run_on_timer < MARKER_SHORT_RUN_WINDOW)
        {
          digitalWrite(SET_L_MARKER, HIGH);                    // włącz siłownik znacznika na krotka chwile
          marker_short_run_off_timer = millis();
        }
        else
        {
          if(millis() - marker_short_run_off_timer < MARKER_SHORT_RUN_WINDOW/2)
          {
            digitalWrite(SET_L_MARKER, LOW);                   // wyłącz siłownik znacznika na krotka chwile
          }
          else
          {
            digitalWrite(SET_L_MARKER, HIGH);                  // włącz siłownik znacznika aż znaczniki zostaną ustawione
          }
        }
        break;
      case 2:
        digitalWrite(SET_L_MARKER, HIGH);                      // wyłącz siłownik znacznika
        digitalWrite(SET_R_MARKER, HIGH);                      // wyłącz siłownik znacznika
        break;
      default:
        break;
    }

    if(DEBUG)
    {
       Serial.println("set_seeder_marker_right_and_left, rozkładanie znacznikow");
    }
  }
  else if(set_marker_left_right == seeder_command)
  {
    // wyłącz składanie znaczników 
    digitalWrite(SET_L_MARKER, LOW);
    digitalWrite(SET_R_MARKER, LOW);
    marker_is_busy = false;
    marker_open_timer = 0;
    
    if(!digitalRead(GET_R_MARKER) ||
       !digitalRead(GET_L_MARKER))                         // jesli wybrany znacznik nadal jest zamkniety
    {
      if(marker_procedure_repetation_counter)
      {
        // ERROR
        SEEDER_ERROR = ERROR_SET_LEFT_RIGHT_MARKER;
        return SEEDER_ERROR;
      }
      marker_procedure_repetation_counter++;
      set_seeder_marker_right_and_left();           // wykonaj procedure rozłożenia znaczników ponownie
    }
    
    marker_procedure_repetation_counter = 0;
    seeder_command = do_nothing;
    
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_right_and_left, koniec");
    }
  }
  return NONE;
}

void control_seeder()
{
  set_seeder_marker_zero();
  set_seeder_marker_right_or_left(GET_R_MARKER, SET_R_MARKER, set_marker_right);
  set_seeder_marker_right_or_left(GET_L_MARKER, SET_L_MARKER, set_marker_left);
}

String meassure_seed_level()
{
  String level = "";

  level += analogRead(GET_SEED_1);
  level += ";";
  level += analogRead(GET_SEED_2);
  level += ";";
  level += analogRead(GET_SEED_3);
  level += ";";
  level += analogRead(GET_SEED_4);
  level += ";";
  level += analogRead(GET_SEED_5);
  level += ";";
  level += analogRead(GET_SEED_6);
  if(NANO)
  {
    level += ";";
    level += analogRead(GET_SEED_7);
    level += ";";
    level += analogRead(GET_SEED_8);   
  }

  return level;
}
