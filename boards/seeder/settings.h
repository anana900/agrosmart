/*
 * Zmienne do obsługi pomiaru prędkości siewnika za pomocą przerwania
 */
const int RPM_SEEDER_READ_WINDOW = 150;  // ms
volatile unsigned long rpm_seeder_read_window_time_past = 0;
volatile unsigned long rpm_seeder_read_window_time_now = 0;
volatile unsigned int rpm_seeder_counter = 0;

/*
 * Zmienne do obsługi pomiaru prędkości siewnika za pomocą przerwania
 */
const int RPM_ROLLER_READ_WINDOW = 150;  // ms
volatile unsigned long rpm_roller_read_window_time_past = 0;
volatile unsigned long rpm_roller_read_window_time_now = 0;
volatile unsigned int rpm_roller_counter = 0;

/*
 * Zmienne do obsługi znaczników
 */
const int MARKER_BUSY_WINDOW = 5000;
unsigned long marker_is_busy_timer = 0;
bool marker_is_busy = false;
bool marker_force_command = false;
 
/*
 * EEPROM Parametry siewnika
 */
float rpm_sensor_distance = 1; // m
int seeder_width = 1;
