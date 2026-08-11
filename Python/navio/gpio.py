import glob

import gpiod
from gpiod.line import Direction, Value


def _find_line(pin):
    """Locate the header GPIO line named GPIO<pin>.

    On Raspberry Pi 5 the header GPIOs are on the RP1 gpiochip, on older
    boards on the BCM pinctrl chip - on both, lines are named "GPIO<n>".
    """
    name = "GPIO%d" % pin
    for path in sorted(glob.glob("/dev/gpiochip*")):
        try:
            with gpiod.Chip(path) as chip:
                offset = chip.line_offset_from_id(name)
                return path, offset
        except (ValueError, OSError, PermissionError):
            continue
    raise RuntimeError("GPIO line %s not found (are you root?)" % name)


class Pin():
    """GPIO pin wrapper backed by the gpiod character device."""

    def __init__(self, pin):
        self.pin = pin
        self._path, self._offset = _find_line(pin)
        self._request = gpiod.request_lines(
            self._path,
            consumer="navio",
            config={self._offset: gpiod.LineSettings(direction=Direction.INPUT)},
        )
        self._direction = Direction.INPUT

    def _set_direction(self, direction):
        if self._direction != direction:
            self._request.reconfigure_lines(
                {self._offset: gpiod.LineSettings(direction=direction)}
            )
            self._direction = direction

    def write(self, value):
        self._set_direction(Direction.OUTPUT)
        self._request.set_value(
            self._offset, Value.ACTIVE if int(value) else Value.INACTIVE
        )

    def read(self):
        return 1 if self._request.get_value(self._offset) == Value.ACTIVE else 0


if __name__ == "__main__":
    pin = Pin(27)
    pin.write(0)
