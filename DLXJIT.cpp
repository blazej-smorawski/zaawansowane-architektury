#include "DLXJIT.h"
#include <iostream>
#include <string>
#include <sstream>
#include <functional>
#include "utils.h"

#if defined(_M_X64)
#include "DLXJITx64.h"
#else
#error "No implementation for current architecture"
#endif

using namespace std;

DLXJIT::DLXJIT()
	: numberOfDLXRegisters(32)
{
}

void DLXJIT::loadCode(istream& codStream)
{
	string line;
	getline(codStream,line);
	if (line != "[Code Memory Content]")
		throw DLXJITException("Invalid cod file");

	while (getline(codStream, line))
	{
		stringstream linestream(line);
		string iaddr, icode, label, itext;

		getline(linestream, iaddr, ':');
		trim(iaddr);
		getline(linestream, icode, '|');
		trim(icode);
		getline(linestream, label, '|');
		trim(label);
		getline(linestream, itext);
		trim(itext);

		DLXJITCodLine codLine = { stoul(iaddr, nullptr, 16), stoul(icode, nullptr, 16), std::move(label), DLXTextInstruction::parse(itext)};
		codContent.push_back(std::move(codLine));

		auto last = codContent.size() - 1;
		if (codContent[last].label != "")
		{
			labelDictionary[codContent[last].label] = last;
		}
	}

}

enum DLXDatRepresentationSize
{
	BYTE = 0,
	HALF = 1,
	WORD = 2
};

enum DLXDatRepresentationFormatBase
{
	UNSIGNED_HEXADECIMAL = 0,
	UNSIGNED_DECIMAL = 1,
	SIGNED_DECIMAL = 2
};


template<typename N = uint32_t>
void readDataFromDatFile(std::istream& datStream, std::function<void(std::size_t, N)> saveInmemory, bool hexFormat)
{
	string line;
	while (getline(datStream, line))
	{
		stringstream lineStream(line);
		
		if (hexFormat)
			lineStream >> std::hex;

		int64_t buf;
		lineStream >> buf;
		std::size_t address = buf;

		char ignore;
		lineStream >> ignore;

		for (int i = 0; i < 8; i++)
		{
			lineStream >> buf;
			saveInmemory(address, (N)buf);
			address += sizeof(N);
		}
	}
}

void DLXJIT::loadData(std::istream& datStream)
{
	string line;
	getline(datStream, line);
	if (line != "[Data Memory]")
		throw DLXJITException("Invalid dat file");

	DLXDatRepresentationSize size = WORD;
	DLXDatRepresentationFormatBase base = UNSIGNED_HEXADECIMAL;

	const string sizeKey = "Size=";
	const string baseKey = "Base=";

	while (getline(datStream, line) && line != "[Data Memory Content]")
	{
		if (line.compare(0, sizeKey.size(), sizeKey) == 0)
		{
			size = (DLXDatRepresentationSize)stol(line.substr(sizeKey.size()));
		}
		else if (line.compare(0, baseKey.size(), baseKey) == 0)
		{
			base = (DLXDatRepresentationFormatBase)stol(line.substr(baseKey.size()));
		}
	}

	if (datStream.eof() || line != "[Data Memory Content]")
		throw DLXJITException("Invalid dat file - no content");


	const bool isHexEnabled = base == UNSIGNED_HEXADECIMAL;

	switch (size)
	{
	case BYTE:
		readDataFromDatFile<uint8_t>(datStream, [=](std::size_t a, uint8_t value) {return this->saveByteInMemory(a,value); }, isHexEnabled);
		break;
	case HALF:
		readDataFromDatFile<uint16_t>(datStream, [=](std::size_t a, uint16_t value) {return this->saveHalfInMemory(a, value); }, isHexEnabled);
		break;
	default:
	case WORD:
		readDataFromDatFile<uint32_t>(datStream, [=](std::size_t a, uint32_t value) {return this->saveWordInMemory(a, value); }, isHexEnabled);
		break;
	}

}

void DLXJIT::saveData(std::ostream& datStream)
{
	datStream << "[Data Memory]" << endl;
	datStream << "Size=" << WORD << endl;
	datStream << "Base=" << UNSIGNED_HEXADECIMAL << endl << endl;

	datStream << "[Data Memory Content]" << endl;
	auto size = getDataMemorySize();

	datStream << std::hex;
	datStream.fill('0');

	for (size_t address = 0; address < size; address += 4)
	{
		if (address % (8 * 4) == 0)
		{
			datStream.width(3);
			datStream << address << ':';
		}

		datStream.width(0);
		datStream << "  ";
		datStream.width(8);
		datStream << this->loadWordFromMemory(address);

		if (address % (8 * 4) == (7 * 4 ))
		{
			datStream << endl;
		}
	}
}

DLXJIT::~DLXJIT()
{
}

shared_ptr<DLXJIT> DLXJIT::createInstance()
{
#if defined(_M_X64)
	return shared_ptr<DLXJIT>(new DLXJITx64());
#else
#error "No implementation for current architecture"
#endif
}