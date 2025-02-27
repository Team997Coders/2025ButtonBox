#include <HID-Project.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 13
#define NEOPIXEL_COUNT 96

#define GROUP1_EN A5
#define GROUP2_EN A4

#define PRINT false //enable / disaable serial printing

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int buttonsRaw[20]; //raw button input in disorganized order
int orderedButtons[20]; //ordered buttons
//final states that are reported over USB

int finalButtons[20] = {
  0, 0, 0, 0, 0, 0, //reef
  0, 0, 0, 0, //level
  0, 0, //pole
  0, 0, //source
  0, 0, 0, 0, //misc
  0, //go
  0 //climb
};

//debugging map to display names of buttons along with their value
const char* orderedButtonMap[] = {
  "Reef1", "Reef2", "Reef3", "Reef4", "Reef5", "Reef6", "Level1", "Level2", "Level3", "Level4", "LeftPole", "RightPole", "LeftSource", "RightSource", "MISC1", "MISC2", "MISC3", "MISC4", "Go", "ClimbMode"
};

//orders the buttons into the desired order. specifies the index of each button in buttonsRaw in order
const int buttonOrder[20] = {
  9, 0, 1, 2, 3, 19, 7, 6, 5, 4, 16, 
  17, 15, 8, 11, 12, 13, 14, 18, 10
};

const int buttonLightOrigin[20] = {
  48, 42, 36, 30, 60, 54, //reef
  6, 12, 18, 24, //level
  72, 84, //pole
  66, 90, //source
  -1, -1, -1, -1, //misc is neglected
  78, //go
  -1 //climb is neglected
};

int fancyGo = 0;

