#if defined(_M_X64) || defined(__x86_64__)

#include "DLXJITx64.h"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__)
#include <sys/mman.h>
#endif
#include <iostream>
#include <ctime>
#include <cstring>
#include "utils.h"

#if defined(__GNUG__)
#define DATA_POINTER_REGISTER RDI
#define swap16 __builtin_bswap16
#define swap32 __builtin_bswap32
#elif defined(_MSC_VER)
#define DATA_POINTER_REGISTER RCX
#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#endif


using namespace std;


template<typename T>
inline void serialize(std::vector<char>& code, T& serializable)
{
	char* ptr = (char*)&serializable;
	for (int i = 0; i < sizeof(T); i++)
		code.push_back(ptr[i]);
}

inline int getDLXRegisterNumber(const std::string& name)
{
	if (!isRegisterName(name))
		throw DLXJITException(("Invalid register name: " + name).c_str());

	return stoi(name.substr(1));
}

inline int getDLXRegisterOffsetOnStack(int regNumber)
{
	return -(regNumber + 1) * 4;
}

DLXJITx64::DLXJITx64()
	: program(nullptr)
{
}


void DLXJITx64::saveWordInMemory(std::size_t address, uint32_t value)
{
	if (address + 4 > data.size())
		data.resize(address + 4);
	*((uint32_t*)&data[address]) = swap32(value);
}

void DLXJITx64::saveHalfInMemory(std::size_t address, uint16_t value)
{
	if (address + 2 > data.size())
		data.resize(address + 2);
	*((uint16_t*)&data[address]) = swap16(value);
}

void DLXJITx64::saveByteInMemory(std::size_t address, uint8_t value)
{
	if (address + 1 > data.size())
		data.resize(address + 1);
	data[address] = value;
}

uint32_t DLXJITx64::loadWordFromMemory(std::size_t address)
{
	if (address + 4 > data.size())
		data.resize(address + 4);
	return swap32(*((uint32_t*)&data[address]));
}

uint16_t DLXJITx64::loadHalfFromMemory(std::size_t address)
{
	if (address + 2 > data.size())
		data.resize(address + 2);
	return swap16(*((uint16_t*)&data[address]));
}

uint8_t DLXJITx64::loadByteFromMemory(std::size_t address)
{
	if (address + 1 > data.size())
		data.resize(address + 1);
	return data[address];
}


std::size_t DLXJITx64::getDataMemorySize()
{
	return data.size();
}

void DLXJITx64::writeREXPrefix(bool W, bool R, bool X, bool B)
{
	REXPrefixByte byte(W, R, X, B);
	serialize(rawCode, byte);
}

void DLXJITx64::writeModRM(uint8_t mod, uint8_t r, uint8_t rm)
{
	ModRMByte byte(mod,r,rm);
	serialize(rawCode,byte);
}

void DLXJITx64::writeSIB(uint8_t scale, uint8_t index, uint8_t base)
{
	SIBByte byte(scale, index, base);
	serialize(rawCode, byte);
}


void DLXJITx64::writeAdd(Reg64 dest, int32_t imm32)
{
	writeREXPrefix(true, false, false, dest > 0x7);
	rawCode.push_back(0x81);
	writeModRM(REGISTER, 0, dest);
	serialize(rawCode, imm32);
}


void DLXJITx64::writeSub(Reg64 dest, int32_t imm32)
{
	writeREXPrefix(true, false, false, dest > 0x7);
	rawCode.push_back(0x81);
	writeModRM(REGISTER, 5, dest);
	serialize(rawCode,imm32);
}

void DLXJITx64::writeMovR64toR64(Reg64 dest, Reg64 op)
{
	writeREXPrefix(true, dest > 0x7, false, op > 0x7);
	rawCode.push_back(0x8b);
	writeModRM(REGISTER, dest, op);
}

void DLXJITx64::writeMovImm32toR64(Reg64 dest, int32_t imm32)
{
	writeREXPrefix(true, false, false, dest > 0x7);
	rawCode.push_back(0xc7);
	writeModRM(REGISTER, 0, dest);
	serialize(rawCode, imm32);
}

void DLXJITx64::writeMovImm64toR64(Reg64 dest, int64_t imm64)
{
	writeREXPrefix(true, false, false, dest > 0x7);
	rawCode.push_back(0xb8 + (dest & 0x7));
	serialize(rawCode, imm64);
}

