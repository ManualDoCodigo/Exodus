/*--------------------------------------------------------------------------------------*\
Things to do:
-Make everything in this core which needs a way of determining the current progression
through the horizontal and vertical scanlines use a different counting mechanism, rather
than using hcounter and vcounter values like they do currently. I would suggest relating
the hcounter back to the serial clock, and make the vertical counter a simple raster
count. This will eliminate the necessity to constantly convert from the "external"
hcounter and vcounter, where the counter values jump at various points, and will also
allow extra accuracy in the case of the horizontal counter, where some events occur at a
sub-pixel level of timing. It appears that SC is the true master clock signal which drives
the video functions of the VDP.
-Within the VDP core, switch to using SC cycles rather than MCLK cycles, since this is the
way the real VDP works, and it'll simplify things. Place MCLK cycles on the outer edge,
and provide functions to convert MCLK cycle counts into SC cycle counts where required.
Note that within a line, SC and the H32/H40 mode is sufficient to convert SC cycles back
to MCLK cycles, but within a frame, SC is not sufficient, because H32/H40 mode changes can
occur between successive lines within a frame.
-We know now that while the FIFO allows an entire word to be read or written to CRAM or
VSRAM on each access slot, VRAM only allows one byte to be transferred per access slot. We
need to update our FIFO implementation to support this. We also need to confirm through
hardware tests if a write to VRAM ends up in two separate byte-wide FIFO entries, or just
in a single FIFO entry which takes two access slots to complete.
-We need to determine when screen mode settings are latched for a line. Here are the
properties we know can be modified on a line-by-line basis: H32/H40, M4/M5, TMS9918A text
mode (R1,B7), Shadow/Highlight). These settings are most likely latched together at one
point in the raster, and this is when we need to base our start of line from. Note
however, that we don't know for sure yet that some or all of these settings don't take
effect immediately, even during a line. There's also the test register to consider.

Tests to run:
-We suspect that CRAM has a buffer register just like VSRAM, and that the CRAM flicker bug
occurs because a CRAM read cycle is stolen from the render process in order to perform the
CRAM write, and that the buffer is loaded with the data to be written, which then gets
read out by the render process as if it was the requested CRAM data. If this is the case
however, it seems logical there should be another CRAM flicker bug, this time caused by
reads from CRAM. If it is possible to perform a CRAM read outside of hblank, a CRAM read
would need to steal a cycle from the render process as well, most likely resulting in the
read value from CRAM replacing the pixel that is being rendered at that time. We should
perform a test on this behaviour, by constantly reading from CRAM with a colourful CRAM
palette loaded, but the first entry set to 0, and see if we get speckles on our display.

//#######################################Old todo#########################################
-Add savestate support
-We have seen a case where invalid sprite data caused flickering on the screen between
successive frames, even though all the VDP state was static between frames. The cause of
this flickering is completely unknown, and we have no theory as to why one frame would
render differently from the next. I can't find any test rom saved, so we'll have to do
tests on the hardware to try and reproduce this behaviour first.

-Check what happens with a data port write to trigger a DMA fill, when that write is held
in the FIFO. The FIFO will always be empty when a data port write to trigger a DMA fill
occurs, but if it's during active scan, that write might not be processed straight away.
In the case that this happens, I would expect that the VDP would not report that DMA is in
progress until the write leaves the FIFO, which would mean an app just monitoring the DMA
flag might think the DMA operation is complete before the write has even been processed.
This needs testing on the hardware. We should confirm that this can occur.
-An additional test for the interaction of DMA and the FIFO relates to the end of a DMA
operation. It is possible that all DMA writes are added to the back of the FIFO. This may
mean that when a DMA operation ends, and the M68000 bus is released if it was locked,
there are still some writes remaining in the FIFO which were added by DMA. Check the FIFO
empty flag in the status register at the completion of a DMA operation during active scan
to determine whether this is the case.
-Check what happens when the data port or control ports are written to during DMA fill
and copy operations. According to genvdp.txt, writing to the control or data ports during
a DMA fill corrupts VDP registers and VRAM somehow. Investigate this behaviour. In
addition, we need to check the behaviour of attempted data port reads during a DMA
operation.
-There are warnings in the official docs that DMA must only be enabled when a DMA
operation is about to be performed, and must be disabled afterwards, otherwise they
"cannot guarantee the operation". Perform some tests to see if anything breaks when DMA
is left enabled for normal port access.
-We know that fills can be done to CRAM and VSRAM, but the way in which this occurs isn't
clear. Test all possible quirks, such as writing to odd addresses, and verify the
behaviour.
-It is currently unknown whether DMA copy operations can be performed on CRAM and VSRAM.
Run some tests to determine whether this is possible.
-Note that further testing has also shown that the VRAM is NOT initialized to all 0. It
appears the VRAM is initialized with a logical, repeating pattern, but further tests must
be carried out to determine what this pattern is. I would suggest reading the entire
contents of VRAM, CRAM, and VSRAM back into RAM for analysis.
-The CRAM is initialized to 0x0EEE in all slots, except for the second entry in the
second palette line, which is initialized to 0x0CEE. This means byte 0x22 in the CRAM is
0x0C instead of 0x0E.
-The VSRAM is initialized to 0x07FF, except for the entry at 0x00, which is 0x07DF, and
the entry at 0x22, which is 0x07FB.

!!!!NOTE!!!!
-As per cfgm2_notes.pdf, "sprites are processed on the previous line than the one they
are shown on, and are not processed when the display is blanked". Also other important
comments about when major changes to screen mode take effect.
-According to a note by Charles Macdonald in newreg.htm, the first column of a layer
cannot be controlled in 2-cell vscroll mode. Instead, the column remains unaffected. This
is demonstrated in the Ecco II globe holder level, where sprites are actually used to
mask the first column. Scroll B is the layer in question in this case, but Layer A is
most likely affected as well. Note that currently we do NOT emulate this behaviour.
-Further on the 2-cell vscroll problem, I think this occurs simply because the designers
ran out of access slots to read the vscroll data. If this is true, the vscroll value
probably uses the last value read for the first column, rather than 0. This may mean that
the very last value in in the vscroll table will affect the first column. I also seem to
remember reading something like this on SpritesMind. Do more research into this behaviour.

Debug outputs changes:
-Modify the palette window to include palette row and column labels, as well as a popup
style window in the same style as the VRAM viewer, which shows the actual RGB palette
values that are being used to create it.
-Note that there's a bug in the layer removal process currently. When layer A is
disabled and shadow highlight mode is in use, the window layer ends up being used for
priority calculations for shadow highlight mode. See the Ecco II globe holder, code
SBFPWWJE for an example.
-Provide options to force all tiles in each layer to either high or low priority. This
can be used to bring hidden sprites into view for example. Also add an option to "mark"
sprites in some way, so that even transparent sprites are visible, and the boundaries of
all sprites on the screen are indicated clearly.
-Provide options to control which area of the screen to make visible. Have an option to
show all overscan regions to make each generated pixel visible.

Known inaccuracies:
-On the real hardware, changes to the H32/H40 cell mode take effect immediately, even
during a line, which can cause the current line to be corrupted. We latch the register
settings during hblank, for convenience, so that each line rendered maintains a rate of
3420 MCLK cycles per line.

References:
-Genesis Software Manual, Sega Enterprises, 1992, scans by antime
-Sega Genesis VDP Documentation (genvdp.txt), Charles MacDonald, v1.5f 10/8/2000
-Sega Master System VDP Documentation (msvdp.txt), Charles MacDonald, 12/11/2002
-315-5313 Information (vdppin.txt), Charles MacDonald, 2008
-TMS9918A/TMS9928A/TMS9929A Video Display Processors, Texas Instruments, 1982
\*--------------------------------------------------------------------------------------*/
#ifndef __S315_5313_H__
#define __S315_5313_H__
#include "WindowFunctions/WindowFunctions.pkg"
#include "SystemInterface/SystemInterface.pkg"
#include "TimedBuffers/TimedBuffers.pkg"
#include <vector>
#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "Device/Device.pkg"

