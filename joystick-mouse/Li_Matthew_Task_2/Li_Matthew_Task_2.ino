/*
 * Joystick Mouse Controller - Transmitter - by Matthew Li
 *
 * Coordinates inputs and sends relevant data to the Rust program on the computer via serial.
 *
 * Problem:
 *     A disabled user is unable to use a computer mouse due to limited mobility.
 * Solution:
 *     The user can use a joystick to control the mouse cursor and many other mouse functionality.
 *
 * The data protocol used to communicate peripheral status to the host is composed of seven numbers which are separated by spaces (ASCII `0x20` or `32` in decimal). The numbers are as follows:
 *    1. `left_click`: `1` if the left mouse button is pressed, `0` otherwise.
 *    2. `right_click`: `1` if the right mouse button is pressed, `0` otherwise.
 *    3. `x`: The x-coordinate of the joystick.
 *    4. `y`: The y-coordinate of the joystick.
 *    5. `sensitivity`: The sensitivity of the joystick. This is used to scale the x and y coordinates.
 *    6. `up`: `1` if the scroll up button is pressed, `0` otherwise.
 *    7. `down`: `1` if the scroll down button is pressed, `0` otherwise.
 *
 * See `../src/main.rs` for the Rust program that receives the data.
 *
 * Components:
 *     A joystick is used to control the mouse cursor.
 *         The button on the joystick is used to left click. Right click is emulated by holding the button for a certain amount of time (RIGHT_CLICK_THRESHOLD).
 *     A relay is used to make an audible and tactile click when the button is held long enough to right click.
 *         The LED on the relay is used to indicate whether the right click is active.
 *     An LED indicates power status.
 *     On the ThinkerShield:
 *         The RESET button is used to scroll down.
 *         The D7 button is used to scroll up.
 *         The potentiometer is used to adjust the sensitivity of the joystick.
 */

/*
 * Constants and variables
 * The preprocessor directive `#define` is used to define macros that are replaced by the preprocessor before compilation.
 */

// Pin definitions
#define CLICK 8        // The button on the joystick (pin 5, SW) is connected to digital pin 8.
#define RIGHT_CLICK 9  // The relay (pin 1, S) is connected to digital pin 9.
#define X A4           // The x-coordinate of the joystick (pin 3, VRx) is connected to analog pin A4.
#define Y A5           // The y-coordinate of the joystick (pin 4, VRy) is connected to analog pin A5.
#define SENSITIVITY A3 // The potentiometer is connected to analog pin A3.
#define UP 10          // The D7 button is connected to digital pin 10.
#define DOWN 11        // The RESET button is connected to digital pin 11.

#define RIGHT_CLICK_THRESHOLD 1000 // The amount of time the button must be held to right click.

// Variables
bool lastState = false;         // The last state of the button. For detecting transitions.
unsigned long lastPressed = 0;  // The last time the button was pressed.
unsigned long lastReleased = 0; // The last time the button was released.
bool rightClick = false;        // Whether the right click is active.
bool lastRightClick = false;    // The last state of the right click. For detecting transitions.

void setup()
{
    pinMode(RIGHT_CLICK, OUTPUT); // The relay is an output.
    pinMode(CLICK, INPUT_PULLUP); // The button is an input with a pullup resistor. This needs to be pulled up to 5V because the button on the joystick sinks current to ground when pressed.
    pinMode(X, INPUT);            // The joystick's X axis potentiometer is an input.
    pinMode(Y, INPUT);            // The joystick's Y axis potentiometer is an input.
    pinMode(SENSITIVITY, INPUT);  // The potentiometer is an input.
    pinMode(UP, INPUT);           // The D7 button is an input.
    pinMode(DOWN, INPUT_PULLUP);  // The RESET button is an input with a pullup resistor. This needs to be pulled up to 5V because the button on the joystick sinks current to ground when pressed.
    Serial.begin(115200);         // The serial port is used to communicate with the host.
}

void loop()
{
    bool state = !digitalRead(CLICK); // The button is active low.

    // Start of the input stage of the IPO model.
    // If the button is pressed, update the last pressed time.
    if (state != lastState)
    {
        if (state)
        {
            lastPressed = millis();
        }
        else
        {
            lastReleased = millis();
            // Additionally, if the button is released, turn off the right click and signal a right click.
            digitalWrite(RIGHT_CLICK, LOW);
            rightClick = false;
        }
    }

    // If the time since the button was last pressed is greater than the threshold and the button is pressed, turn on the right click.
    if (millis() - lastPressed > RIGHT_CLICK_THRESHOLD && state)
    {
        digitalWrite(RIGHT_CLICK, HIGH); // Signal a right click.
        rightClick = true;
    }

    // Send the data to the host. The protocol is described in detail above in lines 11-18. This is the processing stage of the IPO model.
    char out[64];               // Create a character buffer to store the output.
    sprintf(                    // Format the output.
        out,                    // Write the output to the buffer.
        "%u %u %u %u %u %u %u", // The format string. `%u` is replaced by an unsigned integer.
        /*
         * The left click has just been pressed if
         * - the button is not pressed,
         * - the button was pressed before,
         * - the right click was not pressed before, and
         * - the right click is not pressed now.
         */
        !state && !rightClick && lastState && !lastRightClick,
        /*
         * The right click has just been pressed if
         * - the button is not pressed,
         * - the button was pressed before, and
         * - the right click was pressed before.
         */
        lastState && !state && lastRightClick,
        analogRead(X),           // The x-coordinate of the joystick.
        analogRead(Y),           // The y-coordinate of the joystick.
        analogRead(SENSITIVITY), // The sensitivity of the joystick.
        digitalRead(UP),         // The scroll up button.
        !digitalRead(DOWN));     // The scroll down button.
    Serial.println(out);         // Send the output to the host. This is the output stage of the IPO model.

    lastState = state;           // Update the last state.
    lastRightClick = rightClick; // Update the last right click state.
    delay(20);                   // Wait for 20 milliseconds to debounce the button and reduce stress on the host.
}