void DLXJITx64::writeMovImm32toMem32(int32_t offset, Reg64 index, int32_t imm32)
{
	if (index != RSP)
	{
		writeREXPrefix(false, false, false, index > 0x7);
		rawCode.push_back(0xc7);
		writeModRM(REGISTER_ADDRESSING_DISP32, 0, index);
		serialize(rawCode, offset);
		serialize(rawCode, imm32);
	}
	else
	{
		writeREXPrefix(false, false, index > 0x7, false);
		rawCode.push_back(0xc7);
		writeModRM(REGISTER_ADDRESSING_DISP32, 0, 0x4);
		writeSIB(0, 0x4, index);
		serialize(rawCode, offset);
		serialize(rawCode, imm32);
	}
}

void DLXJITx64::writeMovSXDReg32toReg64(Reg64 dest, Reg32 source)
{
	writeREXPrefix(true, dest > 0x7, false, source > 0x7);
	rawCode.push_back(0x63);
	writeModRM(REGISTER, dest, source);
}

void DLXJITx64::writeMovSXDMem32toReg64(Reg64 dest, int32_t offset, Reg64 index)
{
	if (index != RSP)
	{
		writeREXPrefix(true, dest > 0x7, false, index > 0x7);
		rawCode.push_back(0x63);
		writeModRM(REGISTER_ADDRESSING_DISP32, dest, index);
		serialize(rawCode, offset);
	}
	else
	{
		writeREXPrefix(true, dest > 0x7, index > 0x7, false);
		rawCode.push_back(0x63);
		writeModRM(REGISTER_ADDRESSING_DISP32, dest, 0x4);
		writeSIB(0, 0x4, index);
		serialize(rawCode, offset);
	}
}

void DLXJITx64::writeMovSXDMem32toReg64(Reg64 dest, int32_t offset, SIBScale scale, Reg64 index, Reg64 base)
{
	//TODO: Uzupe³niæ
}

void DLXJITx64::writeMovReg32toMem32(int32_t offset, Reg64 index, Reg32 source)
{
	//TODO: Uzupe³niæ
}

void DLXJITx64::writeMovReg32toMem32(int32_t offset, SIBScale scale, Reg64 index, Reg64 base, Reg32 source)
{
	if (source > 0x7 || index > 0x7 || base > 0x7)
		writeREXPrefix(false, source > 0x7, index > 0x7, base > 0x7);
	rawCode.push_back(0x89);
	writeModRM(REGISTER_ADDRESSING_DISP32, source, 0x4);
	writeSIB(scale, index, base);
	serialize(rawCode, offset);
}

void DLXJITx64::writeMovMem32toReg32(Reg32 dest, int32_t offset, Reg64 index)
{
	bool rexRequired = dest > 0x7 || index > 0x7;
	if (index != RSP)
	{
		if (rexRequired)
			writeREXPrefix(false, dest > 0x7, false, index > 0x7);
		rawCode.push_back(0x8b);
		writeModRM(REGISTER_ADDRESSING_DISP32, dest, index);
		serialize(rawCode, offset);
	}
	else
	{
		if (rexRequired)
			writeREXPrefix(false, dest > 0x7, index > 0x7, false);
		rawCode.push_back(0x8b);
		writeModRM(REGISTER_ADDRESSING_DISP32, dest, 0x4);
		writeSIB(0, 0x4, index);
		serialize(rawCode, offset);
	}
}

void DLXJITx64::writeMovMem32toReg32(Reg32 dest, int32_t offset, SIBScale scale, Reg64 index, Reg64 base)
{
	if (dest > 0x7 || index > 0x7 || base > 0x7)
		writeREXPrefix(false, dest > 0x7, index > 0x7, base > 0x7);
	rawCode.push_back(0x8b);
	writeModRM(REGISTER_ADDRESSING_DISP32, dest, 0x4);
	writeSIB(scale, index, base);
	serialize(rawCode, offset);
}

