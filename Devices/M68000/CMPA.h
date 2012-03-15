#include "M68000Instruction.h"
#ifndef __M68000_CMPA_H__
#define __M68000_CMPA_H__
namespace M68000 {

class CMPA :public M68000Instruction
{
public:
	virtual CMPA* Clone() {return new CMPA();}
	virtual CMPA* ClonePlacement(void* buffer) {return new(buffer) CMPA();}

	virtual bool RegisterOpcode(OpcodeTable<M68000Instruction>* table)
	{
		return table->AllocateRegionToOpcode(this, L"1011***CC1DDDDDD", L"CC=01,11 DDDDDD=000000-110111,111000,111001,111010,111011,111100");
	}

	virtual Disassembly M68000Disassemble()
	{
		return Disassembly(L"CMPA." + DisassembleSize(size), source.Disassemble() + L", " + target.Disassemble());
	}

	virtual void M68000Decode(M68000* cpu, const M68000Long& location, const M68000Word& data, bool transparent)
	{
//	-----------------------------------------------------------------
//	|15 |14 |13 |12 |11 |10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//	|---|---|---|---|-----------|-------|---|-----------|-----------|
//	| 1 | 0 | 1 | 1 |  REGISTER |  SIZE | 1 |    MODE   |  REGISTER |
//	----------------------------------------=========================
//                              |--<opmode>-|----------<ea>---------|
		switch(data.GetDataSegment(7, 2))
		{
		case 1:	//01
			size = BITCOUNT_WORD;
			break;
		case 3:	//11
			size = BITCOUNT_LONG;
			break;
		}

		//CMPA	<ea>,An
		target.BuildAddressDirect(size, location + GetInstructionSize(), data.GetDataSegment(9, 3));
		source.Decode(data.GetDataSegment(0, 3), data.GetDataSegment(3, 3), size, location + GetInstructionSize(), cpu, transparent, GetInstructionRegister());
		AddInstructionSize(source.ExtensionSize());

		AddExecuteCycleCount(ExecuteTime(6, 1, 0));
		AddExecuteCycleCount(source.DecodeTime());
	}

	virtual ExecuteTime M68000Execute(M68000* cpu, const M68000Long& location) const
	{
		double additionalTime = 0;
		M68000Long op1;
		M68000Long op2;
		M68000Long result;
		Data temp(size);

		//Perform the operation
		additionalTime += source.Read(cpu, temp, GetInstructionRegister());
		additionalTime += target.Read(cpu, op2, GetInstructionRegister());
		op1 = M68000Long(temp.SignExtend(BITCOUNT_LONG));
		result = op2 - op1;

		//Set the flag results
		bool overflow = (op1.MSB() == result.MSB()) && (op2.MSB() != op1.MSB());
		bool borrow = (op1.MSB() && result.MSB()) || (!op2.MSB() && (op1.MSB() || result.MSB()));
		cpu->SetN(result.Negative());
		cpu->SetZ(result.Zero());
		cpu->SetV(overflow);
		cpu->SetC(borrow);

		//Adjust the PC and return the execution time
		cpu->SetPC(location + GetInstructionSize());
		return GetExecuteCycleCount(additionalTime);
	}

private:
	EffectiveAddress source;
	EffectiveAddress target;
	Bitcount size;
};

} //Close namespace M68000
#endif