class S315_5313 :public Device
{
public:
	//Constructors
	S315_5313(const std::wstring& ainstanceName, unsigned int amoduleID);
	virtual ~S315_5313();

	//Line functions
	virtual unsigned int GetLineID(const wchar_t* lineName) const;
	virtual const wchar_t* GetLineName(unsigned int lineID) const;
	virtual unsigned int GetLineWidth(unsigned int lineID) const;
	virtual void SetLineState(unsigned int targetLine, const Data& lineData, IDeviceContext* caller, double accessTime);

	//Initialization functions
	virtual bool BuildDevice();
	virtual bool ValidateDevice();
	virtual void Initialize();
	//##TODO## Determine whether we actually need a reset function for this device. Does
	//the VDP have a reset line? If it does, we should create it, and have the handler for
	//that line call the reset function at the appropriate time. If it does not, we should
	//remove the reset function, and just implement the Initialize() function. Also note,
	//if a reset line is present, we have to verify what gets cleared by a reset.
	void Reset();
	virtual void BeginExecution();
	virtual void SuspendExecution();

	//Reference functions
	virtual bool AddReference(const wchar_t* referenceName, IBusInterface* target);
	virtual bool AddReference(const wchar_t* referenceName, IDevice* target);
	virtual bool RemoveReference(IBusInterface* target);
	virtual bool RemoveReference(IDevice* target);

	//Execute functions
	virtual UpdateMethod GetUpdateMethod() const;
	virtual bool SendNotifyUpcomingTimeslice() const;
	virtual void NotifyUpcomingTimeslice(double nanoseconds);
	virtual void NotifyBeforeExecuteCalled();
	virtual bool SendNotifyBeforeExecuteCalled() const;
	virtual void NotifyAfterExecuteCalled();
	virtual bool SendNotifyAfterExecuteCalled() const;
	virtual void ExecuteTimeslice(double nanoseconds);
	virtual double GetNextTimingPointInDeviceTime() const;
	virtual void ExecuteRollback();
	virtual void ExecuteCommit();

	//CE line state functions
	virtual unsigned int GetCELineID(const wchar_t* lineName, bool inputLine) const;
	virtual void SetCELineInput(unsigned int lineID, bool lineMapped, unsigned int lineStartBitNumber);
	virtual void SetCELineOutput(unsigned int lineID, bool lineMapped, unsigned int lineStartBitNumber);
	virtual unsigned int CalculateCELineStateMemory(unsigned int location, const Data& data, unsigned int currentCELineState, const IBusInterface* sourceBusInterface, IDeviceContext* caller, double accessTime) const;
	virtual unsigned int CalculateCELineStateMemoryTransparent(unsigned int location, const Data& data, unsigned int currentCELineState, const IBusInterface* sourceBusInterface, IDeviceContext* caller) const;
	unsigned int BuildCELine(unsigned int targetAddress, bool vdpIsSource, bool currentLowerDataStrobe, bool currentUpperDataStrobe, bool operationIsWrite, bool rmwCycleInProgress, bool rmwCycleFirstOperation) const;

