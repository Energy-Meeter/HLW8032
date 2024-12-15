/**
 * HLW8032.c - source file for energy meter library
 * @file 			HLW8032.c
 * @author			Habib Ullah
 * @date			14-DEC-2024
 * @version			1.0
 * 
 * Modified by Cullen Sharp 2024
 * 
 * This is a source file for a library to interact with a HLW8032 energy meter.
 */

#include "HLW8032.h"

HLW8032::HLW8032()
{ 
	// Construction deferred 
}

// Utils

void HLW8032::begin(HardwareSerial& SerialData)
{
	
	SerialID = &SerialData; 
	SerialID->begin(4800,SERIAL_8E1);   
	

	_VCoeff = (UpstrR + DwstrR) / DwstrR;  
	_CCoeff = 1.0 / (ShuntR * 1000.0);    
}

void HLW8032::SerialReadLoop()
{
	if (SerialID->available()>0)   
	{
		delay(56);

		// Get the number of bytes available to read from serial
		SerialDataLen = SerialID->available();
		
		// A full transmission is 24 bytes
		// From state reg to checksum reg
		if (SerialDataLen != TRANSMISSION_LENGTH)
		{
			// Clear the data out
			while(SerialID->read()>= 0){}
			return;
		}

		// Read bytes into an array
		for (byte a = 0; a < SerialDataLen; a++)  
		{
			SerialBuffer[a] =  SerialID->read();
		}

		// Read the Check Register
		if(SerialBuffer[1] != 0x5A) 
		{
			while(SerialID->read()>= 0){}
			return;
		}

		if(Checksum() == false)   
		{
			Serial.println("ERROR: (HLW8032): Checksum error");
			return;
		}
		
		// Raise successful read flag
		SerialRead = 1; 

		/*
			HLW8032 transmits data in big endian byte ordering where
			High byte -> middle byte -> low byte

			To convert to the proper value we apply LSL (i.e. multiply by 2^x)
			High byte is multiplied by 2^16 (LSL 16)
			Middle byte is multiplied by 2^8 (LSL 8)
			Low byte is left unchanged
		*/

		VoltageParam = 	  ((uint32_t) SerialBuffer[2] <<16)
						+ ((uint32_t) SerialBuffer[3] <<8)
				 		+ SerialBuffer[4]; 
		
		// Check if Voltage Register is done updating
		if(bitRead(SerialBuffer[20], 6) == 1)  
		{
			VoltageData = 	  ((uint32_t) SerialBuffer[5] <<16)
							+ ((uint32_t) SerialBuffer[6] <<8)
							+ SerialBuffer[7];
		}

		CurrentParam =    ((uint32_t) SerialBuffer[8] <<16)
					 	+ ((uint32_t) SerialBuffer[9] <<8)
						+ SerialBuffer[10];

		// Check if Current Register is done updating
		if(bitRead(SerialBuffer[20], 5) == 1)   
		{
			CurrentData =   ((uint32_t) SerialBuffer[11] <<16)
						  + ((uint32_t) SerialBuffer[12] <<8)
						  + SerialBuffer[13];  
		}

		PowerParam = 	  ((uint32_t) SerialBuffer[14] <<16)
						+ ((uint32_t) SerialBuffer[15] <<8)
						+ SerialBuffer[16];   

		// Check if Power Register is done updating
		if(bitRead(SerialBuffer[20], 4) == 1)   
		{
			PowerData = 	((uint32_t) SerialBuffer[17] <<16)
			 			  + ((uint32_t) SerialBuffer[18] <<8)
						  + SerialBuffer[19];    
		}

		PF = 			  ((uint16_t) SerialBuffer[21] <<8)
						+ SerialBuffer[22];
		
		// PFdata refers to the number of times that PF has overflow'd
		if(bitRead(SerialBuffer[20], 7) == 1)
		{
			PFData++;
		}
	}
}

bool HLW8032::Checksum()
{
	byte check_sum = 0, a = 2;
	for (;a <= TRANSMISSION_LENGTH-2; ++a) { check_sum += SerialBuffer[a]; }
	if (check  == SerialBuffer[23]) { return true; }

	return false;
}

// Setters

void HLW8032::setVCoeff(float VCoeff) { _VCoeff = VCoeff; }

void HLW8032::setCCoeff(float CCoeff) { _CCoeff = CCoeff; }

// Getters


voltage_t HLW8032::GetEffVoltage()
{
	float Vol = (VoltageParam / VoltageData) * _VCoeff;   
	return Vol;
} 


voltage_t HLW8032::GetDividerVoltage()
{
	float DividerVoltage = VoltageParam / VoltageData;
	return DividerVoltage;
}


current_t HLW8032::GetEffCurrent()
{
	current_t  Current = (CurrentParam / CurrentData) * _CCoeff;    
	return Current;
}


current_t HLW8032::GetShuntVoltage()
{
	voltage_t  ShuntVoltage  = CurrentParam / CurrentData;
	return ShuntVoltage;
}



power_t HLW8032::GetActivePower()
{
	power_t Power = (PowerParam/PowerData) * _VCoeff * _CCoeff;  
	return Power;
}


power_t HLW8032::GetApparentPower()
{
	voltage_t SupplyVoltage = GetEffVoltage();
	current_t LoadCurrent = GetEffCurrent();

	return SupplyVoltage * LoadCurrent;
}


unitless_t HLW8032::GetPowerFactor()
{
	power_t ActivePower = GetActivePower();   
	power_t ApparentPower = GetApparentPower();

	return ActivePower / ApparentPower;  
}


uint16_t HLW8032::GetPF() { return PF; }

uint32_t HLW8032::GetPFAll() { return PFData * PF; }

energy_t HLW8032::GetKWh()
{
	// Calculates the pulse count for one kWh
	uint32_t PulseCount = 	(1/PowerParam)
						  * (1/(_VCoeff * _CCoeff))
						  * 1000000000 * 3600;
	
	energy_t KWh = (PFData * PF) / PulseCount;  
	return KWh;

}