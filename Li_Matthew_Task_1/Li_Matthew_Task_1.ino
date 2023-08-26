/*
 * Morse Code Tapper - by Matthew Li
 *
 * Decodes tapped morse code, sending the decoded message through serial.
 *
 * Problem:
 *     A disabled user wants to communicate with others.
 * Solution:
 *     The user can tap morse code on a button. The morse code is decoded and sent through serial.
 *     Motor control is limited, so a button is used to input morse code.
 *
 * Components:
 *     An LED is lit up if the corresponding morse code is a dash.
 *         This allows for visual verification of the inputted morse code.
 *     The buzzer provided on the ThinkerShield will also beep the corresponding morse code.
 *         This allows for audio verification of the inputted morse code.
 *     The potentiometer on the ThinkerShield controls the pitch of the sound.
 *         This allows for the user to adjust the pitch to their preference.
 *     The LDR on the ThinkerShield can be used as a silent input method.
 *         This allows for the user to input morse code without disturbing others.
 *     The button on the ThinkerShield is used to input morse code.
 *         This allows for the user to input morse code with limited motor control.
 */

/*
 * Constants and variables
 * The preprocessor directive `#define` is used to define macros that are replaced by the preprocessor before compilation.
 * `const` is used to define constants that are stored in memory. These may not be changed during the execution of the program.
 */

// Pin definitions
#define POT A5   // The potentiometer is connected to analog pin A5.
#define BUTTON 7 // The button is connected to digital pin 7.
#define LDR A4   // The LDR is connected to analog pin A4.
#define BUZZER 3 // The buzzer is connected to digital pin 3.

#define DASH_THRESHOLD 200      // The threshold for a dash in milliseconds. Any time less than this is a dot.
#define CHARACTER_TIME 350      // The time to wait before sending the character through serial.
#define WORD_TIME 1000          // The time to wait before sending a space through serial.
#define DOT '.'                 // The character for a dot.
#define DASH '-'                // The character for a dash.
#define UNKNOWN '#'             // The character for an unknown morse code.
#define CHARACTER_SEPARATOR ' ' // The character to separate characters.
#define WORD_SEPARATOR "/ "     // The character to separate words.

const int leds[] = {8, 9, 10, 11, 12, 13}; // The LEDs are connected to digital pins 8 to 13.

// The characters corresponding to the morse code.
const char characters[] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',
    '.',
    ':',
    '?',
    '!',
    '-',
    '/',
    '@',
    '(',
    ')',
};
// The morse code corresponding to the characters.
const char *morse[] = {
    "-----",
    ".----",
    "..---",
    "...--",
    "....-",
    ".....",
    "-....",
    "--...",
    "---..",
    "----.",
    ".-",
    "-...",
    "-.-.",
    "-..",
    ".",
    "..-.",
    "--.",
    "....",
    "..",
    ".---",
    "-.-",
    ".-..",
    "--",
    "-.",
    "---",
    ".--.",
    "--.-",
    ".-.",
    "...",
    "-",
    "..-",
    "...-",
    ".--",
    "-..-",
    "-.--",
    "--..",
    ".-.-.-",
    "--..",
    "..--..",
    "-.-.--",
    "-....-",
    "-..-.",
    ".--.-.",
    "-.--.",
    "-.--.-",
};

// Variables
bool ledStates[] = {false, false, false, false, false, false}; // The states of the LEDs. True if the LED is on, false if the LED is off.
String input = String();                                       // The inputted morse code. Truncated for every character.
bool lastButtonState = LOW;                                    // The last state of the button. For detecting transitions.
int ambientLight = 0;                                          // The ambient light level.

// Timing variables. These are `unsigned long` to allow for large numbers.
unsigned long timePressed = 0;       // The time the button was pressed.
unsigned long timeReleased = 0;      // The time the button was released.
unsigned long separateCharacter = 0; // The time to separate characters.
unsigned long separateWord = 0;      // The time to separate words.

// Functions
// Setup function. This is called once when the Arduino is powered on or reset.
void setup()
{
    Serial.begin(115200); // Start serial communication at 115200 baud.
    pinMode(POT, INPUT);  // Set the potentiometer as an input.
    /*
     * Set the LEDs as outputs.
     * This is done with a for loop to avoid repeating the same code.
     * It also allows for flexibility in the number of LEDs.
     * The number of LEDs is calculated by dividing the size of the array by the size of the first element.
     */
    for (int index = 0; index < sizeof(leds) / sizeof(leds[0]); index++)
    {
        pinMode(leds[index], OUTPUT);
    }
    pinMode(BUTTON, INPUT);  // Set the button as an input.
    pinMode(LDR, INPUT);     // Set the LDR as an input.
    pinMode(BUZZER, OUTPUT); // Set the buzzer as an output.

    ambientLight = analogRead(LDR) - 200; // Read the ambient light level.
    Serial.println(ambientLight);
}

