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
LIBS:htmate-cache
EELAYER 27 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 1 1
Title "htmate"
Date "16 may 2015"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L STM32F107VC U?
U 1 1 5554B9DA
P 4400 3950
F 0 "U?" H 2400 6700 60  0000 C CNN
F 1 "STM32F107VC" H 3550 1600 60  0000 C CNN
F 2 "LQFP100" H 4350 3950 50  0000 C CIN
F 3 "" H 4400 3950 60  0000 C CNN
	1    4400 3950
	1    0    0    -1  
$EndComp
$Comp
L USB-MINI-B CON?
U 1 1 5554BB51
P 2300 9450
F 0 "CON?" H 2050 9900 60  0000 C CNN
F 1 "USB-MINI-B" H 2250 8950 60  0000 C CNN
F 2 "" H 2300 9450 60  0000 C CNN
F 3 "" H 2300 9450 60  0000 C CNN
	1    2300 9450
	1    0    0    -1  
$EndComp
Text Label 1200 9150 2    39   ~ 0
VBUS
Text Label 7100 2400 0    39   ~ 0
VBUS
Text Label 1750 9300 2    39   ~ 0
DM
Text Label 1750 9450 2    39   ~ 0
DP
Text Label 7100 2500 0    39   ~ 0
FSID
Text Label 1750 9600 2    39   ~ 0
FSID
$Comp
L GND #PWR?
U 1 1 5554BDB3
P 1750 9950
F 0 "#PWR?" H 1750 9950 30  0001 C CNN
F 1 "GND" H 1750 9880 30  0001 C CNN
F 2 "" H 1750 9950 60  0000 C CNN
F 3 "" H 1750 9950 60  0000 C CNN
	1    1750 9950
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5554BE63
P 6850 2600
F 0 "R?" V 6930 2600 40  0000 C CNN
F 1 "22" V 6857 2601 40  0000 C CNN
F 2 "~" V 6780 2600 30  0000 C CNN
F 3 "~" H 6850 2600 30  0000 C CNN
	1    6850 2600
	0    1    1    0   
$EndComp
$Comp
L R R?
U 1 1 5554BE85
P 6850 2700
F 0 "R?" V 6930 2700 40  0000 C CNN
F 1 "22" V 6857 2701 40  0000 C CNN
F 2 "~" V 6780 2700 30  0000 C CNN
F 3 "~" H 6850 2700 30  0000 C CNN
	1    6850 2700
	0    1    1    0   
$EndComp
Text Label 7100 2600 0    39   ~ 0
DM
Text Label 7100 2700 0    39   ~ 0
DP
$Comp
L CONN_4 P?
U 1 1 5554C4B3
P 950 7900
F 0 "P?" V 900 7900 50  0000 C CNN
F 1 "CONN_4" V 1000 7900 50  0000 C CNN
F 2 "" H 950 7900 60  0000 C CNN
F 3 "" H 950 7900 60  0000 C CNN
	1    950  7900
	-1   0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5554C4DF
P 1400 7500
F 0 "R?" V 1480 7500 40  0000 C CNN
F 1 "R" V 1407 7501 40  0000 C CNN
F 2 "~" V 1330 7500 30  0000 C CNN
F 3 "~" H 1400 7500 30  0000 C CNN
	1    1400 7500
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5554C4EE
P 1500 7500
F 0 "R?" V 1580 7500 40  0000 C CNN
F 1 "R" V 1507 7501 40  0000 C CNN
F 2 "~" V 1430 7500 30  0000 C CNN
F 3 "~" H 1500 7500 30  0000 C CNN
	1    1500 7500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 5554C552
P 1300 8100
F 0 "#PWR?" H 1300 8100 30  0001 C CNN
F 1 "GND" H 1300 8030 30  0001 C CNN
F 2 "" H 1300 8100 60  0000 C CNN
F 3 "" H 1300 8100 60  0000 C CNN
	1    1300 8100
	1    0    0    -1  
$EndComp
$Comp
L 3V3 #PWR?
U 1 1 5554C56F
P 1300 7250
F 0 "#PWR?" H 1300 7350 40  0001 C CNN
F 1 "3V3" H 1300 7375 40  0000 C CNN
F 2 "" H 1300 7250 60  0000 C CNN
F 3 "" H 1300 7250 60  0000 C CNN
	1    1300 7250
	1    0    0    -1  
$EndComp
Wire Wire Line
	1750 9750 1750 9950