	//Memory interface functions
	virtual IBusInterface::AccessResult ReadInterface(unsigned int interfaceNumber, unsigned int location, Data& data, IDeviceContext* caller, double accessTime);
	virtual IBusInterface::AccessResult WriteInterface(unsigned int interfaceNumber, unsigned int location, const Data& data, IDeviceContext* caller, double accessTime);

protected:
	//Window functions
	virtual void AddDebugMenuItems(IMenuSegment& menuSegment, IViewModelLauncher& viewModelLauncher);
	virtual void RestoreViewModelState(const std::wstring& menuHandlerName, int viewModelID, IHeirarchicalStorageNode& node, int xpos, int ypos, int width, int height, IViewModelLauncher& viewModelLauncher);
	virtual void OpenViewModel(const std::wstring& menuHandlerName, int viewModelID, IViewModelLauncher& viewModelLauncher);

private:
	//Enumerations
	enum CELineID
	{
		CELINE_LDS = 1,
		CELINE_UDS,
		CELINE_RW,
		CELINE_AS,
		CELINE_RMWCYCLEINPROGRESS,
		CELINE_RMWCYCLEFIRSTOPERATION,
		CELINE_LWR,
		CELINE_UWR,
		CELINE_CAS0,
		CELINE_RAS0,
		CELINE_OE0
	};
	enum LineID
	{
		LINE_IPL = 1,
		LINE_INT,
		LINE_INTAK,
		LINE_BR,
		LINE_BG,
		LINE_PAL
	};
	enum Event;
	enum Layer;

	//Structures
	struct HScanSettings;
	struct VScanSettings;
	struct LineRenderSettings;
	struct EventProperties
	{
		Event event;
		unsigned int mclkCycleCounter;
		unsigned int hcounter;
		unsigned int vcounter;
	};
	//##TODO## Implement this structure
	struct FIFOBufferEntry;

	//Typedefs
	typedef RandomTimeAccessBuffer<Data, unsigned int> RegBuffer;
	typedef RegBuffer::AccessTarget AccessTarget;
	typedef ITimedBufferInt::AccessTarget RAMAccessTarget;

	//Constants
	static const unsigned int registerCount = 24;
	static const unsigned int vramSize = 0x10000;
	static const unsigned int cramSize = 0x80;
	static const unsigned int vsramSize = 0x50;
	static const unsigned int fifoBufferSize = 4;
	static const unsigned int statusRegisterMask = 0x03FF;

	//Interrupt settings
	static const unsigned int exintIPLLineState = 2;
	static const unsigned int hintIPLLineState = 4;
	static const unsigned int vintIPLLineState = 6;

	//View and menu classes
	class DebugMenuHandler;
	class VRAMViewModel;
	class PaletteViewModel;
	class ImageViewModel;
	class RegistersViewModel;
	class VRAMView;
	class PaletteView;
	class ImageView;
	class RegistersView;
	friend class VRAMViewModel;
	friend class PaletteViewModel;
	friend class ImageViewModel;
	friend class RegistersViewModel;
	friend class VRAMView;
	friend class PaletteView;
	friend class ImageView;
	friend class RegistersView;

private:
	//Execute functions
	void RenderThread();
	void RenderFrame();
	void RenderPrefetchLineForSprites(const AccessTarget& accessTarget);
	void RenderBorderLine(const AccessTarget& accessTarget, unsigned int renderBufferLineNo, unsigned int sectionLineNo);
	void RenderActiveLine(const AccessTarget& accessTarget, unsigned int renderBufferLineNo, unsigned int sectionLineNo);
	void CalculateLayerPriorityIndex(unsigned int& layerIndex, bool& shadow, bool& highlight, bool shadowHighlightEnabled, bool spriteIsShadowOperator, bool spriteIsHighlightOperator, bool foundSpritePixel, bool foundLayerAPixel, bool foundLayerBPixel, bool prioritySprite, bool priorityLayerA, bool priorityLayerB) const;
	void WriteColorValueToImageBuffer(unsigned int paletteLine, unsigned int paletteEntry, bool shadow, bool highlight, unsigned int xpos, unsigned int ypos);
	void ReadMappingDataPair(Data& mappingDataEntry1, Data& mappingDataEntry2, unsigned int nameTableBaseAddress, unsigned int mappingColumnNumber, unsigned int mappingRowNumber, unsigned int screenSizeModeH, unsigned int screenSizeModeV) const;
	void ReadPatternDataRow(const LineRenderSettings& renderSettings, Data& patternData, const Data& mappingData, unsigned int patternRowNumber) const;
	void RenderColumnBlockPair(unsigned int columnNumber, unsigned int scrollValueDisplacement, const Data& mappingDataCell1, const Data& mappingDataCell2, const Data& patternDataCell1, const Data& patternDataCell2, std::vector<unsigned int>& outputPaletteLines, std::vector<unsigned int>& outputPaletteEntries, unsigned int& currentRenderPixel) const;
	void DMAWorkerThread();

	//Event functions
	void ExecuteEvent(EventProperties event, double accessTime, unsigned int ahcounter, unsigned int avcounter, bool ascreenModeH40, bool ascreenModeV30, bool apalMode, bool ainterlaceEnabled);
	void GetNextEvent(unsigned int currentMclkCycleCounter, bool timingPointsOnly, EventProperties& nextEvent) const;
	bool EventOccursWithinCounterRange(const HScanSettings& hscanSettings, unsigned int hcounterStart, unsigned int vcounterStart, unsigned int hcounterEnd, unsigned int vcounterEnd, unsigned int hcounterEventPos, unsigned int vcounterEventPos) const;

	//Port functions
	Data GetHVCounter() const;
	void ProcessCommandDataWriteFirstHalf(const Data& data);
	void ProcessCommandDataWriteSecondHalf(const Data& data);
	void RegisterSpecialUpdateFunction(unsigned int mclkCycle, double accessTime, double accessDelay, IDeviceContext* caller, unsigned int registerNo, const Data& data);

