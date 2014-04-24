#include "Tests.h"

/*
    Get explorer.exe path
*/
void TestRemoteCall()
{
    std::wcout << L"\r\nRemote function call test\n";
    std::wcout << L"Searching for explorer.exe... ";

    Process explorer;
    std::vector<DWORD> found;
    Process::EnumByName( L"explorer.exe", found );

    if (found.size() > 0)
    {
        std::wcout << L"Found. Attaching to process " << std::dec << found.front() << std::endl;

        if (explorer.Attach( found.front() ) != STATUS_SUCCESS)
        {
            std::wcout << L"Can't attach to process, status code " << LastNtStatus() << " aborting\n\n";
            return;
        }

        auto barrier = explorer.core().native()->GetWow64Barrier().type;

        if (barrier != wow_32_32 && barrier != wow_64_64)
        {
            std::wcout << L"Can't execute call through WOW64 barrier, aborting\n\n";
            return;
        }

        std::wcout << L"Searching for NtQueryVirtualMemory... ";

        auto hMainMod = explorer.modules().GetMainModule();
        auto pRemote = explorer.modules().GetExport(
            explorer.modules().GetModule( L"ntdll.dll" ), "NtQueryVirtualMemory" );

        if (hMainMod && pRemote.procAddress)
        {
            std::wcout << L"Found. Executing...\n";
            uint8_t buf[1024] = { 0 };

            RemoteFunction<fnNtQueryVirtualMemory> pFN( explorer, (fnNtQueryVirtualMemory)pRemote.procAddress,
                                                        INVALID_HANDLE_VALUE, (LPVOID)hMainMod->baseAddress,
                                                        MemorySectionName, buf, sizeof(buf), nullptr );

            decltype(pFN)::ReturnType result;

            pFN.setArg( 3, AsmVariant( buf, sizeof(buf) ) );
            pFN.Call( result );

            wchar_t* pStr = (wchar_t*)(buf + sizeof(UNICODE_STRING));

            std::wcout << L"Call result " << result << L" . Module path " << pStr << std::endl;
        }
        else
            std::wcout << L"Not found, aborting\n";
    }
    else
        std::wcout << L"Not found, aborting\n";

    std::wcout << std::endl;
}