Wire Wire Line
	2850 9150 2850 9900
Connection ~ 2850 9300
Connection ~ 2850 9600
Wire Wire Line
	2850 9900 1750 9900
Connection ~ 2850 9750
Connection ~ 1750 9900
Wire Wire Line
	1300 7850 1600 7850
Wire Wire Line
	1500 7750 1500 7850
Connection ~ 1500 7850
Wire Wire Line
	1300 7950 1600 7950
Wire Wire Line
	1300 8050 1300 8100
Wire Wire Line
	1400 7750 1400 7950
Connection ~ 1400 7950
Wire Wire Line
	1300 7750 1300 7250
Wire Wire Line
	1300 7250 1500 7250
Connection ~ 1400 7250
Text Label 1600 7850 0    39   ~ 0
TMS
Text Label 1600 7950 0    39   ~ 0
TCK
Text Label 6600 2800 0    39   ~ 0
TMS
Text Label 6600 2900 0    39   ~ 0
TCK
$Comp
L C C?
U 1 1 5554C6A1
P 1850 1700
F 0 "C?" H 1850 1800 40  0000 L CNN
F 1 "104" H 1856 1615 40  0000 L CNN
F 2 "~" H 1888 1550 30  0000 C CNN
F 3 "~" H 1850 1700 60  0000 C CNN
	1    1850 1700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1850 1500 2200 1500
$Comp
L GND #PWR?
U 1 1 5554C6C9
P 1850 1900
F 0 "#PWR?" H 1850 1900 30  0001 C CNN
F 1 "GND" H 1850 1830 30  0001 C CNN
F 2 "" H 1850 1900 60  0000 C CNN
F 3 "" H 1850 1900 60  0000 C CNN
	1    1850 1900
	1    0    0    -1  
$EndComp
$Comp
L CRYSTAL X?
U 1 1 5554C752
P 1050 1400
F 0 "X?" H 1050 1550 60  0000 C CNN
F 1 "RTC" H 1050 1250 60  0000 C CNN
F 2 "~" H 1050 1400 60  0000 C CNN
F 3 "~" H 1050 1400 60  0000 C CNN
	1    1050 1400
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 5554C7EF
P 750 1600
F 0 "C?" H 750 1700 40  0000 L CNN
F 1 "10P" H 756 1515 40  0000 L CNN
F 2 "~" H 788 1450 30  0000 C CNN
F 3 "~" H 750 1600 60  0000 C CNN
	1    750  1600
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 5554C7F5
P 1350 1600
F 0 "C?" H 1350 1700 40  0000 L CNN
F 1 "10P" H 1356 1515 40  0000 L CNN
F 2 "~" H 1388 1450 30  0000 C CNN
F 3 "~" H 1350 1600 60  0000 C CNN
	1    1350 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	750  1800 1350 1800
Wire Wire Line
	1350 1800 1350 1850
$Comp
L GND #PWR?
U 1 1 5554C825
P 1350 1850
F 0 "#PWR?" H 1350 1850 30  0001 C CNN
F 1 "GND" H 1350 1780 30  0001 C CNN
F 2 "" H 1350 1850 60  0000 C CNN
F 3 "" H 1350 1850 60  0000 C CNN
	1    1350 1850
	1    0    0    -1  
$EndComp
$Comp
L CRYSTAL X?
U 1 1 5554C832
P 1050 2200
F 0 "X?" H 1050 2350 60  0000 C CNN
F 1 "8M" H 1050 2050 60  0000 C CNN
F 2 "~" H 1050 2200 60  0000 C CNN
F 3 "~" H 1050 2200 60  0000 C CNN
	1    1050 2200
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 5554C838
P 750 2400
F 0 "C?" H 750 2500 40  0000 L CNN
F 1 "10P" H 756 2315 40  0000 L CNN
F 2 "~" H 788 2250 30  0000 C CNN
F 3 "~" H 750 2400 60  0000 C CNN
	1    750  2400
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 5554C83E
P 1350 2400
F 0 "C?" H 1350 2500 40  0000 L CNN
F 1 "10P" H 1356 2315 40  0000 L CNN
F 2 "~" H 1388 2250 30  0000 C CNN
F 3 "~" H 1350 2400 60  0000 C CNN
	1    1350 2400
	1    0    0    -1  
$EndComp
Wire Wire Line
	750  2600 1350 2600
Wire Wire Line
	1350 2600 1350 2650
