EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:HomeTether
LIBS:sensorend-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "noname.sch"
Date "23 feb 2018"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_4 P2
U 1 1 5A71CD3C
P 8750 2600
F 0 "P2" V 8700 2600 50  0000 C CNN
F 1 "IRMOTION" V 8800 2600 50  0000 C CNN
F 2 "~" H 8750 2600 60  0000 C CNN
F 3 "~" H 8750 2600 60  0000 C CNN
	1    8750 2600
	1    0    0    -1  
$EndComp
$Comp
L ESP8266-07 U2
U 1 1 5A71CD64
P 4650 5850
F 0 "U2" H 4650 5850 60  0000 C CNN
F 1 "ESP8266-07" H 4600 5400 60  0000 C CNN
F 2 "~" H 4650 5850 60  0000 C CNN
F 3 "~" H 4650 5850 60  0000 C CNN
	1    4650 5850
	-1   0    0    -1  
$EndComp
$Comp
L DGND #PWR2
U 1 1 5A71CD91
P 2650 2800
F 0 "#PWR2" H 2650 2800 40  0001 C CNN
F 1 "DGND" H 2650 2730 40  0000 C CNN
F 2 "" H 2650 2800 60  0000 C CNN
F 3 "" H 2650 2800 60  0000 C CNN
	1    2650 2800
	1    0    0    -1  
$EndComp
$Comp
L REG1117 IC1
U 1 1 5A71CDD8
P 4700 2250
F 0 "IC1" H 4400 2475 50  0000 L BNN
F 1 "REG1117" H 4500 2350 39  0000 L BNN
F 2 "burr-brown-SOT223" H 4700 2400 50  0001 C CNN
F 3 "" H 4700 2250 60  0000 C CNN
	1    4700 2250
	1    0    0    -1  
$EndComp
$Comp
L DGND #PWR10
U 1 1 5A71CEFC
P 7750 2800
F 0 "#PWR10" H 7750 2800 40  0001 C CNN
F 1 "DGND" H 7750 2730 40  0000 C CNN
F 2 "" H 7750 2800 60  0000 C CNN
F 3 "" H 7750 2800 60  0000 C CNN
	1    7750 2800
	1    0    0    -1  
$EndComp
$Comp
L R R15
U 1 1 5A71CF4C
P 8200 2200
F 0 "R15" V 8280 2200 40  0000 C CNN
F 1 "10K" V 8207 2201 40  0000 C CNN
F 2 "~" V 8130 2200 30  0000 C CNN
F 3 "~" H 8200 2200 30  0000 C CNN
	1    8200 2200
	1    0    0    -1  
$EndComp
$Comp
L CONN_3 K1
U 1 1 5A71CF96
P 5850 2150
F 0 "K1" V 5800 2150 50  0000 C CNN
F 1 "DS18B20" V 5900 2150 40  0000 C CNN
F 2 "~" H 5850 2150 60  0000 C CNN
F 3 "~" H 5850 2150 60  0000 C CNN
	1    5850 2150
	-1   0    0    1   
$EndComp
$Comp
L PNP Q1
U 1 1 5A71D07F
P 7650 2100
F 0 "Q1" H 7650 1950 60  0000 R CNN
F 1 "S8550" H 7650 2250 60  0000 R CNN
F 2 "~" H 7650 2100 60  0000 C CNN
F 3 "~" H 7650 2100 60  0000 C CNN
	1    7650 2100
	1    0    0    1   
$EndComp
$Comp
L LED D1
U 1 1 5A71D093
P 7750 2550
F 0 "D1" H 7750 2650 50  0000 C CNN
F 1 "LED" H 7750 2450 50  0000 C CNN
F 2 "~" H 7750 2550 60  0000 C CNN
F 3 "~" H 7750 2550 60  0000 C CNN
	1    7750 2550
	0    1    1    0   
$EndComp
$Comp
L R R14
U 1 1 5A71D0D4
P 7750 1600
F 0 "R14" V 7850 1650 40  0000 C CNN
F 1 "100" V 7757 1601 40  0000 C CNN
F 2 "~" V 7680 1600 30  0000 C CNN
F 3 "~" H 7750 1600 30  0000 C CNN
	1    7750 1600
	-1   0    0    1   
