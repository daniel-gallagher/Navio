CC = g++
NAVIO = ../../Navio
INCLUDES = -I ../..

all:
	$(CC) $(INCLUDES) PPM.cpp $(NAVIO)/PCA9685.cpp $(NAVIO)/I2Cdev.cpp $(NAVIO)/gpio.cpp $(NAVIO)/Util.cpp -o PPM -lgpiod -lrt -lpthread

clean:
	rm PPM
