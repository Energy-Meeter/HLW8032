/**
 * HLW8032.c - source file for energy meter library
 * @file 			HLW8032.c
 * @author			Habib Ullah
 * @date			14-DEC-2024
 * @version			1.0
 * 
 * Modified by Cullen Sharp 2024
 * Optimizing/Debugging by Derek Hernandez 2025
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
		// The HLW8032 transmits data every 50ms
		delay(56);

		// Get the number of bytes available to read from serial
		bytesInRXBuffer = SerialID->available();
		
		// If there aren't enough bytes in the RX buffer
		if (bytesInRXBuffer < TRANSMISSION_LENGTH)
		{
			// Empty the buffer
			while(SerialID->read()>= 0){}
			return;
		}

		// Read bytes into an 
		for (byte a = 0; a < TRANSMISSION_LENGTH; a++)  
		{
			transmission[a] =  SerialID->read();
		}

		// Read the Check Register
		if(transmission[1] != 0x5A) 
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
		readSuccess = 1; 

		/*
			HLW8032 transmits data in big endian byte ordering where
			High byte -> middle byte -> low byte

			To convert to the proper value we apply LSL (i.e. multiply by 2^x)
			High byte is multiplied by 2^16 (LSL 16)
			Middle byte is multiplied by 2^8 (LSL 8)
			Low byte is left unchanged
		*/
		VoltageParam = 	  ((uint32_t) transmission[2] <<16)
						+ ((uint32_t) transmission[3] <<8)
				 		+ transmission[4]; 
		
		// Check if Voltage Register is done updating
		if(bitRead(transmission[20], 6) == 1)  
		{
			VoltageData = 	  ((uint32_t) transmission[5] <<16)
							+ ((uint32_t) transmission[6] <<8)
							+ transmission[7];
		}

		CurrentParam =    ((uint32_t) transmission[8] <<16)
					 	+ ((uint32_t) transmission[9] <<8)
						+ transmission[10];

		// Check if Current Register is done updating
		if(bitRead(transmission[20], 5) == 1)   
		{
			CurrentData =   ((uint32_t) transmission[11] <<16)
						  + ((uint32_t) transmission[12] <<8)
						  + transmission[13];  
		}

		PowerParam = 	  ((uint32_t) transmission[14] <<16)
						+ ((uint32_t) transmission[15] <<8)
						+ transmission[16];   

		// Check if Power Register is done updating
		if(bitRead(transmission[20], 4) == 1)   
		{
			PowerData = 	((uint32_t) transmission[17] <<16)
			 			  + ((uint32_t) transmission[18] <<8)
						  + transmission[19];    
		}

		PF = 			  ((uint16_t) transmission[21] <<8)
						+ transmission[22];
		
		// PFdata refers to the number of times that PF has overflow'd
		if(bitRead(transmission[20], 7) == 1)
		{
			PFData++;
		}
	}
}

bool HLW8032::Checksum()
{
	byte check_sum = 0, a = 2;
	for (;a <= TRANSMISSION_LENGTH-2; ++a) { check_sum += transmission[a]; }
	byte check = transmission[23];
	if (check_sum == check) { return true; }

	return false;
}

//Check for division by zero, and return 0 if true
float checkDivisionOfZero(float denominator, float numerator)
{
	if (denominator == 0){
		Serial.println("WARNING: (HLW8032): Division by zero. Set to -1 to avoid errors");
		return -1;
	}
	else{
	return numerator / denominator;
	}
}

// Setters

void HLW8032::setVCoeff(float VCoeff) { _VCoeff = VCoeff; }

void HLW8032::setCCoeff(float CCoeff) { _CCoeff = CCoeff; }

// Getters


voltage_t HLW8032::GetEffVoltage()
{
	float Vol = checkDivisionOfZero(VoltageParam , VoltageData) * _VCoeff;   
	return Vol;
} 


voltage_t HLW8032::GetDividerVoltage()
{
	float DividerVoltage = checkDivisionOfZero(VoltageParam , VoltageData);
	return DividerVoltage;
}


current_t HLW8032::GetEffCurrent()
{
	current_t  Current = checkDivisionOfZero(CurrentParam, CurrentData) * _CCoeff;    
	return Current;
}


current_t HLW8032::GetShuntVoltage()
{
	voltage_t  ShuntVoltage  = checkDivisionOfZero(CurrentParam, CurrentData);
	return ShuntVoltage;
}



power_t HLW8032::GetActivePower()
{
	power_t Power = checkDivisionOfZero(PowerParam,PowerData) * _VCoeff * _CCoeff;  
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

	return checkDivisionOfZero(ActivePower, ApparentPower);  
}


uint16_t HLW8032::GetPF() { return PF; }

uint32_t HLW8032::GetPFAll() { return PFData * PF; }

energy_t HLW8032::GetKWh()
{
	// Calculates the pulse count for one kWh
	uint32_t PulseCount = 	checkDivisionOfZero(1,PowerParam)
						  * checkDivisionOfZero(1,(_VCoeff * _CCoeff))
						  * 1000000000 * 3600;
	
	energy_t KWh =checkDivisionOfZero((PFData * PF) , PulseCount);  
	return KWh;

}