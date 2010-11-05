/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include <dl/dl_util.h>
#include <dl/dl_txt.h>

#include <stdlib.h>
#include <stdio.h>

// TODO: Add function to load instance form file without knowing if it is text or binary?

EDLError dl_util_load_instance_from_file(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void** _ppInstance)
{
	(void)_Ctx; (void)_pFileName; (void)_DLType; (void)_ppInstance;
	return DL_ERROR_INTERNAL_ERROR;
}

EDLError dl_util_load_instance_from_text_file(HDLContext _Ctx, const char* _pFileName, void** _ppInstance)
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
	EDLError Err = dl_required_text_pack_size(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		return Err;
	}

	unsigned char* pPackedData = (unsigned char*)malloc(PackedSize);

	Err = dl_pack_text(_Ctx, (char*)pData, pPackedData, PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		free(pPackedData);
		return Err;
	}

	Err = dl_load_instance_inplace(_Ctx, pPackedData, pPackedData, PackedSize);
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

EDLError dl_util_load_instance_from_text_file_inplace(HDLContext _Ctx, const char* _pFileName, void* _pInstance, unsigned int _InstanceSize)
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
	EDLError Err = dl_required_text_pack_size(_Ctx, (char*)pData, &PackedSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		return Err;
	}

	unsigned char* pPackedData = (unsigned char*)malloc(PackedSize);

	Err = dl_pack_text(_Ctx, (char*)pData, pPackedData, _InstanceSize);
	if(Err != DL_ERROR_OK)
	{
		free(pData);
		free(pPackedData);
		return Err;
	}

	Err = dl_load_instance_inplace(_Ctx, (unsigned char*)_pInstance, pPackedData, PackedSize);

	free(pPackedData);
	free(pData);

	return Err;
}

EDLError dl_util_store_instance_to_file(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance, DLECpuEndian _OutEndian, unsigned int _OutPtrSize)
{
	unsigned int PackSize = 0;

	EDLError DLErr = dl_instace_size_stored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned char* pOutData = new unsigned char[PackSize];

	DLErr = dl_store_instace(_Ctx, _DLType, _pInstance, pOutData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned int ConvertSize = 0;
	DLErr = dl_instance_size_converted(_Ctx, pOutData, PackSize, _OutPtrSize, &ConvertSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	if(ConvertSize <= PackSize)
	{
		// we can do inplace convert!
		DLErr = dl_convert_instance_inplace(_Ctx, pOutData, PackSize, _OutEndian, _OutPtrSize);
		if(DLErr != DL_ERROR_OK) return DLErr;
	}
	else
	{
		// the data will grow on convert, need to alloc more space!
		unsigned char* pConvertedData = new unsigned char[ConvertSize];

		DLErr = dl_convert_instance(_Ctx, pOutData, PackSize, pConvertedData, ConvertSize, _OutEndian, _OutPtrSize);
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

EDLError dl_util_store_instance_to_text_file(HDLContext _Ctx, const char* _pFileName, StrHash _DLType, void* _pInstance)
{
	unsigned int PackSize = 0;

	EDLError DLErr = dl_instace_size_stored(_Ctx, _DLType, _pInstance, &PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned char* pPackedData = new unsigned char[PackSize];

	DLErr = dl_store_instace(_Ctx, _DLType, _pInstance, pPackedData, PackSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	unsigned int TxtSize = 0;
	DLErr = dl_required_unpack_size(_Ctx, pPackedData, PackSize, &TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	char* pOutData = new char[PackSize];
	DLErr = dl_unpack(_Ctx, pPackedData, PackSize, pOutData, TxtSize);
	if(DLErr != DL_ERROR_OK) return DLErr;

	FILE* OutFile = fopen(_pFileName, "wb");
	if(OutFile == 0x0) return DL_ERROR_UTIL_FILE_NOT_FOUND;

	fwrite(pPackedData, sizeof(unsigned char), PackSize, OutFile);
	fclose(OutFile);

	delete[] pOutData;
	delete[] pPackedData;

	return DL_ERROR_OK;
}