void DLXJITx64::writeMovImm32toMem64(int32_t offset, Reg64 index, int32_t imm32)
{
	if (index != RSP)
	{
		writeREXPrefix(true, false, false, index > 0x7);
		rawCode.push_back(0xc7);
		writeModRM(REGISTER_ADDRESSING_DISP32, 0, index);
		serialize(rawCode, offset);
		serialize(rawCode, imm32);
	}
	else
	{
		writeREXPrefix(true, false, index > 0x7, false);
		rawCode.push_back(0xc7);
		writeModRM(REGISTER_ADDRESSING_DISP32, 0, 0x4);
		writeSIB(0, 0x4, index);
		serialize(rawCode, offset);
		serialize(rawCode, imm32);
	}
}

void DLXJITx64::writePop(Reg64 dest)
{
	if (dest < 0x7)
	{
		rawCode.push_back(0x58 + (dest & 0x7));
	}
	else
	{
		writeREXPrefix(true, false, false, dest > 0x7);
		rawCode.push_back(0x8f);
		writeModRM(REGISTER, 0, dest);
	}
}

void DLXJITx64::writePush(Reg64 dest)
{
	if (dest < 0x7)
	{
		rawCode.push_back(0x50 + (dest & 0x7));
	}
	else
	{
		writeREXPrefix(true, false, false, dest > 0x7);
		rawCode.push_back(0xff);
		writeModRM(REGISTER, 6, dest);
	}
}

void DLXJITx64::writeJLE(int32_t offset)
{
	rawCode.push_back(0x0f);
	rawCode.push_back(0x8e);
	serialize(rawCode, offset);
}

void DLXJITx64::writeRet()
{
	rawCode.push_back(0xC3);
}


void DLXJITx64::compileDLXInstruction(const DLXJITCodLine& line)
{
	if (line.textInstruction->opcode() == "ADD")
	{
		shared_ptr<DLXRTypeTextInstruction> instr = dynamic_pointer_cast<DLXRTypeTextInstruction>(line.textInstruction);
		int destination = getDLXRegisterNumber(instr->reg(2));
		if (destination > 0) //Zapisywanie do rejsetru R0 jest niedozwolone
		{
			int s1 = getDLXRegisterNumber(instr->reg(0));
			int s2 = getDLXRegisterNumber(instr->reg(1));
			writeMovSXDMem32toReg64(RAX, getDLXRegisterOffsetOnStack(s1), RBP); //Odczyt pierwszego rejestru Ÿród³owego DLX
			writeMovSXDMem32toReg64(RDX, getDLXRegisterOffsetOnStack(s2), RBP); //Odczyt drugiego rejestru Ÿród³owego DLX
			//writeAdd(RAX, RDX);
			writeMovReg32toMem32(getDLXRegisterOffsetOnStack(destination), RBP, EAX); //Zapis wyniku
		}
	}
	else if (line.textInstruction->opcode() == "ADDI")
	{
		shared_ptr<DLXITypeTextInstruction> instr = dynamic_pointer_cast<DLXITypeTextInstruction>(line.textInstruction);
		int destination = getDLXRegisterNumber(instr->reg(1));
		if (destination > 0) //Zapisywanie do rejsetru R0 jest niedozwolone
		{
			int s1 = getDLXRegisterNumber(instr->reg(0));
			writeMovSXDMem32toReg64(RAX, getDLXRegisterOffsetOnStack(s1), RBP); //Odczyt pierwszego rejestru Ÿród³owego DLX
			writeAdd(RAX, instr->immediate());
			writeMovReg32toMem32(getDLXRegisterOffsetOnStack(destination), RBP, EAX); //Zapis wyniku
		}
	}
	else if (line.textInstruction->opcode() == "LDW")
	{
		shared_ptr<DLXMTypeTextInstruction> instr = dynamic_pointer_cast<DLXMTypeTextInstruction>(line.textInstruction);
		int destination = getDLXRegisterNumber(instr->dataRegister());
		if (destination > 0) //Zapisywanie do rejsetru R0 jest niedozwolone
		{
			int index = getDLXRegisterNumber(instr->indexRegister());
			int offset = instr->baseAddress();
			writeMovSXDMem32toReg64(RDX, getDLXRegisterOffsetOnStack(index), RBP); //Odczyt rejsetru indeksuj¹cego DLX
			writeMovMem32toReg32(EAX, offset, x1, RDX, DATA_POINTER_REGISTER); //Odczyt danych z pamiêci
			//writeBswap(EAX); //Zamiana endianów
			//Niepotrzebne: writeMovSXDReg32toReg64(RAX, EAX); //Rozszerzanie liczby to 64 bitów
			writeMovReg32toMem32(getDLXRegisterOffsetOnStack(destination), RBP, EAX); //Zapis wyniku
		}
	}
	else if (line.textInstruction->opcode() == "BRLE")
	{
		shared_ptr<DLXJTypeTextInstruction> instr = dynamic_pointer_cast<DLXJTypeTextInstruction>(line.textInstruction);
		int branch = getDLXRegisterNumber(instr->branchRegister());

		writeMovSXDMem32toReg64(RAX, getDLXRegisterOffsetOnStack(branch), RBP); //Odczyt rejestru DLX zawieraj¹cego warunek skoku 
		//writeCmp(RAX, 0);
		int32_t offset = 0;
		auto destDlxInstr = labelDictionary[instr->label()];
		if (dlxOffsetsInRawCode.size() > destDlxInstr)
			offset = dlxOffsetsInRawCode[destDlxInstr] - (rawCode.size() + 6);
		else
			jumpOffsetsToRepair.push_back({ instr, rawCode.size() }); //Skok w przód!
		writeJLE(offset);
	}
	else if (line.textInstruction->opcode() == "NOP")
	{
		//NOP ;)
	}
	else
	{
		string message = "Unsupported DLX opcode: " + line.textInstruction->opcode();
		throw DLXJITException(message.c_str());
	}
}

