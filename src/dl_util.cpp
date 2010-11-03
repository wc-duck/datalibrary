/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>

#include <stdlib.h>
#include <stdio.h>

// TODO: Add function to load instance form file without knowing if it is text or binary?

EDLError DLUtilLoadInstanceFromFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void** _ppInstance)
{
	DL_UNUSED(_Ctx); DL_UNUSED(_pFileName); DL_UNUSED(_DLType); DL_UNUSED(_ppInstance);
	return DL_ERROR_INTERNAL_ERROR;
}

EDLError DLUtilLoadInstanceFromTextFile(HDLContext _Ctx, const char* _pFileName, void** _ppInstance)
{
	FILE* File = fopen(_pFileName, "rb");
	if(File == 0x0)
		return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fseek(File, 0, SEEK_END);
	unsigned int Size = ftell(File);
	fseek(File, 0, SEEK_SET);

	unsigned char* pData = (unsigned char*)malloc((unsigned int)(Size) + 1);
	size_t ReadBytes = fread(pData, sizeof(unsigned char), Size, File);
	pData[Size] = '\0';
	M_ASSERT(ReadBytes == Size);
	fclose(File);

	unsigned int PackedSize = 0;
	EDLError Err = DLRequiredTextPackSize(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		return Err;
	}

	unsigned char* pPackedData = (unsigned char*)malloc(PackedSize);

	Err = DLPackText(_Ctx, (char*)pData, pPackedData, PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		free(pPackedData);
		return Err;
	}

	Err = DLLoadInstanceInplace(_Ctx, pPackedData, pPackedData, PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		free(pPackedData);
		*_ppInstance = 0x0;
		return Err;
	}

	*_ppInstance = pPackedData;
	free(pData);

	return DL_ERROR_OK;
}

EDLError DLUtilLoadInstanceFromTextFileInplace(HDLContext _Ctx, const char* _pFileName, void* _pInstance, unsigned int _InstanceSize)
{
	FILE* File = fopen(_pFileName, "rb");
	if(File == 0x0)
		return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fseek(File, 0, SEEK_END);
	unsigned int Size = ftell(File);
	fseek(File, 0, SEEK_SET);

	unsigned char* pData = (unsigned char*)malloc((unsigned int)(Size) + 1);
	unsigned int ReadBytes = fread(pData, sizeof(unsigned char), Size, File);
	pData[Size] = '\0';
	fclose(File);
	M_ASSERT(ReadBytes == Size);

	unsigned int PackedSize = 0;
	EDLError Err = DLRequiredTextPackSize(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		return Err;
	}

	unsigned char* pPackedData = (unsigned char*)malloc(PackedSize);

	Err = DLPackText(_Ctx, (char*)pData, pPackedData, _InstanceSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		free(pPackedData);
		return Err;
	}

	Err = DLLoadInstanceInplace(_Ctx, (unsigned char*)_pInstance, pPackedData, PackedSize);

	free(pPackedData);
	free(pData);

	return Err;
}

EDLError DLUtilStoreInstanceToFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance, ECpuEndian _OutEndian, unsigned int _OutPtrSize)
{
	unsigned int PackSize = 0;

	EDLError DLErr = DLInstaceSizeStored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned char* pOutData = new unsigned char[PackSize];

	DLErr = DLStoreInstace(_Ctx, _DLType, _pInstance, pOutData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned int ConvertSize = 0;
	DLErr = DLInstanceSizeConverted(_Ctx, pOutData, PackSize, _OutPtrSize, &ConvertSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	if(ConvertSize <= PackSize)
	{
		// we can do inplace convert!
		DLErr = DLConvertInstanceInplace(_Ctx, pOutData, PackSize, _OutEndian, _OutPtrSize);
		if(DLErr != DL_ERROR_OK) return DLErr;
	}
	else
	{
		// the data will grow on convert, need to alloc more space!
		unsigned char* pConvertedData = new unsigned char[ConvertSize];

		DLErr = DLConvertInstance(_Ctx, pOutData, PackSize, pConvertedData, ConvertSize, _OutEndian, _OutPtrSize);
		if(DLErr != DL_ERROR_OK) return DLErr;

		delete[] pOutData;
		pOutData = pConvertedData;
		PackSize = ConvertSize;
	}

	FILE* OutFile = fopen(_pFileName, "wb");
	if(OutFile == 0x0) return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fwrite(pOutData, sizeof(unsigned char), PackSize, OutFile);
	fclose(OutFile);

	delete[] pOutData;
	return DL_ERROR_OK;
}

EDLError DLUtilStoreInstanceToTextFile(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance)
{
	unsigned int PackSize = 0;

	EDLError DLErr = DLInstaceSizeStored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned char* pPackedData = new unsigned char[PackSize];

	DLErr = DLStoreInstace(_Ctx, _DLType, _pInstance, pPackedData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned int TxtSize = 0;
	DLErr = DLRequiredUnpackSize(_Ctx, pPackedData, PackSize, &TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	char* pOutData = new char[PackSize];
	DLErr = DLUnpack(_Ctx, pPackedData, PackSize, pOutData, TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	FILE* OutFile = fopen(_pFileName, "wb");
	if(OutFile == 0x0) return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fwrite(pPackedData, sizeof(unsigned char), PackSize, OutFile);
	fclose(OutFile);

	delete[] pOutData;
	delete[] pPackedData;

	return DL_ERROR_OK;
}
