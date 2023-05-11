#pragma once
#include <memory>
#include <cstdint>
#include <vector>
#include <map>
#include "DLXTextInstruction.h"
#include "DLXJITException.h"

//template<typename T>
//T swapBits(const T in, int bitCount = sizeof(T)*8)
//{
//	T ret = { 0 };
//	char* in_raw = (char*)&in;
//	char* out_raw = (char*)&ret;
//	for (int i = 0; i < bitCount; i++)
//	{
//		int dest_it = bitCount - i - 1;
//
//		unsigned char byte = in_raw[i / 8];
//		int in_bit_no = i % 8;
//		unsigned char in_bit_mask = 1 << in_bit_no;
//		unsigned char bit = (byte & in_bit_mask) >> in_bit_no;
//
//		int out_bit_no = dest_it % 8;
//		unsigned char out_bit_mask = 1 << out_bit_no;
//		unsigned char out_byte = bit << out_bit_no;
//
//		out_raw[dest_it / 8] |= (out_byte & out_bit_mask);
//	}
//	return ret;
//}


struct DLXJITCodLine
{
	uint32_t iaddr;
	uint32_t icode;
	std::string label;
	std::shared_ptr<DLXTextInstruction> textInstruction;
};

class DLXJIT
{
protected:
	int numberOfDLXRegisters;
	typedef std::vector<DLXJITCodLine> CodCollection;
	CodCollection codContent;
	typedef std::map<std::string, CodCollection::size_type> LabelDictionary;
	LabelDictionary labelDictionary;

	virtual void saveWordInMemory(std::size_t address, uint32_t data) = 0;
	virtual void saveHalfInMemory(std::size_t address, uint16_t data) = 0;
	virtual void saveByteInMemory(std::size_t address, uint8_t data) = 0;

	virtual uint32_t loadWordFromMemory(std::size_t address) = 0;
	virtual uint16_t loadHalfFromMemory(std::size_t address) = 0;
	virtual uint8_t loadByteFromMemory(std::size_t address) = 0;
public:
	DLXJIT();
	virtual void loadCode(std::istream& codStream);
	virtual void loadData(std::istream& datStream);
	virtual void saveData(std::ostream& datStream);
	virtual std::size_t getDataMemorySize() = 0;
	virtual void execute() = 0;
	virtual ~DLXJIT();

	static std::shared_ptr<DLXJIT> createInstance();
};