void DLXJITx64::compile()
{
	rawCode.clear();
	dlxOffsetsInRawCode.clear();
	jumpOffsetsToRepair.clear();

	writePush(RBP);
	writeMovR64toR64(RBP, RSP);
	writeSub(RSP, 4 * numberOfDLXRegisters); //Alokowanie miejsca na rejestry DLX na stosie

	writeMovImm32toMem32(getDLXRegisterOffsetOnStack(0), RBP, 0); //Wpisywanie 0 do R0

	for (int i = 0; i < codContent.size(); i++)
	{
		dlxOffsetsInRawCode.push_back(rawCode.size());
		compileDLXInstruction(codContent[i]);
	}

	writeAdd(RSP, 4 * numberOfDLXRegisters); //Dealokacja rejestrów DLX ze stosu
	writeMovR64toR64(RSP, RBP);
	writePop(RBP);
	writeRet();

	for (auto& toRepair : this->jumpOffsetsToRepair) //Naprawianie skoków w przód
	{
		auto destDlxInstr = labelDictionary[toRepair.dlxInstruction->label()];
		auto destInRawCode = dlxOffsetsInRawCode[destDlxInstr];
		int32_t offset = destInRawCode - (toRepair.jumpInstructionOffset + 6);

		char* jumpInstructionLocation = &rawCode[toRepair.jumpInstructionOffset];
		int32_t* targetOffset = (int32_t*)(jumpInstructionLocation + 2);
		*targetOffset = offset;
	}

#if defined(_WIN32) || defined(_WIN64)
	void* mem = VirtualAlloc(nullptr, rawCode.size(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined(__linux__)
	void* mem = mmap(
		NULL,
		rawCode.size(),
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_PRIVATE,
		0,
		0);
#endif

	std::memcpy(mem, rawCode.data(), rawCode.size());

	program = (DlxProgram)mem;

#if 1
	{
		cout << "Compiled program size: " << rawCode.size() << " bytes" << endl;
		for (int i = 0; i < codContent.size(); i++)
		{
			cout << (codContent[i].label == "" ? "" : codContent[i].label+":") << "\t" << codContent[i].textInstruction->toString() << ":\t0x" << std::hex << (((unsigned int)mem) + dlxOffsetsInRawCode[i]) << endl;
		}
		cout << std::dec;
	}
#endif
}

void DLXJITx64::execute()
{
	if (program == nullptr)
		compile();
	uint8_t* data_ptr = data.data();
	program(data_ptr);
}


DLXJITx64::~DLXJITx64()
{
	if (program != nullptr)
	{
#if defined(_WIN32) || defined(_WIN64)
		VirtualFree(program, 0, MEM_RELEASE);
#elif defined(__linux__)
		munmap((void*)program, rawCode.size());
#endif
	}
}

#endif
