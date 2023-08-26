/*
 * Joystick Mouse Controller - Reciever - by Matthew Li
 *
 * Decodes serial data from the transmitter and moves the mouse accordingly.
 *
 * The transmitter's code is in `../mouse/mouse.ino`.
 */

// Warn on clippy::nursery and clippy::pedantic
#![warn(clippy::nursery, clippy::pedantic)]

// Import stuff from the standard library to read from and write to the serial port
use std::{
    io::{BufRead, BufReader},
    time::Duration, // Duration is used to set the timeout for the serial port
};

// `inquire::Select` is used to prompt the user to select a serial port
use inquire::Select;
// `mouse_rs` is used to move the mouse
use mouse_rs::{types::keys::Keys, Mouse};
// `serialport` is used to interact with the serial port
use serialport::{available_ports, new};

// Program entry point
fn main() {
    // Get the list of available serial ports
    let ports = available_ports().unwrap();
    // Prompt the user to select a serial port
    let selected_port = Select::new(
        "select a port",
        ports.iter().map(|port| port.port_name.clone()).collect(),
    )
    .prompt()
    .unwrap();
    // Open the serial port
    let port = new(selected_port, 115_200)
        .timeout(Duration::MAX) // With a timeout of `Duration::MAX`
        .open()
        .unwrap();
    // Create a buffered reader to read from the serial port
    let mut reader = BufReader::new(port);
    // Create a new mouse object
    let mouse = Mouse::new();
    // Loop forever
    loop {
        // Create a new buffer to store the data
        let mut buffer = Vec::new();
        // Read until a `\r` is found
        reader.read_until(b'\r', &mut buffer).unwrap();
        // Convert the data to a string
        let data = String::from_utf8(buffer);
        // If the data is invalid, skip this iteration
        let data = match data {
            Ok(data) => data.to_string(),
            Err(_) => continue,
        };

        // Split the data into a vector of numbers
        let mut data = data
            .split_whitespace()
            .map(|number| number.parse::<i32>().unwrap());

        // If there are not 7 numbers, skip this iteration
        if data.clone().count() != 7 {
            continue;
        }
        // Decode the click state
        let click = match (data.next().unwrap() == 1, data.next().unwrap() == 1) {
            (false, false) => None,
            (_, true) => Some(Keys::RIGHT),
            (true, _) => Some(Keys::LEFT),
        };
        // Decode the mouse movement
        let x = {
            let mut x = (data.next().unwrap() - 512).clamp(-511, 511);
            if x.abs() < 40 { // Check for the deadzone (square with side length `40 * 2` centered at the origin `(0, 0)`)
                x = 0;
            }
            x
        };
        let y = {
            let mut y = (data.next().unwrap() - 512).clamp(-511, 511);
            if y.abs() < 40 { // Check for the deadzone (square with side length `40 * 2` centered at the origin `(0, 0)`)
                y = 0;
            }
            y
        };
        // Decode the sensitivity
        let sensitivity = data.next().unwrap();
        // Decode the scroll wheel
        let up = data.next().unwrap() == 1;
        let down = data.next().unwrap() == 1;
        let scroll = match (up, down) {
            (false, false) | (true, true) => 0,
            (true, false) => 1,
            (false, true) => -1,
        };

        // Get the original mouse position
        let position = mouse.get_position().unwrap();
        // Move the mouse by the decoded amount
        mouse
            .move_to(
                position.x + x.checked_div(sensitivity).unwrap_or(0),
                position.y + y.checked_div(sensitivity).unwrap_or(0),
            )
            .unwrap();
        // Click and scroll the mouse
        if let Some(click) = click {
            mouse.click(&click).unwrap();
        }
        mouse.wheel(scroll).unwrap();
    }
}
