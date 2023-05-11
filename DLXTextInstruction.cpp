#include "DLXTextInstruction.h"
#include <sstream>
#include "utils.h"

using namespace std;


#pragma region DLXTextInstruction 
DLXTextInstruction::DLXTextInstruction(const string& opcode)
	: opcode_(opcode)
{
}

DLXTextInstruction::DLXTextInstruction(string&& opcode)
	: opcode_(std::move(opcode))
{
}

const string& DLXTextInstruction::opcode()
{
	return opcode_;
}

string DLXTextInstruction::toString()
{
	return opcode_;
}

DLXTextInstruction::~DLXTextInstruction()
{}

shared_ptr<DLXTextInstruction> DLXTextInstruction::parse(const string& text)
{
	stringstream instruction_stream(text);

	string opcode;
	instruction_stream >> opcode;

	vector<string> tokens;
	string token;
	bool allRegisters = true;

	while (getline(instruction_stream, token, ','))
	{
		trim(token);
		allRegisters = allRegisters && isRegisterName(token);
		tokens.push_back(std::move(token));
	}

	if (tokens.empty())
		return shared_ptr<DLXTextInstruction>(new DLXTextInstruction(std::move(opcode)));

	if (allRegisters)
		return shared_ptr<DLXTextInstruction>(new DLXRTypeTextInstruction(std::move(opcode),std::move(tokens)));

	if (tokens.size() == 3 && isRegisterName(tokens[0]) && isHexNumber(tokens[1]) && isRegisterName(tokens[2]))
		return shared_ptr<DLXTextInstruction>(new DLXITypeTextInstruction(std::move(opcode), { tokens[0], tokens[2] }, stoi(tokens[1], nullptr, 16)));

	if (tokens.size() == 2 && isRegisterName(tokens[0]) && isIndirectAddressing(tokens[1]))
		return shared_ptr<DLXTextInstruction>(new DLXMTypeTextInstruction(std::move(opcode), std::move(tokens[0]), getOffsetFromIndirectAddressing(tokens[1]), getIndexRegisterFromIndirectAddressing(tokens[1])));

	if (tokens.size() == 2 && isRegisterName(tokens[0]) && !isRegisterName(tokens[1]) && !isHexNumber(tokens[1]))
		return shared_ptr<DLXTextInstruction>(new DLXJTypeTextInstruction(std::move(opcode), std::move(tokens[0]), std::move(tokens[1])));

	string message = "Unknown instruction format: " + text;
	throw DLXJITException(message.c_str());
}
#pragma endregion

#pragma region DLXRTypeTextInstruction

DLXRTypeTextInstruction::DLXRTypeTextInstruction(const string& opcode, const RegistersCollection& registers)
	: DLXTextInstruction(opcode), registers(registers)
{
}

DLXRTypeTextInstruction::DLXRTypeTextInstruction(string&& opcode, RegistersCollection&& registers)
	: DLXTextInstruction(move(opcode)), registers(move(registers))
{
}

const std::string& DLXRTypeTextInstruction::reg(RegistersCollection::size_type no)
{
	return registers[no];
}

DLXRTypeTextInstruction::RegistersCollection::size_type DLXRTypeTextInstruction::numberOfRegisters()
{
	return registers.size();
}

string DLXRTypeTextInstruction::toString()
{
	stringstream str;
	str << opcode() << "\t";
	for (int i = 0; i < registers.size(); i++)
	{
		if (i != 0)
			str << ", ";
		str << registers[i];
	}
	return str.str();
}

DLXRTypeTextInstruction::~DLXRTypeTextInstruction()
{
}

#pragma endregion

#pragma region DLXITypeTextInstruction
DLXITypeTextInstruction::DLXITypeTextInstruction(const std::string& opcode, const RegistersCollection& registers, int32_t immediate)
	: DLXRTypeTextInstruction(opcode, registers), immediate_(immediate)
{
}

DLXITypeTextInstruction::DLXITypeTextInstruction(std::string&& opcode, RegistersCollection&& registers, int32_t immediate)
	: DLXRTypeTextInstruction(std::move(opcode), std::move(registers)), immediate_(immediate)
{

}

int32_t DLXITypeTextInstruction::immediate()
{
	return immediate_;
}

string DLXITypeTextInstruction::toString()
{
	auto regNo = numberOfRegisters();
	if (regNo == 0)
		return DLXTextInstruction::toString();

	stringstream str;
	str << opcode() << "\t" << reg(0) << ", " << std::hex << immediate_;
	


	for (int i = 1; i < regNo; i++)
	{
		str << ", ";
		str << reg(i);
	}

	return str.str();
}

DLXITypeTextInstruction::~DLXITypeTextInstruction()
{}
#pragma endregion

#pragma region DLXMTypeTextInstruction
DLXMTypeTextInstruction::DLXMTypeTextInstruction(const std::string& opcode, const std::string& dataRegister, int32_t baseAddress, const std::string& indexRegister)
	: DLXRTypeTextInstruction(opcode, { indexRegister, dataRegister }), baseAddress_(baseAddress)
{
}

DLXMTypeTextInstruction::DLXMTypeTextInstruction(std::string&& opcode, std::string&& dataRegister, int32_t baseAddress, std::string&& indexRegister)
	: DLXRTypeTextInstruction(move(opcode), { move(indexRegister), move(dataRegister) }), baseAddress_(baseAddress)
{
}

int32_t DLXMTypeTextInstruction::baseAddress()
{
	return baseAddress_;
}

const std::string& DLXMTypeTextInstruction::dataRegister()
{
	return reg(1);
}

const std::string& DLXMTypeTextInstruction::indexRegister()
{
	return reg(0);
}

string DLXMTypeTextInstruction::toString()
{
	stringstream str;
	str << opcode() << "\t" << dataRegister() << ", 0x" << std::hex << baseAddress_ << '(' << indexRegister() << ')';

	return str.str();
}

DLXMTypeTextInstruction::~DLXMTypeTextInstruction()
{}
#pragma endregion

#pragma region DLXJTypeTextInstruction
DLXJTypeTextInstruction::DLXJTypeTextInstruction(const std::string& opcode, const std::string& branchRegister, const std::string& label)
	: DLXRTypeTextInstruction(opcode, { branchRegister }), label_(label)
{
}

DLXJTypeTextInstruction::DLXJTypeTextInstruction(std::string&& opcode, std::string&& branchRegister, std::string&& label)
	: DLXRTypeTextInstruction(std::move(opcode), { std::move(branchRegister) }), label_(std::move(label))
{

}
const std::string& DLXJTypeTextInstruction::label()
{
	return label_;
}

const std::string& DLXJTypeTextInstruction::branchRegister()
{
	return reg(0);
}

string DLXJTypeTextInstruction::toString()
{
	stringstream str;
	str << opcode() << "\t" << branchRegister() << ", " << label_;

	return str.str();
}

DLXJTypeTextInstruction::~DLXJTypeTextInstruction()
{}
#pragma endregion