$EndComp
$Comp
L R R12
U 1 1 5A71D0E9
P 7450 1600
F 0 "R12" V 7550 1650 40  0000 C CNN
F 1 "10K" V 7457 1601 40  0000 C CNN
F 2 "~" V 7380 1600 30  0000 C CNN
F 3 "~" H 7450 1600 30  0000 C CNN
	1    7450 1600
	-1   0    0    1   
$EndComp
Text Label 7450 2650 2    39   ~ 0
SND
$Comp
L R R9
U 1 1 5A71DDEA
P 6350 1750
F 0 "R9" V 6450 1800 40  0000 C CNN
F 1 "10K" V 6357 1751 40  0000 C CNN
F 2 "~" V 6280 1750 30  0000 C CNN
F 3 "~" H 6350 1750 30  0000 C CNN
	1    6350 1750
	-1   0    0    1   
$EndComp
Text Label 6200 2150 0    39   ~ 0
1WR
$Comp
L CONN_3 K2
U 1 1 5A71DF01
P 6600 1950
F 0 "K2" V 6550 1950 50  0000 C CNN
F 1 "HX1838" V 6650 1950 40  0000 C CNN
F 2 "~" H 6600 1950 60  0000 C CNN
F 3 "~" H 6600 1950 60  0000 C CNN
	1    6600 1950
	-1   0    0    1   
$EndComp
$Comp
L R R11
U 1 1 5A71DF4B
P 7200 1750
F 0 "R11" V 7100 1750 40  0000 C CNN
F 1 "10K" V 7207 1751 40  0000 C CNN
F 2 "~" V 7130 1750 30  0000 C CNN
F 3 "~" H 7200 1750 30  0000 C CNN
	1    7200 1750
	-1   0    0    1   
$EndComp
Text Label 4050 5700 2    39   ~ 0
RCV
$Comp
L JUMPER JP2
U 1 1 5A71E004
P 3550 6200
F 0 "JP2" H 3550 6350 60  0000 C CNN
F 1 "PROG" H 3550 6120 40  0000 C CNN
F 2 "~" H 3550 6200 60  0000 C CNN
F 3 "~" H 3550 6200 60  0000 C CNN
	1    3550 6200
	0    -1   -1   0   
$EndComp
$Comp
L DGND #PWR6
U 1 1 5A71E0B6
P 4050 6700
F 0 "#PWR6" H 4050 6700 40  0001 C CNN
F 1 "DGND" H 4050 6630 40  0000 C CNN
F 2 "" H 4050 6700 60  0000 C CNN
F 3 "" H 4050 6700 60  0000 C CNN
	1    4050 6700
	-1   0    0    -1  
$EndComp
NoConn ~ 5350 5500
$Comp
L R R7
U 1 1 5A71E21D
P 5450 2500
F 0 "R7" V 5550 2550 40  0000 C CNN
F 1 "1K" V 5457 2501 40  0000 C CNN
F 2 "~" V 5380 2500 30  0000 C CNN
F 3 "~" H 5450 2500 30  0000 C CNN
	1    5450 2500
	-1   0    0    1   
$EndComp
$Comp
L RVAR R6
U 1 1 5A71E223
P 5450 1900
F 0 "R6" V 5530 1850 50  0000 C CNN
F 1 "LUMIN" V 5370 1960 50  0000 C CNN
F 2 "~" H 5450 1900 60  0000 C CNN
F 3 "~" H 5450 1900 60  0000 C CNN
	1    5450 1900
	1    0    0    -1  
$EndComp
$Comp
L SI4432 P1
U 1 1 5A71F4FB
P 6700 4450
F 0 "P1" V 6800 4450 -50 0000 C BNN
F 1 "SI4432" V 6700 4450 50  0000 C CNN
F 2 "" H 6700 4450 60  0000 C CNN
F 3 "" H 6700 4450 60  0000 C CNN
	1    6700 4450
	0    -1   -1   0   
$EndComp
Text Label 5350 5900 0    39   ~ 0
SCLK
Text Label 5350 6000 0    39   ~ 0
MISO
Text Label 5350 6100 0    39   ~ 0
MOSI
Text Label 6850 5050 3    39   ~ 0
SCLK
Text Label 6750 5050 3    39   ~ 0
MOSI
Text Label 6650 5050 3    39   ~ 0
MISO
NoConn ~ 6250 5050
NoConn ~ 6350 5050
NoConn ~ 6450 5050
Text Label 7150 5050 3    39   ~ 0
SDN
Text Label 5350 5800 0    39   ~ 0
IRM
$Comp
L R R8
U 1 1 5A720FD4
P 5650 5700
F 0 "R8" V 5550 5700 40  0000 C CNN
F 1 "4K7" V 5657 5701 40  0000 C CNN
F 2 "~" V 5580 5700 30  0000 C CNN
F 3 "~" H 5650 5700 30  0000 C CNN
	1    5650 5700
	0    -1   1    0   