	bool ValidReadTargetInCommandCode() const;
//	bool ReadVideoMemory(unsigned int mclkCycle, Data& data);
//	void WriteVideoMemory(unsigned int mclkCycle, const Data& data);

	//HV counter internal/linear conversion
	static unsigned int HCounterValueFromVDPInternalToLinear(const HScanSettings& hscanSettings, unsigned int hcounterCurrent);
	static unsigned int VCounterValueFromVDPInternalToLinear(const VScanSettings& vscanSettings, unsigned int vcounterCurrent, bool oddFlagSet);
	static unsigned int HCounterValueFromLinearToVDPInternal(const HScanSettings& hscanSettings, unsigned int hcounterCurrent);
	static unsigned int VCounterValueFromLinearToVDPInternal(const VScanSettings& vscanSettings, unsigned int vcounterCurrent, bool oddFlagSet);

	//Video scan settings functions
	static const HScanSettings& GetHScanSettings(bool screenModeH40Active);
	static const VScanSettings& GetVScanSettings(bool screenModeV30Active, bool palModeActive, bool interlaceActive);

	//HV counter comparison functions
	static unsigned int GetPixelClockStepsBetweenHVCounterValues(const HScanSettings& hscanSettings, unsigned int hcounterCurrent, unsigned int hcounterTarget, const VScanSettings& vscanSettings, bool interlaceIsEnabled, bool oddFlagSet, unsigned int vcounterCurrent, unsigned int vcounterTarget);
	static unsigned int GetPixelClockStepsBetweenHCounterValues(const HScanSettings& hscanSettings, unsigned int hcounterCurrent, unsigned int hcounterTarget);
	static unsigned int GetPixelClockStepsBetweenVCounterValues(const HScanSettings& hscanSettings, unsigned int hcounterCurrent, const VScanSettings& vscanSettings, bool interlaceIsEnabled, bool oddFlagSet, unsigned int vcounterCurrent, unsigned int vcounterTarget);

	//HV counter advancement functions
	static unsigned int AddStepsToHCounter(const HScanSettings& hscanSettings, unsigned int hcounterCurrent, unsigned int hcounterStepsToAdd);
	static unsigned int AddStepsToVCounter(const HScanSettings& hscanSettings, unsigned int hcounterCurrent, const VScanSettings& vscanSettings, bool interlaceIsEnabled, bool oddFlagSet, unsigned int vcounterCurrent, unsigned int vcounterStepsToAdd);
	static void AdvanceHVCounters(const HScanSettings& hscanSettings, unsigned int& hcounterCurrent, const VScanSettings& vscanSettings, bool interlaceIsEnabled, bool& oddFlagSet, unsigned int& vcounterCurrent, unsigned int pixelClockSteps);

	//Pixel clock functions
	static unsigned int GetPixelClockTicksUntilNextAccessSlot(unsigned int hcounterCurrent, bool screenModeH40Current, unsigned int vcounterCurrent, bool screenModeV30Current, bool palModeCurrent, bool interlaceEnabledCurrent);
	static unsigned int GetPixelClockTicksForMclkTicks(unsigned int mclkTicks, unsigned int hcounterCurrent, bool screenModeH40Active, unsigned int& mclkTicksUnused);
	static unsigned int GetMclkTicksForPixelClockTicks(unsigned int pixelClockTicks, unsigned int hcounterCurrent, bool screenModeH40Active);

	//Access time functions
	unsigned int ConvertAccessTimeToMclkCount(double accessTime) const;
	double ConvertMclkCountToAccessTime(unsigned int mclkCount) const;

	//Processor state advancement functions
	void UpdateInternalState(unsigned int mclkCyclesTarget, bool checkFifoStateBeforeUpdate, bool stopWhenFifoEmpty, bool stopWhenFifoFull, bool stopWhenFifoNotFull, bool stopWhenReadDataAvailable, bool stopWhenNoDMAOperationInProgress, bool allowAdvancePastCycleTarget);
	bool AdvanceProcessorState(unsigned int mclkCyclesTarget, bool stopAtNextAccessSlot, bool allowAdvancePastTargetForAccessSlot);
	bool AdvanceProcessorStateNew(unsigned int mclkCyclesTarget, bool stopAtNextAccessSlot, bool allowAdvancePastTargetForAccessSlot);
	void PerformReadCacheOperation();
	void PerformFIFOWriteOperation();
	void PerformDMACopyOperation();
	void PerformDMAFillOperation();
	void CacheDMATransferReadData(unsigned int mclkTime);
	void PerformDMATransferOperation();
	void AdvanceDMAState();
	bool TargetProcessorStateReached(bool stopWhenFifoEmpty, bool stopWhenFifoFull, bool stopWhenFifoNotFull, bool stopWhenReadDataAvailable, bool stopWhenNoDMAOperationInProgress);
	void AdvanceStatusRegisterAndHVCounter(unsigned int pixelClockSteps);
	void AdvanceStatusRegisterAndHVCounterWithCurrentSettings(unsigned int pixelClockSteps);
	double GetProcessorStateTime() const;
	unsigned int GetProcessorStateMclkCurrent() const;

	//FIFO functions
	bool IsWriteFIFOEmpty() const;
	bool IsWriteFIFOFull() const;

	//##TODO## Mode 4 control functions

	//Mode 5 control functions
//	void M5ReadVRAM(const Data& address, Data& data, const RAMAccessTarget& accessTarget);
	void M5ReadVRAM8Bit(const Data& address, Data& data, const RAMAccessTarget& accessTarget);
	void M5ReadCRAM(const Data& address, Data& data, const RAMAccessTarget& accessTarget);
	void M5ReadVSRAM(const Data& address, Data& data, const RAMAccessTarget& accessTarget);
//	void M5WriteVRAM(const Data& address, const Data& data, const RAMAccessTarget& accessTarget);
	void M5WriteVRAM8Bit(const Data& address, const Data& data, const RAMAccessTarget& accessTarget);
	void M5WriteCRAM(const Data& address, const Data& data, const RAMAccessTarget& accessTarget);
	void M5WriteVSRAM(const Data& address, const Data& data, const RAMAccessTarget& accessTarget);

