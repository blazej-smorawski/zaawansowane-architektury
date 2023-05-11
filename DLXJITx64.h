#pragma once
#if defined(_M_X64) || defined(__x86_64__)
#include "DLXJIT.h"
#include <vector>

#if defined(_MSC_VER)
#pragma pack( push, DLXJITx64, 1 )
#define packed_attribute 
#elif defined(__GNUG__)
#define packed_attribute __attribute__((__packed__))
#endif

enum Reg32
{
	EAX = 0x0,
	ECX = 0x1,
	EDX = 0x2,
	EBX = 0x3,
	ESP = 0x4,
	EBP = 0x5,
	ESI = 0x6,
	EDI = 0x7,
	R8D = 0x8,
	R9D = 0x9,
	R10D = 0xa,
	R11D = 0xb,
	R12D = 0xc,
	R13D = 0xd,
	R14D = 0xe,
	R15D = 0xf,
};

enum Reg64
{
	RAX = 0x0,
	RCX = 0x1,
	RDX = 0x2,
	RBX = 0x3,
	RSP = 0x4,
	RBP = 0x5,
	RSI = 0x6,
	RDI = 0x7,
	R8 = 0x8,
	R9 = 0x9,
	R10 = 0xa,
	R11 = 0xb,
	R12 = 0xc,
	R13 = 0xd,
	R14 = 0xe,
	R15 = 0xf,
};

struct REXPrefixByte
{
	inline REXPrefixByte(uint8_t w, uint8_t r, uint8_t x, uint8_t b)
		: code(0x4), w(w), r(r), x(x), b(b)
	{
	}

	inline REXPrefixByte()
		: REXPrefixByte(0, 0, 0, 0)
	{}

	uint8_t b : 1;
	uint8_t x : 1;
	uint8_t r : 1;
	uint8_t w : 1;
	uint8_t code : 4; // <-- 0100
} packed_attribute;

enum ModRMMode
{
	REGISTER_ADDRESSING = 0x0,
	REGISTER_ADDRESSING_DISP8 = 0x1,
	REGISTER_ADDRESSING_DISP32 = 0x2,
	REGISTER = 0x3
};

struct ModRMByte
{
	inline ModRMByte(uint8_t mod, uint8_t r, uint8_t rm)
		: mod(mod), r(r), rm(rm)
	{
	}

	inline ModRMByte()
		: ModRMByte(0, 0, 0)
	{
	}

	uint8_t rm : 3;
	uint8_t r : 3;
	uint8_t mod : 2;
} packed_attribute;

enum SIBScale
{
	x1 = 0x0,
	x2 = 0x1,
	x4 = 0x2,
	x8 = 0x3
};

struct SIBByte
{
	inline SIBByte(uint8_t scale, uint8_t index, uint8_t base)
		: scale(scale), index(index), base(base)
	{
	}

	inline SIBByte()
		: SIBByte(0, 0, 0)
	{
	}

	uint8_t base : 3;
	uint8_t index : 3;
	uint8_t scale : 2;
} packed_attribute;

#if defined(_MSC_VER)
#pragma pack( pop, DLXJITx64 )
#endif

class DLXJITx64 :
	public DLXJIT
{
	typedef void(*DlxProgram)(uint8_t* data_memory);
	typedef std::vector<char> RawCodeContainer;
	RawCodeContainer rawCode;
	std::vector<uint8_t> data;
	DlxProgram program;

	std::vector<RawCodeContainer::size_type> dlxOffsetsInRawCode;

	struct JumpOffsetToRepair
	{
		std::shared_ptr<DLXJTypeTextInstruction> dlxInstruction;
		RawCodeContainer::size_type jumpInstructionOffset;
	};

	std::vector<JumpOffsetToRepair> jumpOffsetsToRepair;

	void compile();

	void writeREXPrefix(bool W, bool R, bool X, bool B);
	void writeModRM(uint8_t mod, uint8_t r, uint8_t rm);
	void writeSIB(uint8_t scale, uint8_t index, uint8_t base);

	void writeAdd(Reg64 dest, int32_t imm32);
	void writeAdd(Reg64 dest, Reg64 src);
	void writeXor(Reg64 dest, Reg64 src);
	void writeMul(Reg64 src);
	void writeBswap(Reg32 src);
	void writeSub(Reg64 dest, int32_t imm32);
	void writeMovR64toR64(Reg64 dest, Reg64 op);
	void writeMovImm32toR64(Reg64 dest, int32_t imm32);
	void writeMovImm64toR64(Reg64 dest, int64_t imm64);
	void writeMovReg32toMem32(int32_t offset, Reg64 index, Reg32 source);
	void writeMovReg32toMem32(int32_t offset, SIBScale scale, Reg64 index, Reg64 base, Reg32 source);
	void writeMovMem32toReg32(Reg32 dest, int32_t offset, Reg64 index);
	void writeMovMem32toReg32(Reg32 dest, int32_t offset, SIBScale scale, Reg64 index, Reg64 base);
	void writeMovImm32toMem32(int32_t offset, Reg64 index, int32_t imm32);
	void writeMovSXDReg32toReg64(Reg64 dest, Reg32 source);
	void writeMovSXDMem32toReg64(Reg64 dest, int32_t offset, Reg64 index);
	void writeMovSXDMem32toReg64(Reg64 dest, int32_t offset, SIBScale scale, Reg64 index, Reg64 base);
	void writeMovImm32toMem64(int32_t offset, Reg64 index, int32_t imm32);

	void writePop(Reg64 dest);
	void writePush(Reg64 dest);

	void writeJLE(int32_t offset);
	void writeJGE(int32_t offset);

	void writeRet();

	void compileDLXInstruction(const DLXJITCodLine& line);

protected:
	 void saveWordInMemory(std::size_t address, uint32_t data) override;
	 void saveHalfInMemory(std::size_t address, uint16_t data) override;
	 void saveByteInMemory(std::size_t address, uint8_t data) override;

	 uint32_t loadWordFromMemory(std::size_t address) override;
	 uint16_t loadHalfFromMemory(std::size_t address) override;
	 uint8_t loadByteFromMemory(std::size_t address) override;

public:
	DLXJITx64();
	std::size_t getDataMemorySize() override;
	void execute() override;
	~DLXJITx64() override;
};

#endif
