/*----------------------------------------------------------------------------                              ----------*\
Things to do:
-Rename the "location" parameter to "address" on all access functions
-Add an access mask to the SetLineState() function, to support tri-state line changes, and
document it in BusInterfaceDoco.txt.
-Change all parameters to use Data arguments, rather than unsigned int?
-Implement the concept of a "Clock mapping". We should be able to define a "Clock" device,
which is a fixed oscillator and produces a clock pulse at a specified frequency. This must
be a device which can be constructed like any other device. A clock device should be able
to be mapped to a device through the system XML structure. There should be a set of
virtual functions in the Device base class to implement the clock interface. A mapping
between a device and a clock should be direct from device to device, like a reference,
rather than running through the bus. The interface should be something like this:
   virtual double GetCurrentClockRate(double accessTime) const;
This function will return the current clock rate at the time the function is called. We
can use this function as a drop-in replacement for the current internal clock rate data
members, and get the desired functionality now. Advanced functions like this should
perhaps be provided:
   // Returns the remaining time between accessTime and the next clock pulse
   virtual double GetCurrentClockPhase(double accessTime) const;
Need to make sure we don't create functions though which encourage compounding errors.

We'll work more on the clock interface a little later. Anyway, other devices need to be
able to act as clock sources too, not just dedicated clock devices, and multiple clock 
outputs need to be supported from the one device, IE, like the VDP in the Mega Drive,
which provides clock signals for most of the system, at different rates. Clock signals
should be mapped just like lines between devices are mapped, with an interface which
resolves text names down to numeric identifiers, which can easily be validated and decoded
when a clock rate is requested.

Note that we need in particular to support clock rates which change frequently at runtime,
like the SNES uses to change the clock frequency of devices on different bus access
operations. We need to think about how we can support this efficiently. In order for it to
be efficient, we need to be able to do it without locks on each clock access.

Perhaps we're going about this the wrong way. Maybe we should take a page out of the model
we used for line-based access, where all subscribed devices are notified about changes in
the clock rate, but its the job of each device to cache the current clock rate. Changes in
clock rate are timestamped, and each device can check its own execution progress to make
sure it hasn't executed past that point yet. Dependent device associations can be used to
prevent excessive rollbacks in the case of a clock rate which is expected to change
frequently.
\*--------------------------------------------------------------------------------------------------------------------*/
#ifndef __BUSINTERFACE_H__
#define __BUSINTERFACE_H__
#include <string>
#include <vector>
#include <list>
#include <map>
#include "HierarchicalStorageInterface/HierarchicalStorageInterface.pkg"
#include "ThinContainers/ThinContainers.pkg"
#include "DeviceInterface/DeviceInterface.pkg"
#include "DataRemapTable.h"

class BusInterface :public IBusInterface
{
public:
	// Structures
	struct BusInterfaceParams;
	struct DeviceMappingParams;
	struct LineMappingParams;
	struct ClockSourceMappingParams;

public:
	// Constructors
	BusInterface();
	~BusInterface();
	bool Construct(IHierarchicalStorageNode& node);
	bool Construct(const BusInterfaceParams& params);

	// Interface version functions
	virtual unsigned int GetIBusInterfaceVersion() const;

	// Unmapping functions
	void RemoveAllReferencesToDevice(IDevice* device);

	// Memory mapping functions
	bool MapDevice(IDevice* device, IHierarchicalStorageNode& node);
	bool MapDevice(IDevice* device, const DeviceMappingParams& params);

	// Port mapping functions
	bool MapPort(IDevice* device, IHierarchicalStorageNode& node);
	bool MapPort(IDevice* device, const DeviceMappingParams& params);

	// Line mapping functions
	bool MapLine(IDevice* sourceDevice, IDevice* targetDevice, IHierarchicalStorageNode& node);
	bool MapLine(IDevice* sourceDevice, IDevice* targetDevice, const LineMappingParams& params);
	bool MapLine(IDevice* sourceDevice, unsigned int targetLineGroupID, IHierarchicalStorageNode& node);
	bool MapLine(IDevice* sourceDevice, unsigned int targetLineGroupID, const LineMappingParams& params);
	bool MapLine(unsigned int sourceLineGroupID, IDevice* targetDevice, IHierarchicalStorageNode& node);
	bool MapLine(unsigned int sourceLineGroupID, IDevice* targetDevice, const LineMappingParams& params);
	bool IsDeviceLineMappedTo(IDevice* device, unsigned int lineNo) const;

