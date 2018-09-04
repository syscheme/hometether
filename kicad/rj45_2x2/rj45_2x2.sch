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
LIBS:stm32-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "noname.sch"
Date "3 may 2015"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_5X2 P2
U 1 1 5546413C
P 4600 1200
F 0 "P2" H 4600 1500 60  0000 C CNN
F 1 "CONN_5X2" V 4600 1200 50  0000 C CNN
F 2 "" H 4600 1200 60  0000 C CNN
F 3 "" H 4600 1200 60  0000 C CNN
	1    4600 1200
	1    0    0    -1  
$EndComp
Text Label 2350 900  2    39   ~ 0
A1
Text Label 3550 900  0    39   ~ 0
A2
Text Label 2350 1000 2    39   ~ 0
A3
Text Label 3550 1000 0    39   ~ 0
A4
$Comp
L HE10-34 P1
U 1 1 55464155
P 2950 1700
F 0 "P1" H 2950 2600 70  0000 C CNN
F 1 "HE10-34" H 2950 800 70  0000 C CNN
F 2 "" H 2950 1700 60  0000 C CNN
F 3 "" H 2950 1700 60  0000 C CNN
	1    2950 1700
	1    0    0    -1  
$EndComp
Text Label 2350 1100 2    39   ~ 0
A5
Text Label 2350 1200 2    39   ~ 0
A7
Text Label 2350 1300 2    39   ~ 0
B1
Text Label 2350 1400 2    39   ~ 0
B3
Text Label 2350 1500 2    39   ~ 0
B5
Text Label 2350 1600 2    39   ~ 0
B7
Text Label 2350 1700 2    39   ~ 0
C1
Text Label 2350 1800 2    39   ~ 0
C3
Text Label 2350 1900 2    39   ~ 0
C5
Text Label 2350 2000 2    39   ~ 0
C7
Text Label 2350 2100 2    39   ~ 0
D1
Text Label 2350 2200 2    39   ~ 0
D3
Text Label 2350 2300 2    39   ~ 0
D5
Text Label 2350 2400 2    39   ~ 0
D7
Text Label 3550 1100 0    39   ~ 0
A6
Text Label 3550 1200 0    39   ~ 0
A8
Text Label 3550 1300 0    39   ~ 0
B2
Text Label 3550 1400 0    39   ~ 0
B4
Text Label 3550 1500 0    39   ~ 0
B6
Text Label 3550 1600 0    39   ~ 0
B8
Text Label 3550 1700 0    39   ~ 0
C2
Text Label 3550 1800 0    39   ~ 0
C4
Text Label 3550 1900 0    39   ~ 0
C6
Text Label 3550 2000 0    39   ~ 0
C8
Text Label 3550 2100 0    39   ~ 0
D2
Text Label 3550 2200 0    39   ~ 0
D4
Text Label 3550 2300 0    39   ~ 0
D6
Text Label 3550 2400 0    39   ~ 0
D8
Wire Wire Line
	2350 2500 2350 2750
Wire Wire Line
	2350 2650 3550 2650
Wire Wire Line
	3550 2650 3550 2500
$Comp
L GND #PWR01
U 1 1 5546418D
P 2350 2750
F 0 "#PWR01" H 2350 2750 30  0001 C CNN
F 1 "GND" H 2350 2680 30  0001 C CNN
F 2 "" H 2350 2750 60  0000 C CNN
F 3 "" H 2350 2750 60  0000 C CNN
	1    2350 2750
	1    0    0    -1  
$EndComp
Connection ~ 2350 2650
Text Label 5000 1000 0    39   ~ 0
A2
Text Label 5000 1100 0    39   ~ 0
A4
Text Label 5000 1200 0    39   ~ 0
A6
Text Label 5000 1300 0    39   ~ 0
A8
Text Label 4200 1000 2    39   ~ 0
A1
Text Label 4200 1100 2    39   ~ 0
A3
Text Label 4200 1200 2    39   ~ 0
A5
Text Label 4200 1300 2    39   ~ 0
A7
$Comp
L CONN_5X2 P4
U 1 1 55464258
P 5850 1200
F 0 "P4" H 5850 1500 60  0000 C CNN
F 1 "CONN_5X2" V 5850 1200 50  0000 C CNN
F 2 "" H 5850 1200 60  0000 C CNN
F 3 "" H 5850 1200 60  0000 C CNN
	1    5850 1200
	1    0    0    -1  
$EndComp
$Comp
L CONN_5X2 P3
U 1 1 55464267
P 4600 2150
F 0 "P3" H 4600 2450 60  0000 C CNN
F 1 "CONN_5X2" V 4600 2150 50  0000 C CNN
F 2 "" H 4600 2150 60  0000 C CNN
F 3 "" H 4600 2150 60  0000 C CNN
	1    4600 2150
	1    0    0    -1  
$EndComp
$Comp
L CONN_5X2 P5
U 1 1 55464276
P 5850 2150
F 0 "P5" H 5850 2450 60  0000 C CNN
F 1 "CONN_5X2" V 5850 2150 50  0000 C CNN
F 2 "" H 5850 2150 60  0000 C CNN
F 3 "" H 5850 2150 60  0000 C CNN
	1    5850 2150
	1    0    0    -1  
$EndComp
Text Label 4200 1950 2    39   ~ 0
C1
Text Label 4200 2050 2    39   ~ 0
C3
Text Label 4200 2150 2    39   ~ 0
C5
Text Label 4200 2250 2    39   ~ 0
C7
Text Label 5450 1950 2    39   ~ 0
D1
Text Label 5450 2050 2    39   ~ 0
D3
Text Label 5450 2150 2    39   ~ 0
D5
Text Label 5450 2250 2    39   ~ 0
D7
Text Label 5450 1000 2    39   ~ 0
B1
Text Label 5450 1100 2    39   ~ 0
B3
Text Label 5450 1200 2    39   ~ 0
B5
Text Label 5450 1300 2    39   ~ 0
B7
Text Label 6250 1000 0    39   ~ 0
B2
Text Label 6250 1100 0    39   ~ 0
B4
Text Label 6250 1200 0    39   ~ 0
B6
Text Label 6250 1300 0    39   ~ 0
B8
Text Label 5000 1950 0    39   ~ 0
C2
Text Label 5000 2050 0    39   ~ 0
C4
Text Label 5000 2150 0    39   ~ 0
C6
Text Label 5000 2250 0    39   ~ 0
C8
Text Label 6250 1950 0    39   ~ 0
D2
Text Label 6250 2050 0    39   ~ 0
D4
Text Label 6250 2150 0    39   ~ 0
D6
Text Label 6250 2250 0    39   ~ 0
D8
NoConn ~ 4200 1400
NoConn ~ 5000 1400
NoConn ~ 4200 2350
NoConn ~ 5000 2350
NoConn ~ 5450 2350
NoConn ~ 6250 2350
NoConn ~ 6250 1400
NoConn ~ 5450 1400
$EndSCHEMATC
