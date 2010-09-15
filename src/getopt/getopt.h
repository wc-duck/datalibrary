/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef COMMON_UTIL_GETOPT_H_INCLUDED
#define COMMON_UTIL_GETOPT_H_INCLUDED

/*
	File: getopt.h
		Sumary:
			Provides functionality to parse standard argc/argv in an easy mannor.

	Example:
		Example from xbexec.exe

		(start code)
		static const SOption option_list[] = 
		{
			{"help",   'h', E_OPTION_TYPE_NO_ARG,   0x0, 'h', "displays this help-message"},
			{"copy",   'c', E_OPTION_TYPE_REQUIRED, 0x0, 'c', "copy this xex to the xbox before executing"},
			{"target", 't', E_OPTION_TYPE_REQUIRED, 0x0, 't', "ip or name of development-kit to run on"},
			{"cmdline",  0, E_OPTION_TYPE_REQUIRED, 0x0, 'l', "command-line to send to xex when run"},
			{0} // end option_list
		};

		SGetOptContext go_ctx;
		CreateGetOptContext(&go_ctx, argc, argv, option_list);

		// TODO: read these from argc/argv!
		char* xex_to_run   = "";
		char* xex_to_copy  = "";
		char* xex_cmd_line = "";
		char* kit_to_use   = "";

		int32 opt;
		while((opt = GetOpt(&go_ctx)) != -1)
		{
			switch(opt)
			{
				case 'h': PrintHelp(); return 0;
				case 'c': xex_to_copy  = (char*)go_ctx.m_CurrentOptArg; break;
				case 't': kit_to_use   = (char*)go_ctx.m_CurrentOptArg; break;
				case 'l': xex_cmd_line = (char*)go_ctx.m_CurrentOptArg; break;
				case '!': 
					printf("error: incorrect usage of flag \"%s\"!\n\n", go_ctx.m_CurrentOptArg);
					PrintHelp();
					return 1;
				case '?':
					printf("error: unrecognized flag \"%s\"!\n\n", go_ctx.m_CurrentOptArg);
					PrintHelp();
					return 1;
				case '+':
					if(xex_to_run[0] != '\0')
					{
						printf("error: main xex already set to: \"%s\", trying to set it to \"%s\"\n", xex_to_run, go_ctx.m_CurrentOptArg);
						PrintHelp();
						return 1;
					}
					xex_to_run = (char*)go_ctx.m_CurrentOptArg;
				default:
					ASSERT(false && "This should not happen!");
			}
		}
		(end)
*/

/*
	Enum: EGetOptError
		Error codes related to GetOpt functionality.

		E_GETOPT_ERROR_OK                - All ok
		E_GETOPT_ERROR_INVALID_PARAMETER - A parameter was invalid, m_Val equals !, ?, +, 0 or -1 for example.
*/
enum EGetOptError
{
	E_GETOPT_ERROR_OK,
	E_GETOPT_ERROR_INVALID_PARAMETER
};

/*
	Enum: EOptionType
		Types of supported options by system.

		E_OPTION_TYPE_NO_ARG   - The option can have no argument
		E_OPTION_TYPE_REQUIRED - The option requires an argument (--option=arg, -o arg)
		E_OPTION_TYPE_OPTIONAL - The option-argument is optional

		E_OPTION_TYPE_FLAG_SET - The option is a flag and value will be set to flag
		E_OPTION_TYPE_FLAG_AND - The option is a flag and value will be and:ed with flag
		E_OPTION_TYPE_FLAG_OR  - The option is a flag and value will be or:ed with flag
*/
enum EOptionType
{
	E_OPTION_TYPE_NO_ARG,
	E_OPTION_TYPE_REQUIRED,
	E_OPTION_TYPE_OPTIONAL,
	E_OPTION_TYPE_FLAG_SET,
	E_OPTION_TYPE_FLAG_AND,
	E_OPTION_TYPE_FLAG_OR
};

/*
	Struct: SOption
		Option in system.

	Members:
		m_Name      - Long name of argument, set to NULL if only short name is valid.
		m_NameShort - Short name of argument, set to 0 if only long name is valid.
		m_Type      - Type of option, see <EOptionType>.
		m_Flag      - Ptr to flag to set if option is of flag-type, set to null NULL if option is not of flag-type.
		m_Val       - If option is of flag-type, this value will be set/and:ed/or:ed to the flag, else it will be returnde from GetOpt when option is found.
		m_Desc      - Description of option.
		m_ValueDesc - Short description of valid values to the option, will only be used when generating help-text. example: "--my_option=<value_desc_goes_here>"
*/
struct SOption
{
	const char* m_Name;
	int         m_NameShort;
	EOptionType m_Type;
	int*        m_Flag;
	int         m_Val;
	const char* m_Desc;
	const char* m_ValueDesc;
};

/*
	Struct: SGetOptContext
		Context used while parsing options.
		Need to be initialized by <CreateGetOptContext> before usage. If reused a reinitialization by <CreateGetOptContext> is needed.
		Do not modify data in this struct manually!

	Members:
		argc            - Internal variable
		argv            - Internal variable
		m_Opts          - Internal variable
		m_NumOpts       - Internal variable
		m_CurrentIndex  - Internal variable
		m_CurrentOptArg - Used to return values. See <GetOpt>
*/
struct SGetOptContext
{
	int             argc;
	char**          argv;
	const SOption*  m_Opts;
	unsigned int    m_NumOpts;
	unsigned int    m_CurrentIndex;
	const char*     m_CurrentOptArg;
};

/*
	Function: GetOptCreateContext
		Initializes an SGetOptContext-struct to be used by <GetOpt>

	Arguments:
		_Ctx  - Ptr to context to initialize.
		argc  - argc from "int main(int argc, char** argv)" or equal.
		argv  - argv from "int main(int argc, char** argv)" or equal. Data need to be valid during option-parsing and usage of data.
		_Opts - Ptr to array with options that should be looked for. Should end with an option that is all zeroed!

	Returns:
		E_GETOPT_ERROR_OK on success, otherwise error-code.
*/
EGetOptError GetOptCreateContext(SGetOptContext* _Ctx, int argc, char** argv, const SOption* _Opts);

/*
	Function: GetOpt
		Used to parse argc/argv with the help of a SGetOptContext.
		Tries to parse the next token in ctx and return id depending on status.

	Arguments:
		_Ctx - Pointer to a initialized <SGetOptContext>

	Returns:
		- '!' on error. ctx->m_CurrentOptArg will be set to flag-name! Errors that can occur, Argument missing if argument is required or Argument found when there should be none.
		- '?' if item was an unreqognized option, ctx->m_CurrentOptArg will be set to item!
		- '+' if item was no option, ctx->m_CurrentOptArg will be set to item!
		- '0' if the opt was a flag and it was set. ctx->m_CurrentOptArg will be set to flag-name!
		- the value stored in m_Val in the found option.
		- -1 no more options to parse!
*/
int GetOpt(SGetOptContext* _Ctx);

/*
	Function: GetOptCreateHelpString
		Builds a string that describes all options for use with the --help-flag etc.

	Arguments:
		_Ctx       - Pointer to a initialized <SGetOptContext>
		_pBuffer   - Pointer to buffer to build string in.
		BufferSize - Size of _pBuffer.

	Returns:
		_pBuffer filled with a help-string.
*/
const char* GetOptCreateHelpString(SGetOptContext* _Ctx, char* _pBuffer, unsigned int BufferSize);

#endif // COMMON_UTIL_GETOPT_H_INCLUDED