	// CE line mapping functions
	bool DefineCELineMemory(IHierarchicalStorageNode& node);
	bool DefineCELinePort(IHierarchicalStorageNode& node);
	bool MapCELineInputMemory(IDevice* device, IHierarchicalStorageNode& node);
	bool MapCELineInputPort(IDevice* device, IHierarchicalStorageNode& node);
	bool MapCELineOutputMemory(IDevice* device, IHierarchicalStorageNode& node);
	bool MapCELineOutputPort(IDevice* device, IHierarchicalStorageNode& node);
	bool BindCELineMappings();

	// Clock source mapping functions
	bool MapClockSource(IClockSource* sourceClock, IDevice* targetDevice, IHierarchicalStorageNode& node);
	bool MapClockSource(IClockSource* sourceClock, IDevice* targetDevice, const ClockSourceMappingParams& params);

	// Memory interface functions
	virtual AccessResult ReadMemory(unsigned int location, Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext, void* calculateCELineStateContext = 0);
	virtual AccessResult WriteMemory(unsigned int location, const Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext, void* calculateCELineStateContext = 0);
	virtual void TransparentReadMemory(unsigned int location, Data& data, IDeviceContext* caller, unsigned int accessContext, void* calculateCELineStateContext = 0) const;
	virtual void TransparentWriteMemory(unsigned int location, const Data& data, IDeviceContext* caller, unsigned int accessContext, void* calculateCELineStateContext = 0) const;

	// Port interface functions
	virtual AccessResult ReadPort(unsigned int location, Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext, void* calculateCELineStateContext = 0);
	virtual AccessResult WritePort(unsigned int location, const Data& data, IDeviceContext* caller, double accessTime, unsigned int accessContext, void* calculateCELineStateContext = 0);
	virtual void TransparentReadPort(unsigned int location, Data& data, IDeviceContext* caller, unsigned int accessContext, void* calculateCELineStateContext = 0) const;
	virtual void TransparentWritePort(unsigned int location, const Data& data, IDeviceContext* caller, unsigned int accessContext, void* calculateCELineStateContext = 0) const;

	// Line interface functions
	virtual bool SetLineState(unsigned int sourceLine, const Data& lineData, IDeviceContext* sourceDevice, IDeviceContext* callingDevice, double accessTime, unsigned int accessContext);
	virtual bool RevokeSetLineState(unsigned int sourceLine, const Data& lineData, double reportedTime, IDeviceContext* sourceDevice, IDeviceContext* callingDevice, double accessTime, unsigned int accessContext);
	virtual bool AdvanceToLineState(unsigned int sourceLine, const Data& lineData, IDeviceContext* sourceDevice, IDeviceContext* callingDevice, double accessTime, unsigned int accessContext);

	// Clock source functions
	virtual void SetClockRate(double newClockRate, const IClockSource* sourceClock, IDeviceContext* callingDevice, double accessTime, unsigned int accessContext);
	virtual void TransparentSetClockRate(double newClockRate, const IClockSource* sourceClock);

private:
	// Structures
	struct MapEntry;
	struct LineEntry;
	struct LineMappingTemplate;
	struct LineGroupMappingInfo;
	struct CELineDefinition;
	struct CELineDeviceLineInput;
	struct CELineDeviceLineOutput;
	struct CELineDeviceEntry;
	struct ClockSourceEntry;

	// Typedefs
	typedef std::map<unsigned int, LineGroupMappingInfo> LineGroupMappings;
	typedef std::pair<unsigned int, LineGroupMappingInfo> LineGroupMappingsEntry;
	typedef std::map<unsigned int, CELineDefinition> CELineMap;
	typedef std::pair<unsigned int, CELineDefinition> CELineMapEntry;

private:
	// Generic map entry functions
	bool BuildMapEntry(MapEntry& mapEntry, IDevice* device, const DeviceMappingParams& params, unsigned int busMappingAddressBusMask, unsigned int busMappingAddressBusWidth, unsigned int busMappingDataBusWidth, bool memoryMapping) const;
	bool DoMapEntriesOverlap(const MapEntry& entry1, const MapEntry& entry2) const;
	void AddMapEntryToPhysicalMap(MapEntry* mapEntry, std::vector<ThinVector<MapEntry*,1>*>& physicalMap, unsigned int mappingAddressBusMask) const;
	void RemoveMapEntryFromPhysicalMap(MapEntry* mapEntry, std::vector<ThinVector<MapEntry*,1>*>& physicalMap, unsigned int mappingAddressBusMask);

	// Memory mapping functions
	bool MapDevice(MapEntry* mapEntry);
	void UnmapMemoryForDevice(IDevice* device);
	void UnmapDevice(MapEntry* mapEntry);