void setup() {
  for (int i = 2; i <= 11; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  pinMode(GROUP1_EN, OUTPUT);
  pinMode(GROUP2_EN, OUTPUT);

  digitalWrite(GROUP1_EN, LOW);
  digitalWrite(GROUP2_EN, LOW);

  if (PRINT) {
    Serial.begin(9600);
  }
  else {
    Gamepad.begin();
  }

  strip.begin();
  strip.show();
}

void loop() {
  readButtons();
  orderButtons();

  if (PRINT) {
    Serial.println(" ");
  }

  getFinals();
  pixelRoutine();

  if (!PRINT) {
    send();
  }
  
  // if (PRINT) {
  //   for (int i = 0; i <= 5; i++) {
  //     Serial.println(" ");
  //   }
  // }
  
  if (PRINT) {
    delay(10);
  }
  else {
    delay(10);
  }
}


//read the raw state of the buttons through the multiplexer
void readButtons() {
  digitalWrite(GROUP1_EN, HIGH);
  delayMicroseconds(10);

  for (int i = 0; i <= 9; i++) {
    buttonsRaw[i] = digitalRead(i + 2);
  }

  digitalWrite(GROUP1_EN, LOW);

  delayMicroseconds(10);

  digitalWrite(GROUP2_EN, HIGH);
  delayMicroseconds(10);

  for (int i = 0; i <= 9; i++) {
    buttonsRaw[i + 10] = digitalRead(i + 2);
  }

  digitalWrite(GROUP2_EN, LOW);

  // for (int i = 0; i <= 9; i++) {
  //   buttonsRaw[i+10] = 1;
  // }

  if (PRINT) {
    for (int i = 0; i <= 19; i++) {
      Serial.print(buttonsRaw[i]);
      Serial.print("  ");
    }
    Serial.println(" ");
  }
}

//order the state of the buttons into the orderedButtons list
void orderButtons() {
  for (int i = 0; i <= 19; i++) {
    orderedButtons[i] = 1 - buttonsRaw[buttonOrder[i]];

    // if (PRINT) {
    //   Serial.print(orderedButtonMap[i]);
    //   Serial.print(": ");
    //   Serial.print(orderedButtons[i]);
    //   Serial.print("  ");
    // }
  }
}

void getFinals() {
  //deal with reef
  if (orderedButtons[14]) {
    for (int i = 0; i <= 5; i++) {
      if (orderedButtons[i] == 1) {
        for (int j = 0; j <= 5; j++) {
          finalButtons[j] = 0;
        }
        finalButtons[i] = 1;

        finalButtons[12] = 0;
        finalButtons[13] = 0;
      }
    }
  }

  //deal with level
  for (int i = 6; i <= 9; i++) {
    if (orderedButtons[i] == 1) {
      for (int j = 6; j <= 9; j++) {
        finalButtons[j] = 0;
      }
      finalButtons[i] = 1;

      finalButtons[12] = 0;
      finalButtons[13] = 0;
    }
  }

  //deal with left-right pole
  for (int i = 10; i <= 11; i++) {
    if (orderedButtons[i] == 1) {
      for (int j = 10; j <= 11; j++) {
        finalButtons[j] = 0;
      }
      finalButtons[i] = 1;

      finalButtons[12] = 0;
      finalButtons[13] = 0;
    }
  }

  //deal with left-right source
  for (int i = 12; i <= 13; i++) {
    if (orderedButtons[i] == 1) {
      for (int j = 12; j <= 13; j++) {
        finalButtons[j] = 0;
      }
      finalButtons[i] = 1;

      for (int j = 0; j <= 11; j++) {
        finalButtons[j] = 0;
      }
    }
  }

  //deal with MISC buttons
  for (int i = 14; i <= 17; i++) {
    finalButtons[i] = orderedButtons[i];
  }
  if (!finalButtons[14]) {
    for (int i = 0; i <= 5; i++) {
      finalButtons[i] = 0;
    }
    finalButtons[10] = 0;
    finalButtons[11] = 0;
    finalButtons[12] = 0;
    finalButtons[13] = 0;
  }
  if (!finalButtons[15]) {
    for (int i = 6; i <= 9; i++) {
      finalButtons[i] = 0;
    }
  }
  if (!finalButtons[16] && !finalButtons[17]) {
    for (int i = 0; i <= 13; i++) {
      finalButtons[i] = 0;
    }
  }

  //deal with climb
  finalButtons[19] = orderedButtons[19];

  // go
  float pulse = ((sin(millis() * 0.05) + 1) / 4) + 0.5;
  buttonLight(18, 0, 0, 0);
  if (finalButtons[16]) {
    if (finalButtons[14]) {
      if (contains(finalButtons, 0, 10, 1) || contains(finalButtons, 12, 14, 1)){
        if (contains(finalButtons, 10, 12, 1)) {
          if (contains(finalButtons, 0, 6, 1)) {
            finalButtons[18] = orderedButtons[18];
            buttonLight(18, 10 * pulse, 10 * pulse, 10 * pulse);
          }
        }
        else {
          finalButtons[18] = orderedButtons[18];
          buttonLight(18, 10 * pulse, 10 * pulse, 10 * pulse);
        }
      }
    }
    else {
      if (contains(finalButtons, 6, 10, 1)) {
        if (!contains(finalButtons, 0, 6, 1) && !contains(finalButtons, 10, 14, 1)) {
          finalButtons[18] = orderedButtons[18];
          buttonLight(18, 10 * pulse, 10 * pulse, 10 * pulse);
        }
      }
    }
  }

  //finalButtons[18] = orderedButtons[18]; //temporary, comment out above. allows all inputs

  // //print results
  // if (PRINT) {
  //   for (int i = 0; i <= 19; i++) {
  //     Serial.print(orderedButtonMap[i]);
  //     Serial.print(": ");
  //     Serial.print(finalButtons[i]);
  //     Serial.print("  ");
  //   }
  // }
}

//send over usb
void send() {
  uint32_t buttonMask = 0;

  for (int i = 0; i <= 19; i++) {
    if (finalButtons[i] == 1) {
      buttonMask |= (1UL << i);
    }
  }

  Gamepad.buttons(buttonMask);  // Send button states
  Gamepad.write();
}

void pixelRoutine() {
  float pulse = ((sin(millis() * 0.01) + 1) / 4) + 0.5;

  //spartan logo
  strip.setPixelColor(0, finalButtons[19] * 255 * pulse, 0, (255 - (finalButtons[19] * 255)) * pulse);
  strip.setPixelColor(1, finalButtons[19] * 255 * pulse, 0, (255 - (finalButtons[19] * 255)) * pulse);

  //MISC switches
  for (int i = 2; i <= 5; i++) {
    strip.setPixelColor(i, (1 - finalButtons[i + 12]) * 25 * pulse, finalButtons[i + 12] * 25 * pulse, 0);
  }

  //level
  for (int i = 6; i <= 9; i++) {
    buttonLight(i, 0, finalButtons[i] * 255 * pulse, 0);
    if (!finalButtons[i]) {
      buttonLight(i, 0, finalButtons[15] * 10 * pulse * finalButtons[16], 0);
    }
  }

  //Reef
  for (int i = 0; i <= 5; i++) {
    buttonLight(i, 0, 0, finalButtons[i] * 255 * pulse);
    if (!finalButtons[i]) {
      buttonLight(i, 0, 0, finalButtons[14] * 10 * pulse * finalButtons[16]);
    }
  }

  //Source
  for (int i = 12; i <= 13; i++) {
    buttonLight(i, finalButtons[i] * 100 * pulse, 0, finalButtons[i] * 150 * pulse);
    if (!finalButtons[i]) {
      buttonLight(i, finalButtons[14] * 7 * pulse * finalButtons[16], 0, finalButtons[14] * 10 * pulse * finalButtons[16]);
    }
  }

  //Pole
  for (int i = 10; i <= 11; i++) {
    buttonLight(i, finalButtons[i] * 255 * pulse, finalButtons[i] * 50 * pulse, 0);
    if (!finalButtons[i]) {
      buttonLight(i, finalButtons[14] * 10 * pulse * finalButtons[16], finalButtons[14] * 3 * pulse * finalButtons[16], 0);
    }
  }

  if (finalButtons[18]) {
    fancyGo = 25;
  }

  if (fancyGo > 0) {
    fancyGo--;

    int brightness = fancyGo * 5;
    buttonLight(18, brightness, brightness, brightness);

    if (fancyGo > 20) {
      buttonLight(12, 50, 50, 50);
      buttonLight(13, 50, 50, 50);
    }
    else if (fancyGo > 15) {
      buttonLight(10, 40, 40, 40);
      buttonLight(11, 40, 40, 40);
      buttonLight(6, 40, 40, 40);

      strip.setPixelColor(5, 25, 25, 25);

      finalButtons[10] = 0;
      finalButtons[11] = 0;
      finalButtons[6] = 0;

      finalButtons[12] = 0;
      finalButtons[13] = 0;
    }
    else if (fancyGo > 10) {
      buttonLight(0, 30, 30, 30);
      buttonLight(5, 30, 30, 30);
      buttonLight(7, 30, 30, 30);

      strip.setPixelColor(4, 25, 25, 25);
      strip.setPixelColor(3, 25, 25, 25);

      finalButtons[0] = 0;
      finalButtons[5] = 0;
      finalButtons[7] = 0;
    }
    else if (fancyGo > 5) {
      buttonLight(1, 20, 20, 20);
      buttonLight(4, 20, 20, 20);
      buttonLight(8, 20, 20, 20);

      strip.setPixelColor(0, 100, 100, 100);
      strip.setPixelColor(1, 100, 100, 100);
      strip.setPixelColor(2, 25, 25, 25);

      finalButtons[1] = 0;
      finalButtons[4] = 0;
      finalButtons[8] = 0;
    }
    else {
      buttonLight(2, 10, 10, 10);
      buttonLight(3, 10, 10, 10);
      buttonLight(9, 10, 10, 10);

      finalButtons[2] = 0;
      finalButtons[3] = 0;
      finalButtons[9] = 0;
    }
  }

  strip.show();
}


//custom functions below

int mod(int a, int b) {
  return (a % b + b) % b;  // Ensures result is always positive
}

bool contains(int arr[], int startIndex, int size, int target) {
  for (int i = startIndex; i < size; i++) {
    if (arr[i] == target) {
      return true;  // Found the value
    }
  }
  return false;  // Not found
}

void buttonLight(int button, int r, int g, int b) {
  for (int i = 0; i <= 5; i++) {
    strip.setPixelColor(i + buttonLightOrigin[button], r, g, b);
  }
}
