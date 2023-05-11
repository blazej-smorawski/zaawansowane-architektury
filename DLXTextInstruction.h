#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "DLXJITException.h"


class DLXTextInstruction
{
	std::string opcode_;
public:
	DLXTextInstruction(const std::string& opcode);
	DLXTextInstruction(std::string&& opcode);
	virtual const std::string& opcode();
	virtual std::string toString();
	virtual ~DLXTextInstruction();
	static std::shared_ptr<DLXTextInstruction> parse(const std::string&);
};

class DLXRTypeTextInstruction
	: public DLXTextInstruction
{
public:
	typedef std::vector<std::string> RegistersCollection;
private:
	RegistersCollection registers;
public:
	DLXRTypeTextInstruction(const std::string& opcode, const RegistersCollection& registers);
	DLXRTypeTextInstruction(std::string&& opcode, RegistersCollection&& registers);
	virtual const std::string& reg(RegistersCollection::size_type no);
	virtual RegistersCollection::size_type numberOfRegisters();
	std::string toString() override;
	~DLXRTypeTextInstruction() override;
};

class DLXITypeTextInstruction
	: public DLXRTypeTextInstruction
{
private:
	int32_t immediate_;
public:
	DLXITypeTextInstruction(const std::string& opcode, const RegistersCollection& registers, int32_t immediate);
	DLXITypeTextInstruction(std::string&& opcode, RegistersCollection&& registers, int32_t immediate);
	virtual int32_t immediate();
	std::string toString() override;
	~DLXITypeTextInstruction() override;
};

class DLXMTypeTextInstruction
	: public DLXRTypeTextInstruction
{
private:
	int32_t baseAddress_;
public:
	DLXMTypeTextInstruction(const std::string& opcode, const std::string& dataRegister, int32_t baseAddress, const std::string& indexRegister);
	DLXMTypeTextInstruction(std::string&& opcode, std::string&& dataRegister, int32_t baseAddress, std::string&& indexRegister);
	virtual int32_t baseAddress();
	virtual const std::string& dataRegister();
	virtual const std::string& indexRegister();
	std::string toString() override;
	~DLXMTypeTextInstruction() override;
};

class DLXJTypeTextInstruction
	: public DLXRTypeTextInstruction
{
private:
	std::string label_;
public:
	DLXJTypeTextInstruction(const std::string& opcode, const std::string& branchRegister, const std::string& label);
	DLXJTypeTextInstruction(std::string&& opcode, std::string&& branchRegister, std::string&& label);
	virtual const std::string& label();
	virtual const std::string& branchRegister();
	std::string toString() override;
	~DLXJTypeTextInstruction() override;
};