	//Status register functions
	inline bool GetStatusFlagFIFOEmpty() const;
	inline void SetStatusFlagFIFOEmpty(bool state);
	inline bool GetStatusFlagFIFOFull() const;
	inline void SetStatusFlagFIFOFull(bool state);
	inline bool GetStatusFlagF() const;
	inline void SetStatusFlagF(bool state);
	inline bool GetStatusFlagSpriteOverflow() const;
	inline void SetStatusFlagSpriteOverflow(bool state);
	inline bool GetStatusFlagSpriteCollision() const;
	inline void SetStatusFlagSpriteCollision(bool state);
	inline bool GetStatusFlagOddInterlaceFrame() const;
	inline void SetStatusFlagOddInterlaceFrame(bool state);
	inline bool GetStatusFlagVBlank() const;
	inline void SetStatusFlagVBlank(bool state);
	inline bool GetStatusFlagHBlank() const;
	inline void SetStatusFlagHBlank(bool state);
	inline bool GetStatusFlagDMA() const;
	inline void SetStatusFlagDMA(bool state);
	inline bool GetStatusFlagPAL() const;
	inline void SetStatusFlagPAL(bool state);

	//Raw register functions
	inline Data GetRegisterData(unsigned int location, const AccessTarget& accessTarget) const;
	inline void SetRegisterData(unsigned int location, const AccessTarget& accessTarget, const Data& data);

	//Mode 4 register functions
	inline bool M4GetVScrollingDisabled(const AccessTarget& accessTarget) const;	//0x00(0)
	inline void M4SetVScrollingDisabled(const AccessTarget& accessTarget, bool data);
	inline bool M4GetHScrollingDisabled(const AccessTarget& accessTarget) const;
	inline void M4SetHScrollingDisabled(const AccessTarget& accessTarget, bool data);
	inline bool M4GetColumnZeroMasked(const AccessTarget& accessTarget) const;
	inline void M4SetColumnZeroMasked(const AccessTarget& accessTarget, bool data);
	inline bool M4GetLineInterruptEnabled(const AccessTarget& accessTarget) const;
	inline void M4SetLineInterruptEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M4GetSpriteShift(const AccessTarget& accessTarget) const;
	inline void M4SetSpriteShift(const AccessTarget& accessTarget, bool data);
	inline bool M4GetM4Bit(const AccessTarget& accessTarget) const;
	inline void M4SetM4Bit(const AccessTarget& accessTarget, bool data);
	inline bool M4GetM2Bit(const AccessTarget& accessTarget) const;
	inline void M4SetM2Bit(const AccessTarget& accessTarget, bool data);
	inline bool M4GetReg0Bit0(const AccessTarget& accessTarget) const;
	inline void M4SetReg0Bit0(const AccessTarget& accessTarget, bool data);

	inline bool M4GetDisplayEnabled(const AccessTarget& accessTarget) const;	//0x01(1)
	inline void M4SetDisplayEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M4GetFrameInterruptEnabled(const AccessTarget& accessTarget) const;
	inline void M4SetFrameInterruptEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M4GetM1Bit(const AccessTarget& accessTarget) const;
	inline void M4SetM1Bit(const AccessTarget& accessTarget, bool data);
	inline bool M4GetM3Bit(const AccessTarget& accessTarget) const;
	inline void M4SetM3Bit(const AccessTarget& accessTarget, bool data);
	inline bool M4GetSpritesDoubleHeight(const AccessTarget& accessTarget) const;
	inline void M4SetSpritesDoubleHeight(const AccessTarget& accessTarget, bool data);
	inline bool M4GetSpriteZooming(const AccessTarget& accessTarget) const;
	inline void M4SetSpriteZooming(const AccessTarget& accessTarget, bool data);