$Comp
L GND #PWR?
U 1 1 5554C846
P 1350 2650
F 0 "#PWR?" H 1350 2650 30  0001 C CNN
F 1 "GND" H 1350 2580 30  0001 C CNN
F 2 "" H 1350 2650 60  0000 C CNN
F 3 "" H 1350 2650 60  0000 C CNN
	1    1350 2650
	1    0    0    -1  
$EndComp
Text Label 2200 1900 2    39   ~ 0
8M_IN
Text Label 2200 2100 2    39   ~ 0
8M_OUT
Text Label 750  2200 2    39   ~ 0
8M_IN
Text Label 1350 2200 0    39   ~ 0
8MOUT
Text Label 750  1400 2    39   ~ 0
RTC_I
Text Label 1350 1400 0    39   ~ 0
RTC_O
Text Label 6600 6400 0    39   ~ 0
RTC_O
Text Label 6600 6300 0    39   ~ 0
RTC_I
Wire Wire Line
	3900 1100 4800 1100
Connection ~ 4500 1100
Connection ~ 4350 1100
Connection ~ 4200 1100
Connection ~ 4050 1100
$Comp
L 3V3 #PWR?
U 1 1 5554C9B1
P 3900 1100
F 0 "#PWR?" H 3900 1200 40  0001 C CNN
F 1 "3V3" H 3900 1225 40  0000 C CNN
F 2 "" H 3900 1100 60  0000 C CNN
F 3 "" H 3900 1100 60  0000 C CNN
	1    3900 1100
	1    0    0    -1  
$EndComp
Wire Wire Line
	4000 6800 4900 6800
Connection ~ 4150 6800
Connection ~ 4300 6800
Connection ~ 4450 6800
Connection ~ 4600 6800
Wire Wire Line
	4000 6800 4000 6850
$Comp
L GND #PWR?
U 1 1 5554CA7B
P 4000 6850
F 0 "#PWR?" H 4000 6850 30  0001 C CNN
F 1 "GND" H 4000 6780 30  0001 C CNN
F 2 "" H 4000 6850 60  0000 C CNN
F 3 "" H 4000 6850 60  0000 C CNN
	1    4000 6850
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 5554CB3D
P 2000 3050
F 0 "#PWR?" H 2000 3050 30  0001 C CNN
F 1 "GND" H 2000 2980 30  0001 C CNN
F 2 "" H 2000 3050 60  0000 C CNN
F 3 "" H 2000 3050 60  0000 C CNN
	1    2000 3050
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 5554CB4A
P 2000 2800
F 0 "C?" H 2000 2900 40  0000 L CNN
F 1 "104" H 2006 2715 40  0000 L CNN
F 2 "~" H 2038 2650 30  0000 C CNN
F 3 "~" H 2000 2800 60  0000 C CNN
	1    2000 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2200 2800 2200 3000
Wire Wire Line
	2200 3000 2000 3000
Wire Wire Line
	2000 3000 2000 3050
Wire Wire Line
	2000 2600 2200 2600
$Comp
L 3V3 #PWR?
U 1 1 5554CBDD
P 2000 2600
F 0 "#PWR?" H 2000 2700 40  0001 C CNN
F 1 "3V3" H 2000 2725 40  0000 C CNN
F 2 "" H 2000 2600 60  0000 C CNN
F 3 "" H 2000 2600 60  0000 C CNN
	1    2000 2600
	1    0    0    -1  
$EndComp
$Comp
L RDA5820 U?
U 1 1 55573959
P 12000 2000
F 0 "U?" H 12000 2050 60  0000 C CNN
F 1 "RDA5820" H 12000 1950 39  0000 C CNN
F 2 "" H 12000 2000 60  0000 C CNN
F 3 "" H 12000 2000 60  0000 C CNN
	1    12000 2000
	1    0    0    -1  
$EndComp
$Comp
L MIX3008 U?
U 1 1 55574905
P 14150 2300
F 0 "U?" H 14150 2800 60  0000 C CNN
F 1 "MIX3008" V 14150 2300 59  0000 C CNN
F 2 "" H 14150 2150 60  0000 C CNN
F 3 "" H 14150 2150 60  0000 C CNN
	1    14150 2300
	1    0    0    -1  
$EndComp
$Comp
L SPEAKER SP?
U 1 1 5557491E
P 15350 2050
F 0 "SP?" H 15250 2300 70  0000 C CNN
F 1 "SPEAKER" H 15250 1800 70  0000 C CNN
F 2 "~" H 15350 2050 60  0000 C CNN
F 3 "~" H 15350 2050 60  0000 C CNN
	1    15350 2050
	1    0    0    -1  
