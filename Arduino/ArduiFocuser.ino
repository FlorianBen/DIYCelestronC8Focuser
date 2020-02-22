#include <AccelStepper.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <Vrekrer_scpi_parser.h>

// State machine.
enum focus_state { INIT, WAITC, CONFIG, TURN, ABORT, SAVE };
uint8_t state = INIT;

// Stepper driver instance.
AccelStepper stepper(AccelStepper::FULL4WIRE, 8, 10, 9, 11);

// SCPI parser instance.
SCPI_Parser my_instrument;

// Global variables.
bool raz = false;
float maxrpsValue = 128.0;
float accelerationValue = 16.0;
int turn = 2048;
uint8_t nturn = 10;
int maxPos = turn * nturn; // Maximum 10 turn by default.

void setup() {
  // Initialize serial:
  Serial.begin(9600);
  delay(1000);

  // Read EEPROM for last position
  int positionValue = EEPROM.read(0) * 256 + EEPROM.read(1);

  // Initialize serial:
  stepper.setCurrentPosition(positionValue);
  stepper.setMaxSpeed(maxrpsValue);
  stepper.setAcceleration(accelerationValue);

  // Create SCPI instrument and register commands.
  my_instrument.RegisterCommand("*IDN?", &Identify);    // Identifier string.
  my_instrument.SetCommandTreeBase("FOCus:STEPper");    // Tree Base.
  my_instrument.RegisterCommand(":ACcel", &SetAcc);  // Set motor speed.
  my_instrument.RegisterCommand(":ACcel?", &GetAcc); // Ask motor speed.
  my_instrument.RegisterCommand(":RPMspeed", &SetRPM);  // Set motor speed.
  my_instrument.RegisterCommand(":RPMspeed?", &GetRPM); // Ask motor speed.
  my_instrument.RegisterCommand(":Go", &Go);            // Do a step.
  my_instrument.RegisterCommand(":GOto", &GoTo); // Go to the given position.
  my_instrument.RegisterCommand(":Go?", &GetGoStatus);  // Is motor turning?
  my_instrument.RegisterCommand(":ABort", &AbortGo);    // Abort motion.
  my_instrument.RegisterCommand(":POSition?", &GetPos); // Ask position.
  my_instrument.RegisterCommand(":RAZPOSition",
                                &RazPos); // Set current position to zero.
  my_instrument.RegisterCommand(":MAXpos",
                                &SetMaxPosition); // Set maximum position.
  my_instrument.RegisterCommand(":MAXpos?",
                                &GetMaxPosition); // Ask maximum position.
}

// Motor loop
void loop() {
  // State Machine for motion.
  switch (state) {
  case INIT: // Useless in facts.
    state = WAITC;
    break;
  case WAITC: // Wait order from serial.
    state = WAITC;
    break;
  case CONFIG: // Config the stepper class.
    if (raz) { // Set position to zero/calibration
      stepper.setCurrentPosition(0);
      raz = false;
    }
    stepper.setMaxSpeed(maxrpsValue);
    stepper.setAcceleration(accelerationValue);
    state = SAVE;
    break;
  case SAVE: // Save stepper options to EEPROM.
    saveEEPROMpos();
    state = WAITC;
    break;
  case TURN: // Turn until position is reached.
    if (stepper.distanceToGo() != 0) {
      stepper.run();
      state = TURN;
    } else { // Then switch off coils and save new position.
      stepper.disableOutputs();
      state = SAVE;
    }
    break;
  case ABORT: // Abort motion and switch off coils.
    stepper.stop();
    stepper.disableOutputs();
    state = SAVE;
    break;
  }
}

// Serial command loop.
void serialEvent() {
  while (Serial.available()) {
    // Process incoming command.
    my_instrument.ProcessInput(Serial, "\n");
  }
}

void Identify(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  interface.println(
      "FlorianBen,Celestron Focusser,#00,v0.1"); // No check, read only.
}

