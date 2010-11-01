/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>

#include <stdlib.h>
#include <stdio.h>

// TODO: Add function to load instance form file without knowing if it is text or binary?

EDLError DLUtilLoadInstanceFromFile(HDLContext _Ctx, const char* _pFileName, uint32 _DLType, void** _ppInstance)
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
	uint64 Size = ftell(File);
	fseek(File, 0, SEEK_SET);

	uint8* pData = (uint8*)malloc(pint(Size) + 1);
	size_t ReadBytes = fread(pData, sizeof(uint8), Size, File);
	pData[Size] = '\0';
	M_ASSERT(ReadBytes == Size);
	fclose(File);

	pint PackedSize = 0;
	EDLError Err = DLRequiredTextPackSize(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK) 
	{
		free(pData);
		return Err;
	}

	uint8* pPackedData = (uint8*)malloc(PackedSize);

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

EDLError DLUtilLoadInstanceFromTextFileInplace(HDLContext _Ctx, const char* _pFileName, void* _pInstance, pint _InstanceSize)
{
	FILE* File = fopen(_pFileName, "rb");
	if(File == 0x0)
		return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fseek(File, 0, SEEK_END);
	uint64 Size = ftell(File);
	fseek(File, 0, SEEK_SET);
	
	uint8* pData = (uint8*)malloc(pint(Size) + 1);
	pint ReadBytes = fread(pData, sizeof(uint8), Size, File);
	pData[Size] = '\0';
	fclose(File);
	M_ASSERT(ReadBytes == Size);

	pint PackedSize = 0;
	EDLError Err = DLRequiredTextPackSize(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		return Err;
	}

	uint8* pPackedData = (uint8*)malloc(PackedSize);

	Err = DLPackText(_Ctx, (char*)pData, pPackedData, _InstanceSize);
	if(Err != DL_ERROR_OK) 
	{
		free(pData);
		free(pPackedData);
		return Err;
	}

	Err = DLLoadInstanceInplace(_Ctx, (uint8*)_pInstance, pPackedData, PackedSize);

	free(pPackedData);
	free(pData);

	return Err;
}

EDLError DLUtilStoreInstanceToFile(HDLContext _Ctx, const char* _pFileName, uint32 _DLType, void* _pInstance, ECpuEndian _OutEndian, pint _OutPtrSize)
{
	pint PackSize = 0;

	EDLError DLErr = DLInstaceSizeStored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	uint8* pOutData = new uint8[PackSize];

	DLErr = DLStoreInstace(_Ctx, _DLType, _pInstance, pOutData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	pint ConvertSize = 0;
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
		uint8* pConvertedData = new uint8[ConvertSize];

		DLErr = DLConvertInstance(_Ctx, pOutData, PackSize, pConvertedData, ConvertSize, _OutEndian, _OutPtrSize);
		if(DLErr != DL_ERROR_OK) return DLErr;

		delete[] pOutData;
		pOutData = pConvertedData;
		PackSize = ConvertSize;
	}

	FILE* OutFile = fopen(_pFileName, "wb");
	if(OutFile == 0x0) return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fwrite(pOutData, sizeof(uint8), PackSize, OutFile);
	fclose(OutFile);

	delete[] pOutData;
	return DL_ERROR_OK;
}

EDLError DLUtilStoreInstanceToTextFile(HDLContext _Ctx, const char* _pFileName, uint32 _DLType, void* _pInstance)
{
	pint PackSize = 0;

	EDLError DLErr = DLInstaceSizeStored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	uint8* pPackedData = new uint8[PackSize];

	DLErr = DLStoreInstace(_Ctx, _DLType, _pInstance, pPackedData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	pint TxtSize = 0;
	DLErr = DLRequiredUnpackSize(_Ctx, pPackedData, PackSize, &TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	char* pOutData = new char[PackSize];
	DLErr = DLUnpack(_Ctx, pPackedData, PackSize, pOutData, TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	FILE* OutFile = fopen(_pFileName, "wb");
	if(OutFile == 0x0) return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fwrite(pPackedData, sizeof(uint8), PackSize, OutFile);
	fclose(OutFile);

	delete[] pOutData;
	delete[] pPackedData;

	return DL_ERROR_OK;
}
