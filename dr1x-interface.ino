const int LED       = LED_BUILTIN;
const int SQL_PIN   = 10; // pin that is polled to check SQL level.

const int EXTIO_PIN = 2;
const int EXT1_PIN  = 4;
const int EXT2_PIN  = 3;
const int PTT_PIN   = 5;

const bool EXT1_AUTOAUTO = HIGH;
const bool EXT2_AUTOAUTO = HIGH;

const bool EXT1_AUTOFM = LOW;
const bool EXT2_AUTOFM = LOW;

const bool EXT1_FMFM = LOW;
const bool EXT2_FMFM = HIGH;

const int MODE_AUTOAUTO = 0;
const int MODE_AUTOFM = 1;
const int MODE_FMFM = 2;

const int mode_delay_ms = 80;

// Mode config
int mode_standby = MODE_AUTOAUTO;
int mode_ptt     = MODE_FMFM; // Only FMFM makes sense here, but is still configurable

bool debug = false;

String input_string = "";         // a string to hold incoming data
boolean string_complete = false;  // whether the string is complete

bool ptt = LOW; // set this to HIGH to trigger PTT sequence. Set to LOW to trigger reverse sequence.
int ptt_sequence = 2; // initialize at 2 to get handle_ptt_sequence() to init to the configured standby mode

unsigned long ptt_sequence_started = 0; // used to track ptt sequence progress
unsigned long ptt_started = 0; // used to trig automatic keydown
unsigned long ptt_max_ms = 600000; // automatic keydown after 10 minutes

bool sql = LOW; // used to track squelch level

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SQL_PIN, INPUT_PULLUP);
  pinMode(EXTIO_PIN, OUTPUT);
  pinMode(EXT1_PIN,  OUTPUT);
  pinMode(EXT2_PIN,  OUTPUT);
  pinMode(PTT_PIN,   OUTPUT);
 
  Serial.begin(9600);
  input_string.reserve(200);
  //Serial.println("# READY");

}

void loop() {
  handle_serial_input();
  if (string_complete) {
    handle_input_string();
    string_complete = false;
    input_string = "";
  }

  handle_sql();
  handle_ptt_sequence();

  // Idle a bit...
  delay(10);
}

void handle_serial_input() {
  if (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read();
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\r') {
      // Both Carriage Return and Linefeed characters trig command parsing.
      // If someone uses both, the second will just be parsed as an empty command and ignored.
      // This ensures that all "client" configurations work
      string_complete = true;
    } else {
      // add it to the input_string:
      input_string += inChar;
    }
  }
}

void handle_input_string() {
  Serial.println(input_string); 
  
  if(input_string.startsWith("PTT")) {
    handle_cmd_ptt(input_string);
  } else if (input_string.startsWith("MODE")) {
    handle_cmd_mode(input_string);
  } else if (input_string.startsWith("DEBUG")) {
    handle_cmd_debug(input_string);
  }
}

void handle_sql() {
  bool sql_read;
  sql_read = digitalRead(SQL_PIN);
  if(sql_read != sql) {
    // Squelch level changed
    sql = sql_read;
    if(sql_read == HIGH) { // test for HIGH assumes Reverse logic on this pin
      delay(900); // hack to avoid hangs in full auto
      Serial.println("SQL 0");
    } else {
      Serial.println("SQL 1");
    }
  }
}

void handle_cmd_ptt(String input_string) {
  if(input_string.endsWith("1")) {
     set_ptt(HIGH);
  } else {
    set_ptt(LOW);
  }
}

void handle_cmd_debug(String input_string) {
  if(input_string.endsWith("1")) {
    debug = true;
  } else {
    debug = false;
  }
}

