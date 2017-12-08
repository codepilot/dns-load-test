#pragma once

class CommandLine {
public:
	std::vector<std::wstring> argVector;
	std::unordered_set<std::wstring> argSet;
	std::unordered_map<std::wstring, std::wstring> argMap;

	static std::vector<std::wstring> GetCommandLineArgs() {
		int nArgs;
		const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		std::vector<std::wstring> cmdLine(nArgs);
		for (decltype(cmdLine.size()) i{ 0 }; i < cmdLine.size(); i++) {
			cmdLine[i] = szArglist[i];
		}
		LocalFree(szArglist);
		return cmdLine;
	}

	auto ArgsToSet() {
		std::unordered_set<std::wstring> ret;
		int nArgs{ 0 };
		const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		std::vector<std::wstring> cmdLine(nArgs);
		for (decltype(cmdLine.size()) i{ 1 }; i < cmdLine.size(); i++) {
			std::wstring arg{ szArglist[i] };
			ret.emplace(arg);
		}
		return ret;
	}

	auto ArgsToMap() {
		std::unordered_map<std::wstring, std::wstring> ret;

		int nArgs{ 0 };
		const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		std::vector<std::wstring> cmdLine(nArgs);
		for (decltype(cmdLine.size()) i{ 1 }; i < cmdLine.size(); i++) {
			std::wstring arg{ szArglist[i] };
			const auto found{ arg.find_first_of(L"=") };
			if (std::wstring::npos == found) {
				continue;
			}
			ret.emplace(arg.substr(0, found), arg.substr(found + 1));
		}
		LocalFree(szArglist);

		return ret;
	}

	CommandLine() : argVector{ GetCommandLineArgs() }, argSet{ ArgsToSet() }, argMap{ ArgsToMap() }  {

	}

};