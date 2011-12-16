#ifndef __DicomFileConverter_h
#define  __DicomFileConverter_h

class DicomFileConverterFactory
{
public:
  DicomFileConverterFactory();
  DicomFileConverter *GetSpecificFileProcessor(const std::string &vendor,
                                               const std::string &modality);
private:
};

#endif // __DicomFileConverter_h