void handle_cmd_mode(String input_string) {
  if(input_string.endsWith("AUTOAUTO")) {
    mode_standby = MODE_AUTOAUTO;
  } else if(input_string.endsWith("AUTOFM")) {
    mode_standby = MODE_AUTOFM;
  } else if(input_string.endsWith("FMFM")) {
    mode_standby = MODE_FMFM;
  }
  // Reset mode if PTT is low
  if(ptt == LOW) {
    ptt_sequence = 2;
    // This introduces a race condition unless we sneak in the EXTIO_PIN here.
    digitalWrite(EXTIO_PIN, HIGH);
    // Just in case the PTT command comes along before the sequence can reset fully.
  }
}

void set_ptt(bool level) {
  ptt = level;
  if(debug) {
    Serial.print("ptt ");
    Serial.println(ptt);
  }
  ptt_sequence_started = millis();
  if(ptt == HIGH) {
    ptt_started = millis();
  }
}

void handle_ptt_sequence() {
  // This function is run every loop iteration.
  // It's purpose is to see if there's some pin switching that needs to be done.
  
  // Automatic keydown timer
  if((ptt == HIGH) && ((ptt_started + ptt_max_ms) < millis())) {
    if(debug) Serial.println("ptt 0 (auto)");
    ptt = LOW;
  }
  
  // Sequencing
  if(millis() >= (ptt_sequence_started + mode_delay_ms)) { 
    if((ptt == HIGH && ptt_sequence < 3) || (ptt == LOW && ptt_sequence > 0)) {
      
      if(ptt == HIGH) { // Going up
        ptt_sequence ++;
        switch(ptt_sequence) {
          case 1:
            // Set Ext I/O pin
            digitalWrite(EXTIO_PIN, HIGH);
            break;
          case 2:
            // Set Mode pins
            if(debug) Serial.print("mode ");
            switch(mode_ptt) {
              case MODE_AUTOAUTO:
                if(debug) Serial.println("autoauto");
                digitalWrite(EXT1_PIN, EXT1_AUTOAUTO);
                digitalWrite(EXT2_PIN, EXT2_AUTOAUTO);
                break;
              case MODE_AUTOFM:
                if(debug) Serial.println("autofm");
                digitalWrite(EXT1_PIN, EXT1_AUTOFM);
                digitalWrite(EXT2_PIN, EXT2_AUTOFM);
                break;
              case MODE_FMFM:
                if(debug) Serial.println("fmfm");
                digitalWrite(EXT1_PIN, EXT1_FMFM);
                digitalWrite(EXT2_PIN, EXT2_FMFM);
                break;
            }
            break;
          case 3:
            // Set PTT pin
            digitalWrite(PTT_PIN, HIGH);
            digitalWrite(LED, HIGH);
            break;
        }
      } else if(ptt == LOW) { // Going down
        ptt_sequence --;
        switch(ptt_sequence) {
          case 0:
            // Reset Ext I/O pin
            digitalWrite(EXTIO_PIN, LOW);
            break;
          case 1:
            // Reset Mode pins
            if(debug) Serial.print("mode ");
            switch(mode_standby) {
              case MODE_AUTOAUTO:
                if(debug) Serial.println("autoauto");
                digitalWrite(EXT1_PIN, EXT1_AUTOAUTO);
                digitalWrite(EXT2_PIN, EXT2_AUTOAUTO);
                break;
              case MODE_AUTOFM:
                if(debug) Serial.println("autofm");
                digitalWrite(EXT1_PIN, EXT1_AUTOFM);
                digitalWrite(EXT2_PIN, EXT2_AUTOFM);
                break;
              case MODE_FMFM:
                if(debug) Serial.println("fmfm");
                digitalWrite(EXT1_PIN, EXT1_FMFM);
                digitalWrite(EXT2_PIN, EXT2_FMFM);
                break;
            }
            break;
          case 2:
            // Reset PTT pin
            digitalWrite(PTT_PIN, LOW);
            digitalWrite(LED, LOW);
            break;
        }
      }
      if(debug) {
        Serial.print("ptt_sequence ");
        Serial.println(ptt_sequence);
      }
      ptt_sequence_started = millis();  
    }
  }
}