$EndComp
Text Label 9100 5900 2    39   ~ 0
SCLK
Text Label 9900 5900 0    39   ~ 0
MOSI
$Comp
L +3.3V #PWR8
U 1 1 5A74784C
P 5200 1650
F 0 "#PWR8" H 5200 1610 30  0001 C CNN
F 1 "+3.3V" H 5200 1760 30  0000 C CNN
F 2 "" H 5200 1650 60  0000 C CNN
F 3 "" H 5200 1650 60  0000 C CNN
	1    5200 1650
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 5A7482B1
P 4750 5150
F 0 "R5" V 4850 5200 40  0000 C CNN
F 1 "10K" V 4750 5150 40  0000 C CNN
F 2 "~" V 4680 5150 30  0000 C CNN
F 3 "~" H 4750 5150 30  0000 C CNN
	1    4750 5150
	0    -1   1    0   
$EndComp
$Comp
L R R4
U 1 1 5A7482C6
P 4750 5050
F 0 "R4" V 4650 5050 40  0000 C CNN
F 1 "10K" V 4757 5051 40  0000 C CNN
F 2 "~" V 4680 5050 30  0000 C CNN
F 3 "~" H 4750 5050 30  0000 C CNN
	1    4750 5050
	0    -1   1    0   
$EndComp
$Comp
L CP1 C2
U 1 1 5A7488DB
P 4100 2550
F 0 "C2" H 4150 2650 50  0000 L CNN
F 1 "100u" H 4150 2450 50  0000 L CNN
F 2 "~" H 4100 2550 60  0000 C CNN
F 3 "~" H 4100 2550 60  0000 C CNN
	1    4100 2550
	1    0    0    -1  
$EndComp
$Comp
L CP1 C3
U 1 1 5A7488E1
P 5200 2550
F 0 "C3" H 5250 2650 50  0000 L CNN
F 1 "10u" H 5000 2450 50  0000 L CNN
F 2 "~" H 5200 2550 60  0000 C CNN
F 3 "~" H 5200 2550 60  0000 C CNN
	1    5200 2550
	1    0    0    -1  
$EndComp
$Comp
L R R13
U 1 1 5A748AC9
P 7450 2400
F 0 "R13" V 7550 2450 40  0000 C CNN
F 1 "1K" V 7457 2401 40  0000 C CNN
F 2 "~" V 7380 2400 30  0000 C CNN
F 3 "~" H 7450 2400 30  0000 C CNN
	1    7450 2400
	-1   0    0    1   
$EndComp
NoConn ~ 6150 3850
NoConn ~ 6250 3850
$Comp
L R R10
U 1 1 5A748047
P 6950 2350
F 0 "R10" V 7050 2400 40  0000 C CNN
F 1 "1K" V 6957 2351 40  0000 C CNN
F 2 "~" V 6880 2350 30  0000 C CNN
F 3 "~" H 6950 2350 30  0000 C CNN
	1    6950 2350
	-1   0    0    1   
$EndComp
Text Label 5450 2150 0    39   ~ 0
ADC
$Comp
L RJ45 J1
U 1 1 5A7526ED
P 2200 1850
F 0 "J1" H 2400 1650 39  0000 C CNN
F 1 "RJ45" H 1950 1650 39  0001 C CNN
F 2 "~" H 2200 1850 60  0000 C CNN
F 3 "~" H 2200 1850 60  0000 C CNN
	1    2200 1850
	0    -1   1    0   
$EndComp
Text Label 2750 1400 2    39   ~ 0
ADC
Text Label 3350 1500 2    39   ~ 0
IRM
Text Label 3150 1700 0    39   ~ 0
1WR
Text Label 3150 1800 0    39   ~ 0
SND
Text Label 3250 2300 0    39   ~ 0
RCV
$Comp
L +12V #PWR5
U 1 1 5A752704
P 3400 1900
F 0 "#PWR5" H 3400 1850 20  0001 C CNN
F 1 "+12V" H 3400 2000 30  0000 C CNN
F 2 "" H 3400 1900 60  0000 C CNN
F 3 "" H 3400 1900 60  0000 C CNN
	1    3400 1900
	1    0    0    -1  