void GetRPM(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  interface.println(maxrpsValue); // No check, read only.
}

void SetRPM(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (parameters.Size() > 0) { // Check if an argument is provided.
    if (state != WAITC) {      // Entry point of the state machine.
      interface.println("WAR#001");
    } else { // Update the global variable and change the state.
      maxrpsValue = constrain(String(parameters[0]).toFloat(), 0.0, 128);
      state = CONFIG;
      interface.println("ACK#000");
    }
  } else {
    interface.println("ERR#001");
  }
}

void GetAcc(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  interface.println(accelerationValue); // No check, read only.
}

void SetAcc(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (parameters.Size() > 0) { // Check if an argument is provided.
    if (state != WAITC) {      // Entry point of the state machine.
      interface.println("WAR#001");
    } else { // Update the global variable and change the state.
      accelerationValue = constrain(String(parameters[0]).toFloat(), 0.1, 128);
      state = CONFIG;
      interface.println("ACK#000");
    }
  } else {
    interface.println("ERR#001");
  }
}

void GetMaxPosition(SCPI_C commands, SCPI_P uparameters, Stream &interface) {
  interface.println(maxPos); // No check, read only.
}

void SetMaxPosition(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (parameters.Size() > 0) { // Check if an argument is provided.
    if (state != WAITC) {      // Entry point of the state machine.
      interface.println("WAR#001");
    } else { // Update the global variable and change the state.
      maxPos = constrain(String(parameters[0]).toInt(), 0.0, turn * nturn);
      state = CONFIG;
      interface.println("ACK#000");
    }
  } else {
    interface.println("ERR#001");
  }
}

void Go(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (parameters.Size() > 0) { // Check if an argument is provided.
    if (state != WAITC) {      // Entry point of the state machine.
      interface.println("WAR#001");
    } else { // Set step if < maxPos and change the state to turn.
      int step = constrain(String(parameters[0]).toInt(), -turn, turn);
      int pos = stepper.currentPosition() + step;
      if ((pos < 0) || (pos > maxPos)) {
        interface.println("WAR#002");
      } else {
        stepper.move(step);
        state = TURN;
        stepper.enableOutputs(); // Switch off the coils.
        interface.println("ACK#000");
      }
    }
  } else {
    interface.println("ERR#001");
  }
}

void GoTo(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (parameters.Size() > 0) { // Check if an argument is provided.
    if (state != WAITC) {      // Entry point of the state machine.
      interface.println("WAR#001");
    } else { // Set destination if < maxPos and change the state to turn.
      long pos = constrain(String(parameters[0]).toInt(), 0, maxPos);
      if ((pos < 0) || (pos > maxPos)) {
        interface.println("WAR#002");
      } else {
        stepper.moveTo(pos);
        state = TURN;
        stepper.enableOutputs(); // Switch off the coils.
        interface.println("ACK#000");
      }
    }
  } else {
    interface.println("ERR#001");
  }
}

void GetGoStatus(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if(state == TURN){
    interface.println("BUSY"); // No check, read only.
  } else{
    interface.println("IDLE"); // No check, read only.
  }
}

void AbortGo(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (state != TURN) { // Abort is only available if the motor turn.
    interface.println("WAR#001");
  } else {
    if (stepper.isRunning()) {
      state = ABORT;
      interface.println("ACK#000");
    }
  }
}

void GetPos(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  interface.println(stepper.currentPosition()); // No check, read only.
}

void RazPos(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  if (state != WAITC) { // Entry point of the state machine.
    interface.println("WAR#001");
  } else {
    raz = true;
    state = CONFIG;
    interface.println("ACK#000");
  }
}

void saveEEPROMpos() {
  // Lazy pos saving on 2 bit, maximum position = 65538.
  long pos = stepper.currentPosition();
  int divi = pos / 256;
  int rem = pos % 256;
  EEPROM.update(0, divi);
  EEPROM.update(1, rem);
}