#ifndef __ROM8VARIABLE_H__
#define __ROM8VARIABLE_H__
#include "ROMBase.h"

class ROM8Variable :public ROMBase<unsigned char>
{
public:
	// Constructors
	ROM8Variable(const std::wstring& implementationName, const std::wstring& instanceName, unsigned int moduleID);

	// Memory interface functions
	virtual IBusInterface::AccessResult ReadInterface(unsigned int interfaceNumber, unsigned int location, Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext);
	virtual IBusInterface::AccessResult WriteInterface(unsigned int interfaceNumber, unsigned int location, const Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext);
	virtual void TransparentReadInterface(unsigned int interfaceNumber, unsigned int location, Data& data, IDeviceContext* caller, unsigned int accessContext);
	virtual void TransparentWriteInterface(unsigned int interfaceNumber, unsigned int location, const Data& data, IDeviceContext* caller, unsigned int accessContext);

	// Debug memory access functions
	virtual unsigned int ReadMemoryEntry(unsigned int location) const;
	virtual void WriteMemoryEntry(unsigned int location, unsigned int data);
};

#endif
