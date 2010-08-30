// #include <common_util/getopt.h>
// 
// #include <platform/assert.h>
// #include <platform/string.h>
// #include <platform/memory.h>

#include "getopt.h"

#include "../dl_types.h"
#include "../dl_temp.h"

#include <ctype.h>
#include <string.h>

EGetOptError GetOptCreateContext(SGetOptContext* _Ctx, int argc, char** argv, const SOption* _Opts)
{
	_Ctx->argc = argc - 1; // stripping away file-name!
	_Ctx->argv = argv + 1; // stripping away file-name!
	_Ctx->m_Opts = _Opts;
	_Ctx->m_CurrentIndex = 0;
	_Ctx->m_CurrentOptArg = 0x0;

	// count opts
	_Ctx->m_NumOpts = 0;
	SOption cmp_opt = { 0 };
	const SOption* opt = _Opts;
	while(memcmp(opt, &cmp_opt, sizeof(SOption)) != 0)
	{
		if(opt->m_Val == '!' || opt->m_Val == '?' || opt->m_Val == '+' || opt->m_Val == 0 || opt->m_Val == -1)
			return E_GETOPT_ERROR_INVALID_PARAMETER;

		_Ctx->m_NumOpts++; opt++;
	}

	return E_GETOPT_ERROR_OK;
}

int GetOpt(SGetOptContext* _Ctx)
{
	// are all options processed?
	if(_Ctx->m_CurrentIndex == (unsigned int)_Ctx->argc)
		return -1;

	// reset opt-arg
	_Ctx->m_CurrentOptArg = 0x0;

	const char* curr_token = _Ctx->argv[_Ctx->m_CurrentIndex];
	
	// this token has been processed!
	_Ctx->m_CurrentIndex++;

	// check if item is no option
	if(curr_token[0] && curr_token[0] != '-')
	{
		_Ctx->m_CurrentOptArg = curr_token;
		return '+'; // return '+' as identifier for no option!
	}
	
	const SOption* found_opt = 0x0;
	const char* found_arg = 0x0;

	if(curr_token[1] != '\0' && curr_token[1] != '-' && curr_token[2] == '\0') // short opt
	{
		for(unsigned int i = 0; i < _Ctx->m_NumOpts; i++)
		{
			const SOption* opt = _Ctx->m_Opts + i;

			if(opt->m_NameShort == curr_token[1])
			{
				found_opt = opt;

				// if there is an value when: - current_index < argc and value in argv[current_index] do not start with '-'
				bool has_value = (_Ctx->m_CurrentIndex != (unsigned int)_Ctx->argc) && (_Ctx->argv[_Ctx->m_CurrentIndex][0] != '-');

				if(has_value && (opt->m_Type == E_OPTION_TYPE_OPTIONAL || opt->m_Type == E_OPTION_TYPE_REQUIRED))
				{
					found_arg = _Ctx->argv[_Ctx->m_CurrentIndex];
					_Ctx->m_CurrentIndex++; // next token has been processed aswell!
				}
				break;
			}
		}
	}
	else if(curr_token[1] == '-' && curr_token[2] != '\0') // long opt
	{
		const char* check_option = curr_token + 2;

		for(unsigned int i = 0; i < _Ctx->m_NumOpts; i++)
		{
			const SOption* opt = _Ctx->m_Opts + i;

			unsigned int name_len = strlen(opt->m_Name);

			if(StrCaseCompareLen(opt->m_Name, check_option, name_len) == 0)
			{
				check_option += name_len;
				found_arg = *check_option == '=' ? check_option + 1 : 0x0;
				found_opt = opt;
				break;
			}
		}
	}
	else // malformed opt "-", "-xyz" or "--"
	{
		_Ctx->m_CurrentOptArg = curr_token;
		return '!';
	}

	if(found_opt == 0x0) // found no matching option!
	{
		_Ctx->m_CurrentOptArg = curr_token;
		return '?';
	}

	if(found_arg != 0x0)
	{
		switch(found_opt->m_Type)
		{
			case E_OPTION_TYPE_FLAG_SET:
			case E_OPTION_TYPE_FLAG_AND:
			case E_OPTION_TYPE_FLAG_OR:
			case E_OPTION_TYPE_NO_ARG:
				// these types should have no argument, usage error!
				_Ctx->m_CurrentOptArg = found_opt->m_Name;
				return '!';

			case E_OPTION_TYPE_OPTIONAL:
			case E_OPTION_TYPE_REQUIRED:
				_Ctx->m_CurrentOptArg = found_arg;
				return found_opt->m_Val;
		}
	}
	else // no argument found
	{
		switch(found_opt->m_Type)
		{
			case E_OPTION_TYPE_FLAG_SET: *found_opt->m_Flag  = found_opt->m_Val; return 0;
			case E_OPTION_TYPE_FLAG_AND: *found_opt->m_Flag &= found_opt->m_Val; return 0;
			case E_OPTION_TYPE_FLAG_OR:  *found_opt->m_Flag |= found_opt->m_Val; return 0; // zero is "a flag was set!"
			
			case E_OPTION_TYPE_NO_ARG:
			case E_OPTION_TYPE_OPTIONAL:
				return found_opt->m_Val;

			case E_OPTION_TYPE_REQUIRED: // the option requires an argument! (--option=arg, -o arg)
					_Ctx->m_CurrentOptArg = found_opt->m_Name;
					return '!';
		}
	}

 	M_ASSERT(false && "This should not happen!");
 	return -1;
}

const char* GetOptCreateHelpString(SGetOptContext* _Ctx, char* _pBuffer, unsigned int BufferSize)
{
	int BufferPos = 0;
	for(uint32 iOpt = 0; iOpt < _Ctx->m_NumOpts; ++iOpt)
	{
		const SOption& Opt = _Ctx->m_Opts[iOpt];

		char LongName[64];
		int Outpos = StrFormat(LongName, 64, "--%s", Opt.m_Name);

		if(Opt.m_Type == E_OPTION_TYPE_REQUIRED)
			StrFormat(LongName + Outpos, 64 - Outpos, "=<%s>", Opt.m_ValueDesc);

		if(Opt.m_NameShort == 0x0)
			BufferPos += StrFormat(_pBuffer + BufferPos, BufferSize - BufferPos, "   %-32s - %s\n", LongName, Opt.m_Desc);
		else
			BufferPos += StrFormat(_pBuffer + BufferPos, BufferSize - BufferPos, "-%c %-32s - %s\n", Opt.m_NameShort, LongName, Opt.m_Desc);
	}

	return _pBuffer;
}