// Loop function. This is called repeatedly as long as the Arduino is powered on or reset.
void loop()
{
    bool buttonState = digitalRead(BUTTON); // Read the state of the button.
    int ldrValue = analogRead(LDR);         // Read the value of the LDR.
    /*
     * If the ambient light level is less than the value of the LDR, the button is pressed.
     * This is used to allow for silent input.
     */
    if (ldrValue < ambientLight)
    {
        buttonState = HIGH; // Set the button state to high.
    }

    // If the button is pressed, turn on the LEDs and the buzzer.
    if (buttonState)
    {
        tone(BUZZER, analogRead(POT)); // Set the buzzer to the value of the potentiometer.
    }
    // Otherwise, turn off the LEDs and the buzzer.
    else
    {
        noTone(BUZZER); // Turn off the buzzer.
    }

    /*
     * Detect a transition, accounting for button bounce.
     * To detect a transition, the current state of the button is compared to the last state of the button.
     * If they are different, the time of the transition is checked to see if it is a bounce.
     * If the time the button was last pressed was more than 50 milliseconds ago, the button was pressed and not bounced.
     * If the time the button was last released was more than 50 milliseconds ago, the button was released and not bounced.
     * This prevents the button from being pressed or released multiple times when it is only pressed or released once.
     * The time of the transition is also stored to prevent the button from being pressed or released multiple times when it is only pressed or released once.
     *
     * This is the input stage of the IPO model.
     */
    if (buttonState != lastButtonState && millis() - timePressed > 50 && millis() - timeReleased > 50)
    {
        // If the button is pressed, call the press function.
        if (buttonState == HIGH)
        {
            press();
        }
        // If the button is released, call the release function.
        else
        {
            release();
        }
    }
    // Store the current state of the button for the next iteration of `void loop()`.
    lastButtonState = buttonState;

    /* If the current time is past the scheduled time to separate characters, separate the character.
     * Consider the zero value of `separateCharacter` to be a 'null' value.
     *
     * This is the process stage of the IPO model.
     */
    if (millis() > separateCharacter && separateCharacter)
    {
        // Seperate the rest of the input from the morse code that makes up the last character.
        // Take a substring of the input from the last character separator to the end of the input.
        input = input.substring(input.lastIndexOf(CHARACTER_SEPARATOR) + 1, input.length());
        // Find the index of the morse code in the array of morse codes. This is used to find the corresponding character.
        int index = find(morse, sizeof(morse) / sizeof(morse[0]), input.c_str());
        // If the morse code is not found, print the unknown character.
        // This is the output stage of the IPO model.
        if (index == -1)
        {
            Serial.print(UNKNOWN);
        }
        // Otherwise, print the corresponding character.
        else
        {
            Serial.print(characters[index]);
        }
        // For each LED, turn it on if the corresponding morse code is a dash, and turn it off if the corresponding morse code is a dot.
        for (int index = sizeof(leds) / sizeof(leds[0]); index >= 0; index--)
        {
            if (input.charAt(index) == DASH)
            {
                digitalWrite(leds[index], HIGH);
            }
            else
            {
                digitalWrite(leds[index], LOW);
            }
        }

        input += CHARACTER_SEPARATOR; // Add a character separator to the input.
        separateCharacter = 0;        // Reset the timer to separate characters.
    }

    // If the current time is past the scheduled time to separate words, separate the word.
    if (millis() > separateWord && separateWord)
    {
        input += WORD_SEPARATOR; // Add a word separator to the input.
        Serial.print(' ');       // Print a space to seperate words.
        separateWord = 0;        // Reset the timer to separate words.
    }
}

// Press function. This is called when the button is pressed (rising transition).
void press()
{
    timePressed = millis(); // Store the time the button was pressed.
    separateCharacter = 0;  // Reset the timer to separate characters.
    separateWord = 0;       // Reset the timer to separate words.
}

// Release function. This is called when the button is released (falling transition).
void release()
{
    timeReleased = millis();                                 // Store the time the button was released.
    int totalTimePressed = timeReleased - timePressed;       // Calculate the total time the button was pressed.
    input += totalTimePressed < DASH_THRESHOLD ? DOT : DASH; // Add a dot or a dash to the input depending on the total time the button was pressed. A ternary operator is used to do this in one line.

    separateCharacter = millis() + CHARACTER_TIME; // Schedule the time to separate characters.
    separateWord = millis() + WORD_TIME;           // Schedule the time to separate words.
}

/*
 * Find function. This finds the index of a value in an array.
 * A template is used to allow for any type of array.
 * The size of the array is also passed as a parameter.
 */
template <typename T>
int find(T array[], int size, T value)
{
    // Iterate through the array.
    for (int i = 0; i < size; i++)
    {
        // If the value is found, return the index.
        // `strcmp` is used to compare strings. It returns 0 if the strings are equal.
        if (!strcmp(array[i], value))
        {
            return i; // Return the index.
        }
    }
    return -1; // If the whole array is iterated through and the value is not found, return -1.
}
