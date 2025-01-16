/**
 * HLW8032.h - header file for energy meter library
 * @file 			HLW8032.h
 * @author			Habib Ullah
 * @date			14-DEC-2024
 * @version			1.0
 * 
 * Modified by Cullen Sharp 2024
 * 
 * This is a header file for interacting with a HLW8032 energy meter.
 * 
 * Memory map:
 * [00,00]			State Register
 * [01,01]			Check Register
 * [02,04]			Voltage Parameter Register x
 * [05,07]			Voltage Register
 * [08,10]			Current Parameter Register
 * [11,13]			Current Register
 * [14,16]			Power Parameter Register
 * [17,19]			Power Register
 * [20,20]			Data Updata Register
 * [21,22]			PF Register
 * [23,23]			Checksum Register
 */

#ifndef HLW8032_h
#define HLW8032_h

// Macroes
#define TRANSMISSION_LENGTH 24	// bytes

// Types
typedef float		voltage_t; 		// [Volt]
typedef float		current_t; 		// [Amp]
typedef float		resistance_t; 	// [Ohm]
typedef float		power_t;	  	// [Watt or VA]
typedef float		unitless_t;		// [Unitless]
typedef float		energy_t;		// [kWh]

// Check if ARDUINO IDE version is greater than 1.00
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// Serial communication library from ARDUINO
#include <SoftwareSerial.h>
#include <inttypes.h>

class HLW8032
{
	public:
		/**
		 * Defines a constructor (unused)
		 */
		HLW8032();

		/**
		 * Begins serial communication at 4800 baud
		 * And sets data, parity, and stop bits
		 * 
		 * @param SerialData serial communication instance
		 */
		void begin(HardwareSerial& SerialData);

		// Set voltage coefficient for a divider net V_in/V_o = (R2 + R1)/R1
		void setVCoeff(float VCoeff);

		// Set current coefficient for shunt resistor
		void setCCoeff(float CCoeff);  

		/**
		 * Reads 24 bytes of serial data out of the HLW8032. 
		 * Starts with the Status REG and ends with the Checksum REG. 
		 * Data is written to a serial buffer.
		 * 
		 * @note Data is read with big endian byte-ordering
		 */
		void SerialReadLoop();

		/**
		 * Calculates the effective voltage from power supply
		 * @returns the effective voltage
		 * 
		 */
		voltage_t GetEffVoltage();

		/**
		 * Calculates the output voltage of the divider network
		 * @returns output voltage of the divider network
		 */
		voltage_t GetDividerVoltage();

		/**
		 * Calculates the effective current through a shunt resistor
		 * @returns the effective current
		 * 
		 * @note This is represents the current through the load
		 */
		current_t GetEffCurrent(); 

		/**
		 * Calculates the voltage across a shunt resistor
		 * @returns the voltage across a shunt resistor
		 */
		voltage_t GetShuntVoltage();

		/**
		 * Calculates the power dissipated in the load
		 * @returns the power dissipated in the load
		 * 
		 * @note this represents the real power (i.e. power that does work)
		 */
		power_t GetActivePower();

		/**
		 * Calculates the apparent in the load
		 * @returns the apparent power in the load
		 * 
		 * @note this represents the sum of real and reactive power
		 */
		power_t GetApparentPower();

		/**
		 * Calculates the % of apparent power which is real
		 * @returns the % of apparent power which is real
		 */
		unitless_t GetPowerFactor();

		/**
		 * Calculates the energy consumed by a load in kWh
		 * @returns the energy consumed by a load in kwh
		 */
		energy_t GetKWh();

		uint16_t GetPF();
		uint32_t GetPFAll();

			
		byte transmission[24];  
		byte bytesInRXBuffer = 0; 
		bool readSuccess = 0;  
		
	private:
		/**
		 * Sums bytes 2-22 of transmision and check against the 24th (checksum)
		 * Skips State REG (0) and Check REG (1)
		 * 
		 * @returns True if the sum matches the checksum
		 */
		bool Checksum();  

		HardwareSerial *SerialID;
		unitless_t _VCoeff;
		unitless_t _CCoeff;
		resistance_t UpstrR = 1880000.0;
		resistance_t DwstrR = 1000.0;
		resistance_t ShuntR = 0.001;
		uint16_t PF;
		uint32_t PFData = 1; 
		uint32_t VoltageParam;   
		uint32_t VoltageData;
		uint32_t CurrentParam; 		
		uint32_t CurrentData; 	
		uint32_t PowerParam;         
		uint32_t PowerData;
        
};

#endif