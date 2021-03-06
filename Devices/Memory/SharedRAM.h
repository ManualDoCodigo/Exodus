#ifndef __SHAREDRAM_H__
#define __SHAREDRAM_H__
#include "MemoryWrite.h"
#include <mutex>
#include <map>
#include <vector>

class SharedRAM :public MemoryWrite
{
public:
	// Constructors
	SharedRAM(const std::wstring& implementationName, const std::wstring& instanceName, unsigned int moduleID);
	virtual bool Construct(IHierarchicalStorageNode& node);

	// Memory size functions
	virtual unsigned int GetMemoryEntrySizeInBytes() const;

	// Initialization functions
	virtual void Initialize();

	// Execute functions
	virtual void ExecuteRollback();
	virtual void ExecuteCommit();

	// Memory interface functions
	virtual IBusInterface::AccessResult ReadInterface(unsigned int interfaceNumber, unsigned int location, Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext);
	virtual IBusInterface::AccessResult WriteInterface(unsigned int interfaceNumber, unsigned int location, const Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext);
	virtual void TransparentReadInterface(unsigned int interfaceNumber, unsigned int location, Data& data, IDeviceContext* caller, unsigned int accessContext);
	virtual void TransparentWriteInterface(unsigned int interfaceNumber, unsigned int location, const Data& data, IDeviceContext* caller, unsigned int accessContext);

	// Debug memory access functions
	virtual unsigned int ReadMemoryEntry(unsigned int location) const;
	virtual void WriteMemoryEntry(unsigned int location, unsigned int data);

	// Memory locking functions
	virtual bool IsMemoryLockingSupported() const;
	virtual void LockMemoryBlock(unsigned int location, unsigned int size, bool state);
	virtual bool IsAddressLocked(unsigned int location) const;

	// Savestate functions
	virtual void LoadState(IHierarchicalStorageNode& node);
	virtual void SaveState(IHierarchicalStorageNode& node) const;

private:
	// Rollback data
	struct MemoryWriteStatus
	{
		MemoryWriteStatus()
		{ }
		MemoryWriteStatus(bool awritten, unsigned char adata, IDeviceContext* aauthor, double atimeslice, unsigned int aaccessContext)
		:written(awritten), shared(false), data(adata), author(aauthor), timeslice(atimeslice), accessContext(aaccessContext)
		{ }

		bool written;
		bool shared;
		unsigned char data;
		IDeviceContext* author;
		double timeslice;
		unsigned int accessContext;
	};
	typedef std::map<unsigned int, MemoryWriteStatus> MemoryAccessBuffer;
	typedef std::pair<unsigned int, MemoryWriteStatus> MemoryAccessBufferEntry;

	std::mutex _accessLock;
	MemoryAccessBuffer _buffer;
	std::vector<unsigned char> _memory;
	std::vector<bool> _memoryLocked;
};

#endif
