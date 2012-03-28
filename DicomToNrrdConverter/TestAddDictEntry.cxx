#include "dcdict.h"
#include "dcdicent.h"

void
AddDictEntry(DcmDictEntry *entry)
{
  DcmDataDictionary &dict = dcmDataDict.wrlock();
  dict.addEntry(entry);
  dcmDataDict.unlock();
}
int main(int argc, char *argv[])
{
  static DcmDictEntry GEDictBValue(0x0043, 0x1039, DcmVR(EVR_IS),
                                   "B Value of diffusion weighting", 1, 1, 0,true,
                                   "dicomtonrrd");
}
