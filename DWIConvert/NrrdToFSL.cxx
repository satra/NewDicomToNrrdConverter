#include "DWIConvertUtils.h"

int NrrdToFSL(const std::string &inputVolume,
              const std::string &outputVolume,
              const std::string &outputBValues,
              const std::string &outputBVectors)
{
  if(CheckArg<std::string>("Input Volume",inputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("Output Volume",outputVolume,"") == EXIT_FAILURE ||
     CheckArg<std::string>("B Values", outputBValues, "") == EXIT_FAILURE ||
     CheckArg<std::string>("B Vectors", outputBVectors, ""))
    {
    return EXIT_FAILURE;
    }
}
