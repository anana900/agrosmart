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
const int MARKER_SHORT_RUN_WINDOW = 600;
const int MARKER_OPEN_WINDOW = 2000;
unsigned long marker_is_busy_timer = 0;
unsigned long marker_short_run_on_timer = 0;
unsigned long marker_short_run_off_timer = 0;
unsigned long marker_open_timer = 0;
bool marker_is_busy = false;
bool marker_force_command = false;
int marker_procedure_repetation_counter = 0;

/*
 * ERROR Codes
 */
 enum error_code{
  NONE,
  ERROR_SET_RIGHT_MARKER,
  ERROR_SET_LEFT_MARKER,
  ERROR_SET_LEFT_RIGHT_MARKER
 } SEEDER_ERROR(NONE);
 
/*
 * EEPROM Parametry siewnika
 */
float rpm_sensor_distance = 1; // m
int seeder_width = 1;