$EndComp
$Comp
L CP1 C1
U 1 1 5A75270A
P 1900 6300
F 0 "C1" H 1950 6400 50  0000 L CNN
F 1 "100u" H 1950 6200 50  0000 L CNN
F 2 "~" H 1900 6300 60  0000 C CNN
F 3 "~" H 1900 6300 60  0000 C CNN
	1    1900 6300
	1    0    0    -1  
$EndComp
$Comp
L JUMPER3 JP1
U 1 1 5A752710
P 3000 1400
F 0 "JP1" H 3050 1300 40  0000 L CNN
F 1 "DBG" H 3000 1500 40  0000 C CNN
F 2 "~" H 3000 1400 60  0000 C CNN
F 3 "~" H 3000 1400 60  0000 C CNN
	1    3000 1400
	1    0    0    -1  
$EndComp
$Comp
L JUMPER3 JP3
U 1 1 5A752716
P 3600 1500
F 0 "JP3" H 3650 1400 40  0000 L CNN
F 1 "DBG" H 3600 1600 40  0000 C CNN
F 2 "~" H 3600 1500 60  0000 C CNN
F 3 "~" H 3600 1500 60  0000 C CNN
	1    3600 1500
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5A75271C
P 2900 1700
F 0 "R1" V 2980 1700 40  0000 C CNN
F 1 "0" V 2907 1701 40  0000 C CNN
F 2 "~" V 2830 1700 30  0000 C CNN
F 3 "~" H 2900 1700 30  0000 C CNN
	1    2900 1700
	0    -1   -1   0   
$EndComp
$Comp
L R R2
U 1 1 5A752722
P 2900 1800
F 0 "R2" V 2980 1800 40  0000 C CNN
F 1 "0" V 2907 1801 40  0000 C CNN
F 2 "~" V 2830 1800 30  0000 C CNN
F 3 "~" H 2900 1800 30  0000 C CNN
	1    2900 1800
	0    -1   -1   0   
$EndComp
$Comp
L R R3
U 1 1 5A752728
P 3000 2300
F 0 "R3" V 2900 2300 40  0000 C CNN
F 1 "100" V 3007 2301 40  0000 C CNN
F 2 "~" V 2930 2300 30  0000 C CNN
F 3 "~" H 3000 2300 30  0000 C CNN
	1    3000 2300
	0    -1   -1   0   
$EndComp
Text Label 3250 1400 0    39   ~ 0
TXD
Text Label 3850 1500 0    39   ~ 0
RXD
Text Label 3600 2500 0    39   ~ 0
5VIN
$Comp
L JUMPER3 JP4
U 1 1 5A7527AC
P 3600 2250
F 0 "JP4" H 3650 2150 40  0000 L CNN
F 1 "5VIN" H 3600 2350 40  0000 C CNN
F 2 "~" H 3600 2250 60  0000 C CNN
F 3 "~" H 3600 2250 60  0000 C CNN
	1    3600 2250
	0    -1   1    0   
$EndComp
$Comp
L +5V #PWR7
U 1 1 5A7528B0
P 4200 1250
F 0 "#PWR7" H 4200 1340 20  0001 C CNN
F 1 "+5V" H 4200 1340 30  0000 C CNN
F 2 "" H 4200 1250 60  0000 C CNN
F 3 "" H 4200 1250 60  0000 C CNN
	1    4200 1250
	1    0    0    -1  
$EndComp
$Comp
L +12V #PWR12
U 1 1 5A7542E4
P 8400 2450
F 0 "#PWR12" H 8400 2400 20  0001 C CNN
F 1 "+12V" H 8400 2550 30  0000 C CNN
F 2 "" H 8400 2450 60  0000 C CNN
F 3 "" H 8400 2450 60  0000 C CNN
	1    8400 2450
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR11
U 1 1 5A7542EA
P 8200 1950
F 0 "#PWR11" H 8200 1910 30  0001 C CNN
F 1 "+3.3V" H 8200 2060 30  0000 C CNN
F 2 "" H 8200 1950 60  0000 C CNN
F 3 "" H 8200 1950 60  0000 C CNN
	1    8200 1950
	1    0    0    -1  
