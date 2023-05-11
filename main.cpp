#include "DLXJIT.h"
#include <fstream>
#include <iostream>

#if defined(WIN32) || defined(_WIN32) 
#define PATH_SEPARATOR "\\" 
#else 
#define PATH_SEPARATOR "/" 
#endif 

int main(int argc, char** argv)
{
	const std::ios::iostate exceptionCauses = std::ios::badbit;
	if (argc < 4)
	{
		std::string programName(argv[0]);
		auto lastSep = programName.find_last_of(PATH_SEPARATOR);
		if (lastSep != std::string::npos)
			programName = programName.substr(lastSep + 1);
		std::cerr << "To few arguments. Please perform following call: " << std::endl;
		std::cerr << "\t" << programName << " input_cod_file input_dat_file output_dat_file" << std::endl;
		return -3;
	}

	std::string inputCodName(argv[1]);
	std::string inputDatName(argv[2]);
	std::string outputDatName(argv[3]);

	try
	{
		std::ifstream codFile(inputCodName);
		codFile.exceptions(exceptionCauses);
		std::ifstream datFile(inputDatName);
		datFile.exceptions(exceptionCauses);
		auto dlx = DLXJIT::createInstance();
		dlx->loadData(datFile);
		dlx->loadCode(codFile);
		dlx->execute();
		std::ofstream odatFile(outputDatName);
		odatFile.exceptions(exceptionCauses);
		dlx->saveData(odatFile);
		return 0;
	}
	catch (DLXJITException& ex)
	{
		std::cerr << ex.what() << std::endl;
		return -2;
	}
	catch (std::ios_base::failure& ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}
	return 0;
}