$EndComp
$Comp
L SPEAKER SP?
U 1 1 5557492D
P 15350 2650
F 0 "SP?" H 15250 2900 70  0000 C CNN
F 1 "SPEAKER" H 15250 2400 70  0000 C CNN
F 2 "~" H 15350 2650 60  0000 C CNN
F 3 "~" H 15350 2650 60  0000 C CNN
	1    15350 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	14700 1950 15050 1950
Wire Wire Line
	14700 2150 15050 2150
Wire Wire Line
	14700 2250 15050 2250
Wire Wire Line
	15050 2250 15050 2550
Wire Wire Line
	14700 2450 14950 2450
Wire Wire Line
	14950 2450 14950 2750
Wire Wire Line
	14950 2750 15050 2750
Wire Wire Line
	14300 3100 14400 3100
Wire Wire Line
	14400 3100 14400 3150
$Comp
L AGND #PWR?
U 1 1 55574A9F
P 14400 3150
F 0 "#PWR?" H 14400 3150 40  0001 C CNN
F 1 "AGND" H 14400 3080 50  0000 C CNN
F 2 "" H 14400 3150 60  0000 C CNN
F 3 "" H 14400 3150 60  0000 C CNN
	1    14400 3150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 55574ABD
P 14050 3150
F 0 "#PWR?" H 14050 3150 30  0001 C CNN
F 1 "GND" H 14050 3080 30  0001 C CNN
F 2 "" H 14050 3150 60  0000 C CNN
F 3 "" H 14050 3150 60  0000 C CNN
	1    14050 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	14050 3100 14050 3150
Wire Wire Line
	13400 2250 13600 2250
$Comp
L GND #PWR?
U 1 1 55574B73
P 13100 2300
F 0 "#PWR?" H 13100 2300 30  0001 C CNN
F 1 "GND" H 13100 2230 30  0001 C CNN
F 2 "" H 13100 2300 60  0000 C CNN
F 3 "" H 13100 2300 60  0000 C CNN
	1    13100 2300
	1    0    0    -1  
$EndComp
Wire Wire Line
	13100 2150 13100 2300
Wire Wire Line
	13400 2050 13600 2050
Wire Wire Line
	13600 2500 13600 2600
$Comp
L C C?
U 1 1 55574CE0
P 14650 1500
F 0 "C?" H 14650 1600 40  0000 L CNN
F 1 "1uF" H 14650 1400 40  0000 L CNN
F 2 "~" H 14688 1350 30  0000 C CNN
F 3 "~" H 14650 1500 60  0000 C CNN
	1    14650 1500
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C?
U 1 1 55574D15
P 14850 1500
F 0 "C?" H 14900 1600 40  0000 L CNN
F 1 "10uF" H 14850 1400 40  0000 L CNN
F 2 "~" H 14950 1350 30  0000 C CNN
F 3 "~" H 14850 1500 300 0000 C CNN
	1    14850 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	14300 1500 14400 1500
Wire Wire Line
	14400 1500 14400 1300
Wire Wire Line
	14400 1300 14850 1300
Connection ~ 14650 1300
Wire Wire Line
	14650 1700 14850 1700
Wire Wire Line
	14850 1700 14850 1750
$Comp
L AGND #PWR?
U 1 1 55574E54
P 14850 1750
F 0 "#PWR?" H 14850 1750 40  0001 C CNN
F 1 "AGND" H 14850 1680 50  0000 C CNN
F 2 "" H 14850 1750 60  0000 C CNN
F 3 "" H 14850 1750 60  0000 C CNN
	1    14850 1750
	1    0    0    -1  
$EndComp
$Comp
L C C?
U 1 1 55574E61
P 13700 1500
F 0 "C?" H 13700 1600 40  0000 L CNN
F 1 "1uF" H 13706 1415 40  0000 L CNN
F 2 "~" H 13738 1350 30  0000 C CNN
F 3 "~" H 13700 1500 60  0000 C CNN
	1    13700 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	13000 1300 13900 1300
Wire Wire Line
	13900 1300 13900 1500
$Comp
L GND #PWR?
U 1 1 55574EA7
P 13700 1750
F 0 "#PWR?" H 13700 1750 30  0001 C CNN
F 1 "GND" H 13700 1680 30  0001 C CNN
F 2 "" H 13700 1750 60  0000 C CNN
F 3 "" H 13700 1750 60  0000 C CNN
	1    13700 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	13700 1750 13700 1700