$EndComp
Text Label 8400 2650 2    39   ~ 0
IRM
$Comp
L +12V #PWR1
U 1 1 5A7546BC
P 1900 6050
F 0 "#PWR1" H 1900 6000 20  0001 C CNN
F 1 "+12V" H 1900 6150 30  0000 C CNN
F 2 "" H 1900 6050 60  0000 C CNN
F 3 "" H 1900 6050 60  0000 C CNN
	1    1900 6050
	1    0    0    -1  
$EndComp
Text Label 2950 6050 0    39   ~ 0
5VIN
Text Label 3950 5050 2    39   ~ 0
RXD
Text Label 4050 5800 2    39   ~ 0
SND
Text Label 5350 5600 0    39   ~ 0
ADC
Text Label 9100 6000 2    39   ~ 0
MISO
$Comp
L +3.3V #PWR9
U 1 1 5A755BA7
P 5900 5050
F 0 "#PWR9" H 5900 5010 30  0001 C CNN
F 1 "+3.3V" H 5900 5160 30  0000 C CNN
F 2 "" H 5900 5050 60  0000 C CNN
F 3 "" H 5900 5050 60  0000 C CNN
	1    5900 5050
	1    0    0    -1  
$EndComp
Text Label 4050 5150 2    39   ~ 0
TXD
Text Label 6950 2600 2    39   ~ 0
RCV
Text Label 9100 5700 2    39   ~ 0
GND
Text Notes 10850 3100 2    79   ~ 0
LEFT
Text Notes 10900 3500 2    79   ~ 0
RIGHT
$Comp
L NRF24L01-M U3
U 1 1 5A753CE8
P 9500 5850
F 0 "U3" H 9500 6100 39  0000 C CNN
F 1 "NRF24L01-M" H 9500 5600 39  0000 C CNN
F 2 "~" H 11250 5600 60  0000 C CNN
F 3 "~" H 11250 5600 60  0000 C CNN
	1    9500 5850
	1    0    0    -1  
$EndComp
$Comp
L LM78M05CT U1
U 1 1 5A7CE953
P 2550 6100
F 0 "U1" H 2350 6300 40  0000 C CNN
F 1 "LM78M05CT" H 2550 6300 40  0000 L CNN
F 2 "TO-220" H 2550 6200 30  0000 C CIN
F 3 "~" H 2550 6100 60  0000 C CNN
	1    2550 6100
	1    0    0    -1  
$EndComp
$Comp
L RJ45 J2
U 1 1 5A7DD1A7
P 2250 4500
F 0 "J2" H 2450 4300 39  0000 C CNN
F 1 "RJ45" H 2000 4300 39  0001 C CNN
F 2 "~" H 2250 4500 60  0000 C CNN
F 3 "~" H 2250 4500 60  0000 C CNN
	1    2250 4500
	0    -1   1    0   
$EndComp
Text Label 2700 4250 0    39   ~ 0
RXD
Text Label 2700 4150 0    39   ~ 0
TXD
Text Label 2700 4350 0    39   ~ 0
IRM
Text Label 2700 4450 0    39   ~ 0
SND
$Comp
L +12V #PWR4
U 1 1 5A7DD1B7
P 2900 4550
F 0 "#PWR4" H 2900 4500 20  0001 C CNN
F 1 "+12V" H 2900 4650 30  0000 C CNN
F 2 "" H 2900 4550 60  0000 C CNN
F 3 "" H 2900 4550 60  0000 C CNN
	1    2900 4550
	1    0    0    -1  
$EndComp
Text Label 2700 4750 0    39   ~ 0
RCV
$Comp
L DGND #PWR3
U 1 1 5A7DD1C3
P 2700 4950
F 0 "#PWR3" H 2700 4950 40  0001 C CNN
F 1 "DGND" H 2700 4880 40  0000 C CNN
F 2 "" H 2700 4950 60  0000 C CNN
F 3 "" H 2700 4950 60  0000 C CNN
	1    2700 4950
	1    0    0    -1  
$EndComp
Text Label 2700 4650 0    39   ~ 0
5VIN
NoConn ~ 8200 5050
NoConn ~ 7700 3850
NoConn ~ 7800 3850
NoConn ~ 8100 3850
NoConn ~ 8200 3850
NoConn ~ 8500 3850
NoConn ~ 1850 2400
NoConn ~ 8300 5050
$Comp
L RFM69HCW_95 M1
U 1 1 5A8982AA
P 8050 4500
F 0 "M1" H 8100 4300 39  0000 C BNN
F 1 "RFM69HCW_95" H 8100 4000 39  0000 C CNN
F 2 "~" H 8100 4300 60  0000 C CNN
F 3 "~" H 8100 4300 60  0000 C CNN
	1    8050 4500
	0    -1   -1   0   