	// Port mapping functions
	bool MapPort(MapEntry* mapEntry);
	void UnmapPortForDevice(IDevice* device);
	void UnmapPort(MapEntry* mapEntry);

	// Line mapping functions
	bool ExtractLineMappingParams(IHierarchicalStorageNode& node, LineMappingParams& params) const;
	bool MapLine(LineEntry* lineEntry);
	void UnmapLineForDevice(IDevice* device);
	void AddLineEntryToPhysicalMap(LineEntry* lineEntry, std::vector<std::vector<ThinVector<LineEntry*,1>*>>& physicalLineMap, IDevice* indexDevice, unsigned int indexLineNo);

	// CE line mapping functions
	bool DefineCELine(IHierarchicalStorageNode& node, bool memoryMapping);
	bool MapCELineInput(IDevice* device, IHierarchicalStorageNode& node, bool memoryMapping);
	bool MapCELineOutput(IDevice* device, IHierarchicalStorageNode& node, bool memoryMapping);
	bool BindCELineMappings(bool memoryMapping);
	void UnmapCELinesForDevice(IDevice* device);

	// Clock source mapping functions
	bool ExtractClockSourceMappingParams(IHierarchicalStorageNode& node, ClockSourceMappingParams& params) const;
	bool MapClockSource(const ClockSourceEntry& clockSourceEntry);
	void UnmapClockSourceForDevice(IDevice* device);

	// CE line state functions
	unsigned int CalculateCELineStateMemory(unsigned int location, const Data& data, IDeviceContext* caller, void* calculateCELineStateContext, double accessTime) const;
	unsigned int CalculateCELineStateMemoryTransparent(unsigned int location, const Data& data, IDeviceContext* caller, void* calculateCELineStateContext) const;
	unsigned int CalculateCELineStatePort(unsigned int location, const Data& data, IDeviceContext* caller, void* calculateCELineStateContext, double accessTime) const;
	unsigned int CalculateCELineStatePortTransparent(unsigned int location, const Data& data, IDeviceContext* caller, void* calculateCELineStateContext) const;

	// Memory interface functions
	MapEntry* ResolveMemoryAddress(unsigned int ce, unsigned int location) const;

	// Port interface functions
	MapEntry* ResolvePortAddress(unsigned int ce, unsigned int location) const;

	// ThinVector helper functions
	template<class T>
	static ThinVector<T*,1>* AddItemToThinVector(ThinVector<T*,1>* existingArray, T* item);
	template<class T>
	static ThinVector<T*,1>* RemoveItemFromThinVector(ThinVector<T*,1>* existingArray, T* item);

private:
	// Memory map
	bool _memoryInterfaceDefined;
	bool _usePhysicalMemoryMap;
	std::vector<ThinVector<MapEntry*,1>*> _physicalMemoryMap;
	std::vector<MapEntry*> _memoryMap;
	unsigned int _addressBusWidth;
	unsigned int _dataBusWidth;
	unsigned int _addressBusMask;

	// Port map
	bool _portInterfaceDefined;
	bool _usePhysicalPortMap;
	std::vector<ThinVector<MapEntry*,1>*> _physicalPortMap;
	std::vector<MapEntry*> _portMap;
	unsigned int _portAddressBusWidth;
	unsigned int _portDataBusWidth;
	unsigned int _portAddressBusMask;

	// Other mapped lines
	std::vector<LineEntry*> _lineMap;
	std::vector<std::vector<ThinVector<LineEntry*,1>*>> _physicalLineMapOnSourceDevice;
	std::vector<std::vector<ThinVector<LineEntry*,1>*>> _physicalLineMapOnTargetDevice;

	// Line mapping templates
	LineGroupMappings _lineGroupMappingTemplates;

	// CE line mappings
	unsigned int _nextCELineID;
	CELineMap _ceLineDefinitionsMemory;
	CELineMap _ceLineDefinitionsPort;
	unsigned int _ceLineInitialStateMemory;
	unsigned int _ceLineInitialStatePort;
	unsigned int _ceLineDeviceMappingsMemoryOutputDeviceSize;
	std::vector<CELineDeviceEntry> _ceLineDeviceMappingsMemory;
	unsigned int _ceLineDeviceMappingsPortOutputDeviceSize;
	std::vector<CELineDeviceEntry> _ceLineDeviceMappingsPort;

	// Clock source mappings
	std::list<ClockSourceEntry> _clockSourceMap;
};

#include "BusInterface.inl"
#endif
