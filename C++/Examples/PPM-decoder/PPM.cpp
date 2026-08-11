/*
This code is provided under the BSD license.
Copyright (c) 2014, Emlid Limited. All rights reserved.
Written by Mikhail Avkhimenia (mikhail.avkhimenia@emlid.com)
twitter.com/emlidtech || www.emlid.com || info@emlid.com

Application: PPM to PWM decoder for Raspberry Pi with Navio.

Uses the libgpiod character-device interface (works on Raspberry Pi 5).
To run this app navigate to the directory containing it and run following commands:
make
sudo ./PPM
*/

#include <gpiod.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <Navio/gpio.h>
#include "Navio/PCA9685.h"
#include "Navio/Util.h"

//================================ Options =====================================

unsigned int ppmInputGpio      = 4;      // PPM input on Navio's 2.54 header
unsigned int ppmSyncLength     = 4000;   // Length of PPM sync pause in us
unsigned int ppmChannelsNumber = 8;      // Number of channels packed in PPM
unsigned int servoFrequency    = 50;     // Servo control frequency
bool verboseOutputEnabled      = true;   // Output channels values to console

//============================ Objects & data ==================================

PCA9685 *pwm;
float channels[8];

//============================== PPM decoder ===================================

unsigned int currentChannel = 0;
uint64_t previousTime = 0; // us

void ppmOnEdge(uint64_t eventTime /* us */)
{
    uint64_t deltaTime = eventTime - previousTime;
    previousTime = eventTime;

    if (deltaTime >= ppmSyncLength) { // Sync
        currentChannel = 0;

        // RC output
        for (unsigned int i = 0; i < ppmChannelsNumber; i++)
            pwm->setPWMuS(i + 3, channels[i]); // 1st Navio RC output is 3

        // Console output
        if (verboseOutputEnabled) {
            printf("\n");
            for (unsigned int i = 0; i < ppmChannelsNumber; i++)
                printf("%4.f ", channels[i]);
        }
    }
    else if (currentChannel < ppmChannelsNumber)
        channels[currentChannel++] = deltaTime;
}

//================================== Main ======================================

using namespace Navio;

int main(int argc, char *argv[])
{
    static const uint8_t outputEnablePin = RPI_GPIO_27;

    if (check_apm()) {
        return 1;
    }

    Pin pin(outputEnablePin);

    if (pin.init()) {
        pin.setMode(Pin::GpioModeOutput);
        pin.write(0); /* drive Output Enable low */
    } else {
        fprintf(stderr, "Output Enable not set. Are you root?\n");
    }

    // Servo controller setup

    pwm = new PCA9685();
    pwm->initialize();
    pwm->setFrequency(servoFrequency);

    // PPM input setup - find the header line named GPIO<n> on any chip

    char line_name[16];
    snprintf(line_name, sizeof(line_name), "GPIO%u", ppmInputGpio);

    struct gpiod_chip *chip = NULL;
    struct gpiod_line *line = NULL;
    struct gpiod_chip_iter *iter = gpiod_chip_iter_new();
    if (!iter) {
        fprintf(stderr, "Cannot iterate gpio chips. Are you root?\n");
        return 1;
    }
    struct gpiod_chip *c;
    gpiod_foreach_chip(iter, c) {
        line = gpiod_chip_find_line(c, line_name);
        if (line) {
            chip = c;
            break;
        }
    }
    if (chip) {
        gpiod_chip_iter_free_noclose(iter);
    } else {
        gpiod_chip_iter_free(iter);
        fprintf(stderr, "PPM input line %s not found\n", line_name);
        return 1;
    }

    if (gpiod_line_request_falling_edge_events(line, "ppm-decoder") < 0) {
        perror("Cannot request falling edge events");
        return 1;
    }

    printf("Waiting for PPM signal on GPIO%u...\n", ppmInputGpio);

    // Event loop - all decoding happens in ppmOnEdge()

    struct timespec timeout = { 1, 0 };
    struct gpiod_line_event event;

    while (true) {
        int ret = gpiod_line_event_wait(line, &timeout);
        if (ret < 0) {
            perror("Event wait failed");
            return 1;
        }
        if (ret == 0)
            continue; // timeout, keep waiting
        if (gpiod_line_event_read(line, &event) < 0)
            continue;

        uint64_t us = (uint64_t)event.ts.tv_sec * 1000000ULL
                    + (uint64_t)event.ts.tv_nsec / 1000ULL;
        ppmOnEdge(us);
    }

    return 0;
}