$EndComp
$Comp
L R R16
U 1 1 5A8ADB3E
P 3750 6350
F 0 "R16" V 3650 6350 40  0000 C CNN
F 1 "4K7" V 3757 6351 40  0000 C CNN
F 2 "~" V 3680 6350 30  0000 C CNN
F 3 "~" H 3750 6350 30  0000 C CNN
	1    3750 6350
	1    0    0    1   
$EndComp
Wire Wire Line
	7750 2300 7750 2350
Wire Wire Line
	7450 1850 7450 2150
Wire Wire Line
	4050 5900 3550 5900
Wire Wire Line
	4050 6200 4050 6700
Connection ~ 4050 6650
Wire Wire Line
	5450 2150 5450 2250
Wire Wire Line
	3550 6650 3550 6500
Wire Wire Line
	6200 2150 6350 2150
Wire Wire Line
	6350 2150 6350 2000
Wire Wire Line
	6950 1950 7100 1950
Wire Wire Line
	7100 1950 7100 2750
Wire Wire Line
	6950 2050 7200 2050
Wire Wire Line
	7200 2050 7200 2000
Wire Wire Line
	7200 1350 7200 1500
Wire Wire Line
	6200 2750 6200 2250
Connection ~ 5450 2150
Wire Wire Line
	7750 1900 7750 1850
Wire Wire Line
	3950 5050 3950 5600
Wire Wire Line
	3950 5600 4050 5600
Wire Wire Line
	4500 5050 3950 5050
Wire Wire Line
	5000 5050 5900 5050
Wire Wire Line
	5150 5050 5150 5150
Connection ~ 7450 2100
Wire Wire Line
	6950 2100 6950 2050
Connection ~ 6950 2050
Wire Wire Line
	2650 1600 3600 1600
Wire Wire Line
	2650 1500 3000 1500
Wire Wire Line
	2650 1900 3400 1900
Wire Wire Line
	2650 2000 3600 2000
Wire Wire Line
	2650 2100 2750 2100
Wire Wire Line
	2750 2100 2750 2300
Wire Wire Line
	2650 2200 2650 2800
Wire Wire Line
	4200 2250 3700 2250
Wire Wire Line
	4100 2350 4100 2250
Connection ~ 4100 2250
Wire Wire Line
	2650 2750 8400 2750
Connection ~ 2650 2750
Connection ~ 4100 2750
Wire Wire Line
	5200 1650 5200 2350
Connection ~ 5200 2250
Wire Wire Line
	5200 1650 5450 1650
Connection ~ 5200 2200
Connection ~ 4700 2750
Connection ~ 5200 2750
Connection ~ 5200 1650
Connection ~ 5450 2750
Connection ~ 6200 2750
Connection ~ 7100 2750
Wire Wire Line
	7750 2750 7750 2800
Wire Wire Line
	6950 1350 6950 1850
Wire Wire Line
	4200 1250 4200 2250
Wire Wire Line
	6200 2050 6200 1350
Wire Wire Line
	4200 1350 7750 1350
Connection ~ 7450 1350
Connection ~ 4200 1350
Connection ~ 7200 1350
Wire Wire Line
	6350 1500 6350 1350
Connection ~ 6350 1350
Connection ~ 6200 1350
Connection ~ 6950 1350
Connection ~ 7750 2750
Wire Wire Line
	8200 2450 8200 2550
Wire Wire Line
	8200 2550 8400 2550
Wire Wire Line
	1900 6050 1900 6100
Wire Wire Line
	5900 6200 5350 6200
Wire Wire Line
	5900 5050 5900 6200
Connection ~ 5150 5050
Connection ~ 5900 5700
Wire Wire Line
	1900 6650 10400 6650
Wire Wire Line
	5350 5700 5400 5700
Wire Wire Line
	5350 5900 9100 5900
Wire Wire Line
	5350 6000 9100 6000
Wire Wire Line
	5350 6100 10000 6100
Wire Wire Line
	10000 6100 10000 5900
Wire Wire Line
	10000 5900 9900 5900
Wire Wire Line
	8900 6650 8900 5700
Wire Wire Line
	8900 5700 9100 5700
Wire Wire Line
	7150 5800 9100 5800
