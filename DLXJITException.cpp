#include "DLXJITException.h"

using namespace std;

DLXJITException::DLXJITException()
	: exception()
{
}

DLXJITException::DLXJITException(const char * message)
	: exception(message)
{
}

DLXJITException::DLXJITException(exception& cause)
	: exception(cause)
{
}

DLXJITException::~DLXJITException()
{
}
