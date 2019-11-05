
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
  //detachInterrupt(digitalPinToInterrupt(GET_SEEDER_RPM));
  call_interrupt(GET_SEEDER_RPM);
  unsigned long time_diff = millis() - rpm_seeder_read_window_time_past;
  //attachInterrupt(digitalPinToInterrupt(GET_SEEDER_RPM), read_rpm, FALLING);
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
  //detachInterrupt(digitalPinToInterrupt(GET_SEEDER_RPM));
  call_interrupt(GET_ROLLER_RPM);
  unsigned long time_diff = millis() - rpm_roller_read_window_time_past;
  //attachInterrupt(digitalPinToInterrupt(GET_SEEDER_RPM), read_rpm, FALLING);
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

void set_seeder_marker_zero()
{
  if (!marker_is_busy)                                            // jesli obecnie nie ma zadnej akcji na znacznikach
  {
    marker_is_busy_timer = millis();                              // zresetuj licznik zabezpieczenia czasowego dla znaacznikow
  }
  
  if (set_marker_zero == seeder_command &&                        // jesli jest komenda
      (((//digitalRead(GET_R_POINTER)  ||                         // jesli prawy znacznik NIE jest złożony lub
        digitalRead(GET_L_POINTER)) &&                            // jesli lewy znacznik NIE jest złożony oraz
      millis() - marker_is_busy_timer < MARKER_BUSY_WINDOW) ||    // jesli licznik zabezpieczenia czasowego nie jest przekroczony lub
      marker_force_command))                                      // jest wymuszenie komendy
  {
    // włącz składanie znaczników
    digitalWrite(SET_L_POINTER, HIGH);                           // wyłącz siłownik znacznika
    digitalWrite(SET_R_POINTER, HIGH);                           // wyłącz siłownik znacznika
    marker_is_busy = true;                                       // wyzeruj zajętość akcji na znacznikach
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_zero, ustawianie znacznikow");
    }
  }
  else if(set_marker_zero == seeder_command)
  {
    // wyłącz składanie znaczników
    
    digitalWrite(SET_L_POINTER, LOW);
    digitalWrite(SET_R_POINTER, LOW);
    marker_is_busy = false;
    marker_is_busy_timer = 0;
    seeder_command = do_nothing;
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_zero, koniec");
    }
  }
}

void set_seeder_marker_r()
{
  if (!marker_is_busy)                                            // jesli obecnie nie ma zadnej akcji na znacznikach
  {
    marker_is_busy_timer = millis();                              // zresetuj licznik zabezpieczenia czasowego dla znaacznikow
  }
  
  if (set_marker_right == seeder_command &&                       // jesli jest komenda
      ((!digitalRead(GET_R_POINTER) &&                             // jesli prawy znacznik jest złożony lub
      millis() - marker_is_busy_timer < MARKER_BUSY_WINDOW) ||    // jesli licznik zabezpieczenia czasowego nie jest przekroczony lub
      marker_force_command))                                      // jest wymuszenie komendy
  {
    // włącz składanie znaczników
    digitalWrite(SET_L_POINTER, HIGH);                           // wyłącz siłownik znacznika
    digitalWrite(SET_R_POINTER, HIGH);                           // wyłącz siłownik znacznika
    marker_is_busy = true;                                       // wyzeruj zajętość akcji na znacznikach
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_r, ustawianie znacznikow");
    }
  }
  else if(set_marker_zero == seeder_command)
  {
    // wyłącz składanie znaczników
    
    digitalWrite(SET_L_POINTER, LOW);
    digitalWrite(SET_R_POINTER, LOW);
    marker_is_busy = false;
    marker_is_busy_timer = 0;
    seeder_command = do_nothing;
    if(DEBUG)
    {
       Serial.println("set_seeder_marker_r, koniec");
    }
  }
}

void control_seeder()
{
  set_seeder_marker_zero();
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