$Comp
L 3V3 #PWR?
U 1 1 55574EF4
P 13700 1300
F 0 "#PWR?" H 13700 1400 40  0001 C CNN
F 1 "3V3" H 13700 1425 40  0000 C CNN
F 2 "" H 13700 1300 60  0000 C CNN
F 3 "" H 13700 1300 60  0000 C CNN
	1    13700 1300
	1    0    0    -1  
$EndComp
Connection ~ 13100 2250
Wire Wire Line
	12750 2250 13000 2250
Wire Wire Line
	13000 1300 13000 2750
Connection ~ 13700 1300
$Comp
L CRYSTAL X?
U 1 1 555750E3
P 12150 3150
F 0 "X?" H 12150 3300 60  0000 C CNN
F 1 "RTC" H 12150 3000 60  0000 C CNN
F 2 "~" H 12150 3150 60  0000 C CNN
F 3 "~" H 12150 3150 60  0000 C CNN
	1    12150 3150
	0    1    1    0   
$EndComp
Wire Wire Line
	12150 2750 12150 2850
Wire Wire Line
	13600 2750 13600 3100
Wire Wire Line
	13600 3100 14050 3100
Wire Wire Line
	12150 3450 12150 3450
$Comp
L GND #PWR?
U 1 1 55575294
P 12150 3450
F 0 "#PWR?" H 12150 3450 30  0001 C CNN
F 1 "GND" H 12150 3380 30  0001 C CNN
F 2 "" H 12150 3450 60  0000 C CNN
F 3 "" H 12150 3450 60  0000 C CNN
	1    12150 3450
	1    0    0    -1  
$EndComp
NoConn ~ 11850 1250
NoConn ~ 11950 1250
Wire Wire Line
	11750 1250 11200 1250
Wire Wire Line
	11200 1250 11200 2800
$Comp
L GND #PWR?
U 1 1 55575341
P 11200 2800
F 0 "#PWR?" H 11200 2800 30  0001 C CNN
F 1 "GND" H 11200 2730 30  0001 C CNN
F 2 "" H 11200 2800 60  0000 C CNN
F 3 "" H 11200 2800 60  0000 C CNN
	1    11200 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	11250 1750 11200 1750
Connection ~ 11200 1750
Wire Wire Line
	11250 1950 11200 1950
Connection ~ 11200 1950
Wire Wire Line
	11250 2050 11200 2050
Connection ~ 11200 2050
Wire Wire Line
	11250 2150 11200 2150
Connection ~ 11200 2150
Wire Wire Line
	11250 2250 11200 2250
Connection ~ 11200 2250
Wire Wire Line
	11750 2750 11200 2750
Connection ~ 11200 2750
NoConn ~ 12050 1250
NoConn ~ 12150 1250
NoConn ~ 12250 1250
Wire Wire Line
	13000 2750 12250 2750
Connection ~ 13000 2250
NoConn ~ 11850 2750
Wire Wire Line
	11250 1850 11000 1850
Text Label 11000 1850 0    39   ~ 0
ANT
$Comp
L CONN_1 P?
U 1 1 555756CC
P 11000 1700
F 0 "P?" H 11080 1700 40  0000 L CNN
F 1 "CONN_1" H 11000 1755 30  0001 C CNN
F 2 "" H 11000 1700 60  0000 C CNN
F 3 "" H 11000 1700 60  0000 C CNN
	1    11000 1700
	0    -1   -1   0   
$EndComp
$Comp
L CSMALL C?
U 1 1 555756EA
P 12900 1550
F 0 "C?" H 12925 1600 30  0000 L CNN
F 1 "1uF" H 12925 1500 30  0000 L CNN
F 2 "~" H 12900 1550 60  0000 C CNN
F 3 "~" H 12900 1550 60  0000 C CNN
	1    12900 1550
	1    0    0    -1  
$EndComp
Wire Wire Line
	12750 2050 13200 2050
Wire Wire Line
	12750 1950 13200 1950
Wire Wire Line
	12750 2150 13100 2150
$Comp
L CSMALL C?
U 1 1 55575837
P 12750 1550
F 0 "C?" H 12775 1600 30  0000 L CNN
F 1 "1uF" H 12775 1500 30  0000 L CNN
F 2 "~" H 12750 1550 60  0000 C CNN
F 3 "~" H 12750 1550 60  0000 C CNN
	1    12750 1550
	1    0    0    -1  