Connection ~ 5900 5050
Wire Wire Line
	5150 5150 5000 5150
Wire Wire Line
	4500 5150 4050 5150
Wire Wire Line
	4050 5150 4050 5500
Wire Wire Line
	1900 6500 1900 6650
Connection ~ 3550 6650
Wire Wire Line
	2550 6350 2550 6650
Connection ~ 2550 6650
Connection ~ 1900 6050
Wire Wire Line
	9900 5400 9900 5700
Wire Wire Line
	5900 5400 10400 5400
Connection ~ 5900 5400
Wire Wire Line
	6550 5050 6550 5400
Connection ~ 6550 5400
Wire Wire Line
	6650 5050 6650 6000
Connection ~ 6650 6000
Wire Wire Line
	6750 5050 6750 6100
Connection ~ 6750 6100
Wire Wire Line
	6850 5050 6850 5900
Connection ~ 6850 5900
Wire Wire Line
	7150 5800 7150 5050
Wire Wire Line
	3750 6100 4050 6100
Wire Wire Line
	3950 6100 3950 6450
Wire Wire Line
	3950 6450 10100 6450
Wire Wire Line
	9900 6550 9900 6000
Wire Notes Line
	600  3250 11200 3250
Wire Wire Line
	7250 5050 7250 6650
Connection ~ 7250 6650
Wire Wire Line
	6150 5050 6150 5250
Wire Wire Line
	6150 5250 8750 5250
Connection ~ 7250 5250
Connection ~ 4200 2250
Wire Wire Line
	1900 6050 2150 6050
Wire Wire Line
	2700 4850 2700 4950
Wire Wire Line
	2700 4550 2900 4550
Wire Wire Line
	7700 5250 7700 5050
Wire Wire Line
	7800 5050 7800 6000
Connection ~ 7800 6000
Wire Wire Line
	7900 5050 7900 6100
Connection ~ 7900 6100
Wire Wire Line
	8000 5050 8000 5900
Connection ~ 8000 5900
Wire Wire Line
	8400 3850 8400 3750
Wire Wire Line
	8400 3750 8750 3750
Wire Wire Line
	8750 3750 8750 5250
Connection ~ 7700 5250
Wire Wire Line
	8400 5050 8400 5250
Connection ~ 8400 5250
Wire Wire Line
	8000 3850 8000 3650
Wire Wire Line
	8000 3650 8850 3650
Wire Wire Line
	8850 3650 8850 5400
Connection ~ 8850 5400
Connection ~ 3750 6650
Connection ~ 3950 6100
Wire Wire Line
	3750 6600 3750 6650
Wire Wire Line
	4050 6000 3850 6000
Wire Wire Line
	3850 6000 3850 6550
Wire Wire Line
	3850 6550 9900 6550
Wire Wire Line
	7900 3850 7900 3650
Wire Wire Line
	7900 3650 7550 3650
Wire Wire Line
	7550 3650 7550 6550
Connection ~ 7550 6550
Wire Wire Line
	7050 5050 7050 6550
Connection ~ 7050 6550
Text Label 9100 5800 2    39   ~ 0
SND
Wire Wire Line
	6950 5050 6950 6450
Connection ~ 6950 6450
Wire Wire Line
	8100 5050 8100 6450
Connection ~ 8100 6450
$Comp
L PNP Q2
U 1 1 5A8AF85B
P 10300 6450
F 0 "Q2" H 10300 6300 60  0000 R CNN
F 1 "S8550" H 10300 6600 60  0000 R CNN
F 2 "~" H 10300 6450 60  0000 C CNN
F 3 "~" H 10300 6450 60  0000 C CNN
	1    10300 6450
	1    0    0    1   
$EndComp
$Comp
L R R17
U 1 1 5A8AF861
P 10400 5650
F 0 "R17" V 10500 5700 40  0000 C CNN
F 1 "4K7" V 10407 5651 40  0000 C CNN
F 2 "~" V 10330 5650 30  0000 C CNN
F 3 "~" H 10400 5650 30  0000 C CNN
	1    10400 5650
	-1   0    0    1   
$EndComp
Connection ~ 9900 5400
Wire Wire Line
	10400 5900 10400 6250
Wire Wire Line
	9900 5800 10200 5800
Wire Wire Line
	10200 5800 10200 6000
Wire Wire Line
	10200 6000 10400 6000
Connection ~ 10400 6000
Connection ~ 8900 6650
$EndSCHEMATC
