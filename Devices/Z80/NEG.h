#include "Z80Instruction.h"
#ifndef __Z80_NEG_H__
#define __Z80_NEG_H__
namespace Z80 {

class NEG :public Z80Instruction
{
public:
	virtual NEG* Clone() const {return new NEG();}
	virtual NEG* ClonePlacement(void* buffer) const {return new(buffer) NEG();}
	virtual size_t GetOpcodeClassByteSize() const {return sizeof(*this);}

	virtual bool RegisterOpcode(OpcodeTable<Z80Instruction>& table) const
	{
		return table.AllocateRegionToOpcode(this, L"01000100", L"");
	}

	virtual std::wstring GetOpcodeName() const
	{
		return L"NEG";
	}

	virtual Disassembly Z80Disassemble(const Z80::LabelSubstitutionSettings& labelSettings) const
	{
		return Disassembly(GetOpcodeName(), L"");
	}

	virtual void Z80Decode(Z80* cpu, const Z80Word& location, const Z80Byte& data, bool transparent)
	{
		_target.SetIndexState(GetIndexState(), GetIndexOffset());

		// NEG		11101101 01000100
		_target.SetMode(EffectiveAddress::Mode::A);
		AddExecuteCycleCount(4);

		AddInstructionSize(GetIndexOffsetSize(_target.UsesIndexOffset()));
		AddInstructionSize(_target.ExtensionSize());
	}

	virtual ExecuteTime Z80Execute(Z80* cpu, const Z80Word& location) const
	{
		double additionalTime = 0;
		Z80Byte op1;
		Z80Byte result;

		// Perform the operation
		additionalTime += _target.Read(cpu, location, op1);
		result = 0 - op1.GetData();
		additionalTime += _target.Write(cpu, location, result);

		// Set the flag results
		cpu->SetFlagS(result.Negative());
		cpu->SetFlagZ(result.Zero());
		cpu->SetFlagY(result.GetBit(5));
		cpu->SetFlagH(!op1.Zero());
		cpu->SetFlagX(result.GetBit(3));
		cpu->SetFlagPV(op1 == 0x80);
		cpu->SetFlagN(true);
		cpu->SetFlagC(!op1.Zero());

		// Adjust the PC and return the execution time
		cpu->SetPC(location + GetInstructionSize());
		return GetExecuteCycleCount(additionalTime);
	}

private:
	EffectiveAddress _target;
};

} // Close namespace Z80
#endif
