#pragma once
#include <exception>

class DLXJITException : public std::exception
{
public:
	DLXJITException();
	DLXJITException(const char* message);
	DLXJITException(std::exception& cause);

	~DLXJITException() throw() override;
};