$EndComp
$Comp
L CSMALL C?
U 1 1 55575847
P 13300 2250
F 0 "C?" H 13325 2300 30  0000 L CNN
F 1 "1uF" H 13325 2200 30  0000 L CNN
F 2 "~" H 13300 2250 60  0000 C CNN
F 3 "~" H 13300 2250 60  0000 C CNN
	1    13300 2250
	0    1    1    0   
$EndComp
Wire Wire Line
	12750 1750 12750 1650
Wire Wire Line
	12750 1850 12900 1850
Wire Wire Line
	12900 1850 12900 1650
Wire Wire Line
	13100 2250 13200 2250
$Comp
L CSMALL C?
U 1 1 55575B3B
P 13300 2050
F 0 "C?" H 13325 2100 30  0000 L CNN
F 1 "1uF" H 13325 2000 30  0000 L CNN
F 2 "~" H 13300 2050 60  0000 C CNN
F 3 "~" H 13300 2050 60  0000 C CNN
	1    13300 2050
	0    1    1    0   
$EndComp
$Comp
L CSMALL C?
U 1 1 55575B41
P 13300 1950
F 0 "C?" H 13325 2000 30  0000 L CNN
F 1 "1uF" H 13325 1900 30  0000 L CNN
F 2 "~" H 13300 1950 60  0000 C CNN
F 3 "~" H 13300 1950 60  0000 C CNN
	1    13300 1950
	0    1    1    0   
$EndComp
Wire Wire Line
	13400 1950 13600 1950
Text Label 13600 2500 2    39   ~ 0
P??
$Comp
L CONN_3 K?
U 1 1 55575BD8
P 12800 850
F 0 "K?" V 12750 850 50  0000 C CNN
F 1 "CONN_3" V 12850 850 40  0000 C CNN
F 2 "" H 12800 850 60  0000 C CNN
F 3 "" H 12800 850 60  0000 C CNN
	1    12800 850 
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR?
U 1 1 55575BFE
P 12800 1250
F 0 "#PWR?" H 12800 1250 30  0001 C CNN
F 1 "GND" H 12800 1180 30  0001 C CNN
F 2 "" H 12800 1250 60  0000 C CNN
F 3 "" H 12800 1250 60  0000 C CNN
	1    12800 1250
	1    0    0    -1  
$EndComp
Wire Wire Line
	12800 1200 12800 1250
Wire Wire Line
	12900 1200 12900 1450
Wire Wire Line
	12700 1200 12700 1450
Wire Wire Line
	12700 1450 12750 1450
$Comp
L FUSE F?
U 1 1 55575D9F
P 1450 9150
F 0 "F?" H 1550 9200 40  0000 C CNN
F 1 "FUSE" H 1350 9100 40  0000 C CNN
F 2 "~" H 1450 9150 60  0000 C CNN
F 3 "~" H 1450 9150 60  0000 C CNN
	1    1450 9150
	1    0    0    -1  
$EndComp
Wire Wire Line
	1700 9150 1750 9150
Wire Wire Line
	1200 9150 1200 9000
$Comp
L +5V #PWR?
U 1 1 55575E80
P 1200 9000
F 0 "#PWR?" H 1200 9090 20  0001 C CNN
F 1 "+5V" H 1200 9090 30  0000 C CNN
F 2 "" H 1200 9000 60  0000 C CNN
F 3 "" H 1200 9000 60  0000 C CNN
	1    1200 9000
	1    0    0    -1  
$EndComp
$Comp
L CONN_5X2 P?
U 1 1 55575E8F
P 3700 8650
F 0 "P?" H 3700 8950 60  0000 C CNN
F 1 "SPI-HID" V 3700 8650 50  0000 C CNN
F 2 "" H 3700 8650 60  0000 C CNN
F 3 "" H 3700 8650 60  0000 C CNN
	1    3700 8650
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 5557602D
P 6850 2400
F 0 "R?" V 6930 2400 40  0000 C CNN
F 1 "0" V 6857 2401 40  0000 C CNN
F 2 "~" V 6780 2400 30  0000 C CNN
F 3 "~" H 6850 2400 30  0000 C CNN
	1    6850 2400
	0    1    1    0   
$EndComp
Wire Wire Line
	6600 2500 7100 2500
$EndSCHEMATC