	inline unsigned int M4GetNameTableBaseScroll(const AccessTarget& accessTarget) const;	//0x02(2)
	inline void M4SetNameTableBaseScroll(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M4GetAttributeTableBaseSprite(const AccessTarget& accessTarget) const;	//0x05(5)
	inline void M4SetAttributeTableBaseSprite(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M4GetPatternBaseSprite(const AccessTarget& accessTarget) const;	//0x06(6)
	inline void M4SetPatternBaseSprite(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M4GetBackdropColorIndex(const AccessTarget& accessTarget) const;	//0x07(7)
	inline void M4SetBackdropColorIndex(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M4GetBackgroundScrollX(const AccessTarget& accessTarget) const;	//0x08(8)
	inline void M4SetBackgroundScrollX(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M4GetBackgroundScrollY(const AccessTarget& accessTarget) const;	//0x09(9)
	inline void M4SetBackgroundScrollY(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M4GetLineCounter(const AccessTarget& accessTarget) const;	//0x0A(10)
	inline void M4SetLineCounter(const AccessTarget& accessTarget, unsigned int data);

	//Mode 5 register functions
	inline bool M5GetHInterruptEnabled(const AccessTarget& accessTarget) const;	//0x00(0)
	inline void M5SetHInterruptEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetPSEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetPSEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetHVCounterLatchEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetHVCounterLatchEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetDisplayEnabled(const AccessTarget& accessTarget) const;	//0x01(1)
	inline void M5SetDisplayEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetVInterruptEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetVInterruptEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetDMAEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetDMAEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetV30CellModeEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetV30CellModeEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetMode5Enabled(const AccessTarget& accessTarget) const;
	inline void M5SetMode5Enabled(const AccessTarget& accessTarget, bool data);

	inline unsigned int M5GetNameTableBaseScrollA(const AccessTarget& accessTarget) const;	//0x02(2)
	inline void M5SetNameTableBaseScrollA(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetNameTableBaseWindow(const AccessTarget& accessTarget) const;	//0x03(3)
	inline void M5SetNameTableBaseWindow(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetNameTableBaseScrollB(const AccessTarget& accessTarget) const;	//0x04(4)
	inline void M5SetNameTableBaseScrollB(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetNameTableBaseSprite(const AccessTarget& accessTarget) const;	//0x05(5)
	inline void M5SetNameTableBaseSprite(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M5GetBackgroundColorPalette(const AccessTarget& accessTarget) const;	//0x07(7)
	inline void M5SetBackgroundColorPalette(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetBackgroundColorIndex(const AccessTarget& accessTarget) const;
	inline void M5SetBackgroundColorIndex(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M5GetHInterruptData(const AccessTarget& accessTarget) const;	//0x0A(10)
	inline void M5SetHInterruptData(const AccessTarget& accessTarget, unsigned int data);
	inline bool M5GetEXInterruptEnabled(const AccessTarget& accessTarget) const;	//0x0B(11)
	inline void M5SetEXInterruptEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetVSCR(const AccessTarget& accessTarget) const;
	inline void M5SetVSCR(const AccessTarget& accessTarget, bool data);
	inline bool M5GetHSCR(const AccessTarget& accessTarget) const;
	inline void M5SetHSCR(const AccessTarget& accessTarget, bool data);
	inline bool M5GetLSCR(const AccessTarget& accessTarget) const;
	inline void M5SetLSCR(const AccessTarget& accessTarget, bool data);

	inline bool M5GetRS0(const AccessTarget& accessTarget) const;	//0x0C(12)
	inline void M5SetRS0(const AccessTarget& accessTarget, bool data);
	inline bool M5GetRS1(const AccessTarget& accessTarget) const;
	inline void M5SetRS1(const AccessTarget& accessTarget, bool data);
	inline bool M5GetShadowHighlightEnabled(const AccessTarget& accessTarget) const;
	inline void M5SetShadowHighlightEnabled(const AccessTarget& accessTarget, bool data);
	inline bool M5GetLSM1(const AccessTarget& accessTarget) const;
	inline void M5SetLSM1(const AccessTarget& accessTarget, bool data);
	inline bool M5GetLSM0(const AccessTarget& accessTarget) const;
	inline void M5SetLSM0(const AccessTarget& accessTarget, bool data);

	inline unsigned int M5GetHScrollDataBase(const AccessTarget& accessTarget) const;	//0x0D(13)
	inline void M5SetHScrollDataBase(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M5GetAutoIncrementData(const AccessTarget& accessTarget) const;	//0x0F(15)
	inline void M5SetAutoIncrementData(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M5GetVSZ(const AccessTarget& accessTarget) const;	//0x10(16)
	inline void M5SetVSZ(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetHSZ(const AccessTarget& accessTarget) const;
	inline void M5SetHSZ(const AccessTarget& accessTarget, unsigned int data);
	inline bool M5GetVSZ1(const AccessTarget& accessTarget) const;
	inline void M5SetVSZ1(const AccessTarget& accessTarget, bool data);
	inline bool M5GetVSZ0(const AccessTarget& accessTarget) const;
	inline void M5SetVSZ0(const AccessTarget& accessTarget, bool data);
	inline bool M5GetHSZ1(const AccessTarget& accessTarget) const;
	inline void M5SetHSZ1(const AccessTarget& accessTarget, bool data);
	inline bool M5GetHSZ0(const AccessTarget& accessTarget) const;
	inline void M5SetHSZ0(const AccessTarget& accessTarget, bool data);

	inline bool M5GetWindowRightAligned(const AccessTarget& accessTarget) const;	//0x11(17)
	inline void M5SetWindowRightAligned(const AccessTarget& accessTarget, bool data);
	inline unsigned int M5GetWindowBasePointX(const AccessTarget& accessTarget) const;
	inline void M5SetWindowBasePointX(const AccessTarget& accessTarget, unsigned int data);
	inline bool M5GetWindowBottomAligned(const AccessTarget& accessTarget) const;	//0x12(18)
	inline void M5SetWindowBottomAligned(const AccessTarget& accessTarget, bool data);
	inline unsigned int M5GetWindowBasePointY(const AccessTarget& accessTarget) const;
	inline void M5SetWindowBasePointY(const AccessTarget& accessTarget, unsigned int data);

	inline unsigned int M5GetDMALengthCounter(const AccessTarget& accessTarget) const;	//0x13-0x14(19-20)
	inline void M5SetDMALengthCounter(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetDMASourceAddress(const AccessTarget& accessTarget) const;	//0x15-0x17(21-23)
	inline void M5SetDMASourceAddress(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetDMASourceAddressByte1(const AccessTarget& accessTarget) const;	//0x15(21)
	inline void M5SetDMASourceAddressByte1(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetDMASourceAddressByte2(const AccessTarget& accessTarget) const;	//0x16(22)
	inline void M5SetDMASourceAddressByte2(const AccessTarget& accessTarget, unsigned int data);
	inline unsigned int M5GetDMASourceAddressByte3(const AccessTarget& accessTarget) const;	//0x17(23)
	inline void M5SetDMASourceAddressByte3(const AccessTarget& accessTarget, unsigned int data);
	inline bool M5GetDMD1(const AccessTarget& accessTarget) const;
	inline void M5SetDMD1(const AccessTarget& accessTarget, bool data);
	inline bool M5GetDMD0(const AccessTarget& accessTarget) const;
	inline void M5SetDMD0(const AccessTarget& accessTarget, bool data);

private:
	//Menu handling
	DebugMenuHandler* menuHandler;

	//Bus interface
	IBusInterface* memoryBus;
	bool busGranted;
	bool bbusGranted;
	bool palModeLineState;
	bool bpalModeLineState;

	//Embedded PSG device
	IDevice* psg;

	//Image buffer
	//##TODO## Fix up these dimensions
	mutable boost::mutex imageBufferMutex;
	static const unsigned int imageWidth = 512;
	static const unsigned int imageHeight = 512;
	static const unsigned int imageBufferPlanes = 3;
	unsigned char image[imageBufferPlanes][imageHeight * imageWidth * 4];
	unsigned int drawingImageBufferPlane;

	//Physical registers and memory buffers
	mutable boost::mutex accessMutex;
	double lastAccessTime;
	RegBuffer reg;
	ITimedBufferInt* vram;
	ITimedBufferInt* cram;
	ITimedBufferInt* vsram;
	Data status;
	Data bstatus;
	Data hcounter;
	Data bhcounter;
	Data vcounter;
	Data bvcounter;
	Data hcounterLatchedData;
	Data bhcounterLatchedData;
	Data vcounterLatchedData;
	Data bvcounterLatchedData;
	bool hvCounterLatched;
	bool bhvCounterLatched;
	unsigned int hintCounter;
	unsigned int bhintCounter;
	bool vintPending;
	bool bvintPending;
	bool hintPending;
	bool bhintPending;
	bool exintPending;
	bool bexintPending;

	//Cached register settings
	bool hvCounterLatchEnabled;
	bool bhvCounterLatchEnabled;
	bool interlaceEnabled;
	bool binterlaceEnabled;
	bool interlaceDouble;
	bool binterlaceDouble;
	bool screenModeH40;
	bool bscreenModeH40;
	bool screenModeV30;
	bool bscreenModeV30;
	bool palMode;
	bool bpalMode;
	bool vintEnabled;
	bool bvintEnabled;
	bool hintEnabled;
	bool bhintEnabled;
	bool exintEnabled;
	bool bexintEnabled;
	unsigned int hintCounterReloadValue;
	unsigned int bhintCounterReloadValue;
	bool dmaEnabled;
	bool bdmaEnabled;
	bool dmd0;
	bool bdmd0;
	bool dmd1;
	bool bdmd1;
	unsigned int dmaLengthCounter;
	unsigned int bdmaLengthCounter;
	unsigned int dmaSourceAddressByte1;
	unsigned int bdmaSourceAddressByte1;
	unsigned int dmaSourceAddressByte2;
	unsigned int bdmaSourceAddressByte2;
	unsigned int dmaSourceAddressByte3;
	unsigned int bdmaSourceAddressByte3;
	unsigned int autoIncrementData;
	unsigned int bautoIncrementData;
	bool interlaceEnabledCached;
	bool binterlaceEnabledCached;
	bool interlaceDoubleCached;
	bool binterlaceDoubleCached;
	bool screenModeH40Cached;
	bool bscreenModeH40Cached;
	bool screenModeV30Cached;
	bool bscreenModeV30Cached;
	bool cachedDMASettingsChanged;

	//FIFO buffer registers
	unsigned int fifoNextReadEntry;
	unsigned int bfifoNextReadEntry;
	unsigned int fifoNextWriteEntry;
	unsigned int bfifoNextWriteEntry;
	std::vector<FIFOBufferEntry> fifoBuffer;
	std::vector<FIFOBufferEntry> bfifoBuffer;
	Data readBuffer;
	Data breadBuffer;
	bool readDataAvailable;
	bool breadDataAvailable;
	bool readDataHalfCached;
	bool breadDataHalfCached;
	bool dmaFillOperationRunning;
	bool bdmaFillOperationRunning;
	//##TODO## Implement this data register
	Data vsramCachedRead;
	Data bvsramCachedRead;

	//##TODO## Sprite buffer registers
	//We know from this: http://gendev.spritesmind.net/forum/viewtopic.php?t=666&postdays=0&postorder=asc&highlight=fifo&start=0
	//And this: http://mamedev.emulab.it/haze/2006/08/09/mirror-mirror/
	//That the sprite table is partially buffered internally. We need to implement that
	//sprite buffer here, and snoop on VRAM writes to update it, just like the real VDP
	//does.
	//##TODO## We know from testing that the sprite table cache simply caches the first
	//4 bytes of each 8-byte sprite entry. Implement that here as a simple data buffer.

	//Update state
	double lastTimesliceRemainingTime; //Rolling value, the difference between the last timeslice length, and the actual period of time we advanced the state.
	double blastTimesliceRemainingTime;
	double currentTimesliceLength; //The length of the current timeslice, as passed to NotifyUpcomingTimeslice().
	double currentTimesliceMclkCyclesRemainingTime; //Rolling value, the unused portion of the current timeslice which wasn't consumed by an mclk cycle.
	double bcurrentTimesliceMclkCyclesRemainingTime;
	unsigned int currentTimesliceTotalMclkCycles; //The total number of mclk cycles added by the current timeslice, including the currentTimesliceMclkCyclesRemainingTime value.
	double stateLastUpdateTime;
	unsigned int stateLastUpdateMclk;
	//##TODO## I think we've solved this issue now. Confirm, and remove these comments.
	//##FIX## This bleeds across timeslices, and we don't have a backup of it. More than
	//that, this is causing accuracy problems. Unused mclk cycles from a previous
	//timeslice are free, and need to be added in on every update step until they are
	//consumed. They do not count towards the target cycle count. Unused mclk cycles from
	//the current timeslice are not free, and need to be counted towards the target cycle
	//count.
	unsigned int stateLastUpdateMclkUnused;
	unsigned int stateLastUpdateMclkUnusedFromLastTimeslice;
	unsigned int bstateLastUpdateMclkUnusedFromLastTimeslice;

	//Control port registers
	bool codeAndAddressRegistersModifiedSinceLastWrite;
	bool commandWritePending;
	bool bcommandWritePending;
	Data originalCommandAddress;
	Data boriginalCommandAddress;
	Data commandAddress;
	Data bcommandAddress;
	Data commandCode;
	Data bcommandCode;

	//Render thread properties
	mutable boost::mutex renderThreadMutex;
	mutable boost::mutex timesliceMutex;
	boost::condition renderThreadUpdate;
	boost::condition renderThreadStopped;
	bool renderThreadActive;
	bool pendingRenderOperation;
	double pendingRenderTimesliceLength;
	double bpendingRenderTimesliceLength;
	bool renderTimeslicePending;
	RegBuffer::Timeslice regTimeslice;
	ITimedBufferInt::Timeslice* vramTimeslice;
	ITimedBufferInt::Timeslice* cramTimeslice;
	ITimedBufferInt::Timeslice* vsramTimeslice;
	double remainingRenderTime;
	RegBuffer::Timeslice regTimesliceCopy;
	ITimedBufferInt::Timeslice* vramTimesliceCopy;
	ITimedBufferInt::Timeslice* cramTimesliceCopy;
	ITimedBufferInt::Timeslice* vsramTimesliceCopy;
	RegBuffer::AdvanceSession regSession;
	ITimedBufferInt::AdvanceSession vramSession;
	ITimedBufferInt::AdvanceSession cramSession;
	ITimedBufferInt::AdvanceSession vsramSession;
	unsigned int mclkCycleRenderProgress;
	static const unsigned int layerPriorityLookupTableSize = 0x200;
	std::vector<unsigned int> layerPriorityLookupTable;

	//DMA worker thread properties
	mutable boost::mutex workerThreadMutex;
	boost::condition workerThreadUpdate;
	boost::condition workerThreadStopped;
	boost::condition workerThreadIdle;
	bool workerThreadActive;
	bool workerThreadPaused;
	bool bworkerThreadPaused;
	double busGrantTime;
	double bbusGrantTime;

	//DMA transfer registers
	//##TODO## Calculate on the hardware how many mclk cycles it takes a DMA transfer
	//update step to read data from memory over the external bus. We can do this by
	//measuring how quickly the data is transferred back to back while the display is
	//disabled. We need to know the actual delay time between successive reads, so
	//that we don't perform DMA transfers too quickly when the display is disabled, or
	//not in an active display region.
	static const unsigned int dmaTransferReadTimeInMclkCycles = 8;
	bool dmaTransferActive;
	bool bdmaTransferActive;
	bool dmaTransferReadDataCached;
	bool bdmaTransferReadDataCached;
	Data dmaTransferReadCache;
	Data bdmaTransferReadCache;
	unsigned int dmaTransferNextReadMclk;
	unsigned int bdmaTransferNextReadMclk;
	unsigned int dmaTransferLastTimesliceUnusedMclkCycles;
	unsigned int bdmaTransferLastTimesliceUnusedMclkCycles;

	//Interrupt settings
	//##TODO## Add rollback settings for these values
	bool externalInterruptPending;
	unsigned int externalInterruptHCounter;
	unsigned int externalInterruptVCounter;

	//Event handling
	bool eventsProcessedForTimeslice;
	boost::condition eventProcessingCompleted;
	unsigned int eventLastUpdateMclk;
	mutable EventProperties nextEventToExecute;
	EventProperties bnextEventToExecute;
	mutable double nextEventToExecuteTime;
	double bnextEventToExecuteTime;
	//##TODO## Implement event snapshots
	bool eventSnapshotSaved;
	bool beventSnapshotSaved;
	bool eventSnapshotScreenModeH40;
	bool beventSnapshotScreenModeH40;
	bool eventSnapshotScreenModeV30;
	bool beventSnapshotScreenModeV30;
	bool eventSnapshotPalMode;
	bool beventSnapshotPalMode;
	bool eventSnapshotInterlaceEnabled;
	bool beventSnapshotInterlaceEnabled;

	//CE line masks
	unsigned int ceLineMaskLowerDataStrobeInput;
	unsigned int ceLineMaskUpperDataStrobeInput;
	unsigned int ceLineMaskReadHighWriteLowInput;
	unsigned int ceLineMaskAddressStrobeInput;
	unsigned int ceLineMaskLowerDataStrobeOutput;
	unsigned int ceLineMaskUpperDataStrobeOutput;
	unsigned int ceLineMaskReadHighWriteLowOutput;
	unsigned int ceLineMaskAddressStrobeOutput;
	unsigned int ceLineMaskRMWCycleInProgress;
	unsigned int ceLineMaskRMWCycleFirstOperation;
	unsigned int ceLineMaskLWR;
	unsigned int ceLineMaskUWR;
	unsigned int ceLineMaskCAS0;
	unsigned int ceLineMaskRAS0;
	unsigned int ceLineMaskOE0;

	//Saved CE line state for Read-Modify-Write cycles
	mutable bool lineLWRSavedStateRMW;
	mutable bool lineUWRSavedStateRMW;
	mutable bool lineCAS0SavedStateRMW;
	mutable bool lineRAS0SavedStateRMW;
	mutable bool lineOE0SavedStateRMW;
	bool blineLWRSavedStateRMW;
	bool blineUWRSavedStateRMW;
	bool blineCAS0SavedStateRMW;
	bool blineRAS0SavedStateRMW;
	bool blineOE0SavedStateRMW;
};

#include "S315_5313.inl"
#endif