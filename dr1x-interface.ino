const int LED       = LED_BUILTIN;
const int SQL_PIN   = 6; // pin that is polled to check SQL level.

const int EXTIO_PIN = 0;
const int EXT1_PIN  = 1;
const int EXT2_PIN  = 2;
const int PTT_PIN   = 3;

const bool EXT1_AUTOAUTO = HIGH;
const bool EXT2_AUTOAUTO = HIGH;

const bool EXT1_AUTOFM = LOW;
const bool EXT2_AUTOFM = LOW;

const bool EXT1_FMFM = HIGH;
const bool EXT2_FMFM = LOW;

const int MODE_AUTOAUTO = 0;
const int MODE_AUTOFM = 1;
const int MODE_FMFM = 2;

const int mode_delay_ms = 150;

// Mode config
int mode_standby = MODE_AUTOAUTO;
int mode_ptt     = MODE_FMFM; // Only FMFM makes sense here, but is still configurable

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

bool ptt = LOW; // set this to HIGH to trigger PTT sequence. Set to LOW to trigger reverse sequence.
int ptt_sequence = 0; // 0 = standby mode
unsigned long ptt_started = 0; // used to track ptt sequence progress

bool sql = LOW; // used to track squelch level

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SQL_PIN, INPUT_PULLUP);
  pinMode(EXTIO_PIN, OUTPUT);
  pinMode(EXT1_PIN,  OUTPUT);
  pinMode(EXT2_PIN,  OUTPUT);
  pinMode(PTT_PIN,   OUTPUT);
 
  Serial.begin(9600);
  inputString.reserve(200);
  //Serial.println("# READY");

}

void loop() {
  if (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } 
  }

  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString); 
    
    if(inputString.startsWith("PTT")) {
      handleCmdPTT(inputString);
    } else if (inputString.startsWith("MODE")) {
      handleCmdMODE(inputString);
    }

    Serial.print("\r");
    
    // clear the string:
    inputString = "";
    stringComplete = false;
    //prompt();
  }
  
  handleSQL();

  handlePTTSequence();

  // Idle a bit...
  delay(10);
}

void handleSQL() {
  bool sql_read;
  sql_read = digitalRead(SQL_PIN);
  if(sql_read != sql) {
    // Squelch level changed
    sql = sql_read;
    if(sql_read == HIGH) { // test for HIGH assumes Reverse logic on this pin
      Serial.println("SQL 0");
    } else {
      Serial.println("SQL 1");
    }
  }
}

void handleCmdPTT(String inputString) {
  if(inputString.endsWith("1\n")) {
     setPTT(HIGH);
  } else {
    setPTT(LOW);
  }
}

void handleCmdMODE(String inputString) {
  if(inputString.endsWith("AUTOAUTO\n")) {
    mode_standby = MODE_AUTOAUTO;
  } else if(inputString.endsWith("AUTOFM\n")) {
    mode_standby = MODE_AUTOFM;
  } else if(inputString.endsWith("FMFM\n")) {
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

void setPTT(bool level) {
  ptt = level;
  ptt_started = millis();
}

void handlePTTSequence() {
  // This function is run every loop iteration. It's purpose is to see if there's some pin switching that needs to be done.
  if(millis() >= (ptt_started + mode_delay_ms)) { 
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
            Serial.print("mode ");
            switch(mode_ptt) {
              case MODE_AUTOAUTO:
                Serial.println("autoauto");
                digitalWrite(EXT1_PIN, EXT1_AUTOAUTO);
                digitalWrite(EXT2_PIN, EXT2_AUTOAUTO);
                break;
              case MODE_AUTOFM:
                Serial.println("autofm");
                digitalWrite(EXT1_PIN, EXT1_AUTOFM);
                digitalWrite(EXT2_PIN, EXT2_AUTOFM);
                break;
              case MODE_FMFM:
                Serial.println("fmfm");
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
            Serial.print("mode ");
            switch(mode_standby) {
              case MODE_AUTOAUTO:
                Serial.println("autoauto");
                digitalWrite(EXT1_PIN, EXT1_AUTOAUTO);
                digitalWrite(EXT2_PIN, EXT2_AUTOAUTO);
                break;
              case MODE_AUTOFM:
                Serial.println("autofm");
                digitalWrite(EXT1_PIN, EXT1_AUTOFM);
                digitalWrite(EXT2_PIN, EXT2_AUTOFM);
                break;
              case MODE_FMFM:
                Serial.println("fmfm");
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
      Serial.print("ptt ");
      Serial.println(ptt_sequence);
      ptt_started = millis();  
    }
  }
}
