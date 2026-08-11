#include <cstdio>
#include <err.h>
#include <gpiod.h>

#include "gpio.h"

#define LOW  0
#define HIGH 1

using namespace Navio;

Pin::Pin(uint8_t pin):
    _pin(pin),
    _mode(GpioModeInput),
    _chip(NULL),
    _line(NULL)
{
}

Pin::~Pin()
{
    if (_line) {
        gpiod_line_release(_line);
    }
    if (_chip) {
        gpiod_chip_close(_chip);
    }
}

bool Pin::init()
{
    char line_name[16];
    snprintf(line_name, sizeof(line_name), "GPIO%u", _pin);

    /* Search every gpiochip for the header line with this name.
       On Pi 5 the header GPIOs are provided by the RP1 (label pinctrl-rp1),
       on older boards by the BCM pinctrl - line names match on both. */
    struct gpiod_chip_iter *iter = gpiod_chip_iter_new();
    if (!iter) {
        warnx("cannot iterate gpio chips (are you root?)");
        return false;
    }

    struct gpiod_chip *chip;
    gpiod_foreach_chip(iter, chip) {
        struct gpiod_line *line = gpiod_chip_find_line(chip, line_name);
        if (line) {
            _chip = chip;
            _line = line;
            break;
        }
    }

    if (_line) {
        gpiod_chip_iter_free_noclose(iter);
    } else {
        gpiod_chip_iter_free(iter);
        warnx("gpio line %s not found", line_name);
        return false;
    }

    return _request(_mode);
}

bool Pin::_request(GpioMode mode)
{
    gpiod_line_release(_line);

    int ret;
    if (mode == GpioModeOutput) {
        ret = gpiod_line_request_output(_line, "navio", 0);
    } else {
        ret = gpiod_line_request_input(_line, "navio");
    }

    if (ret < 0) {
        warn("cannot request gpio line %u", _pin);
        return false;
    }

    return true;
}

void Pin::setMode(GpioMode mode)
{
    if (_line && _request(mode)) {
        _mode = mode;
    }
}

uint8_t Pin::read() const
{
    if (!_line) {
        return 0;
    }
    int value = gpiod_line_get_value(_line);
    return value > 0 ? 1 : 0;
}

void Pin::write(uint8_t value)
{
    if (_mode != GpioModeOutput) {
        warnx("no effect because mode is not set");
        return;
    }
    if (_line) {
        gpiod_line_set_value(_line, value == LOW ? 0 : 1);
    }
}

void Pin::toggle()
{
    write(!read());
}
