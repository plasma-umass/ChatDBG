/*******

 ChatDBG extension for WinDBG.

 https://github.com/plasma-umass/ChatDBG
 @author Emery Berger <https://emeryberger.com>

 ******/

#include <dbgeng.h>
#include <iostream>
#include <windows.h>

const auto MaxStackFrames =
    20; // maximum number of stack frames to use for a stack trace
const auto MaxNameLength = 1024;

// Declare the IDebugClient interface
extern "C" HRESULT DebugCreate(_In_ REFIID InterfaceId, _Out_ PVOID *Interface);

IDebugClient *g_client;
IDebugControl *g_control;
IDebugSymbols *g_symbols;
IDebugSymbols3 *g_symbols3;

extern "C" __declspec(dllexport) HRESULT CALLBACK
    DebugExtensionInitialize(PULONG Version, PULONG Flags) {
  *Version = DEBUG_EXTENSION_VERSION(1, 0);
  *Flags = 0;

  // Obtain the client interface
  if (DebugCreate(__uuidof(IDebugClient), (void **)&g_client) != S_OK)
    return E_FAIL;

  if (g_client->QueryInterface(__uuidof(IDebugControl), (void **)&g_control) !=
      S_OK)
    return E_FAIL;

  if (g_client->QueryInterface(__uuidof(IDebugSymbols), (void **)&g_symbols) !=
      S_OK)
    return E_FAIL;
  
  if (g_client->QueryInterface(__uuidof(IDebugSymbols3), (void **)&g_symbols3) !=
      S_OK)
    return E_FAIL;
  
  return S_OK;
}

extern "C" __declspec(dllexport) void CALLBACK
    DebugExtensionUninitialize(void) {
  if (g_control) {
    g_control->Output(DEBUG_OUTPUT_NORMAL, "goodbye cruel world\n");
  }

  if (g_symbols)
    g_symbols->Release();
  if (g_control)
    g_control->Release();
  if (g_client)
    g_client->Release();
}

extern "C" __declspec(dllexport) HRESULT CALLBACK
    why(PDEBUG_CLIENT4 Client, PCSTR Args) {
  // Get and print the module name
  char nameBuffer[MAX_PATH] = {};
  ULONG64 moduleBase = 0;
  bool hasDebugSymbols = false;

  // Get the base address of the first loaded module
  if (g_symbols->GetModuleByIndex(0, &moduleBase) == S_OK) {
    ULONG nameSize;
    // Get the base name of the module
    if (g_symbols->GetModuleNames(DEBUG_ANY_ID, moduleBase, NULL, 0, NULL,
                                  nameBuffer, MAX_PATH, &nameSize, NULL, 0,
                                  NULL) == S_OK) {
      g_control->Output(DEBUG_OUTPUT_NORMAL, "hello world %s\n", nameBuffer);
    }
  }

  // If unable to get module name, print unknown
  if (strlen(nameBuffer) == 0) {
    g_control->Output(DEBUG_OUTPUT_NORMAL, "hello world unknown\n");
    return S_OK;
  }

  DEBUG_STACK_FRAME stackFrames[MaxStackFrames] = {};
  ULONG framesFilled;

  // Get up to MaxStackFrames stack frames from the current call stack
  if (g_control->GetStackTrace(0, 0, 0, stackFrames, MaxStackFrames,
                               &framesFilled) == S_OK) {
    
    for (ULONG i = 0; i < framesFilled; i++) {
      ULONG64 offset;
      char functionName[MAX_PATH] = {};
      char fileName[MAX_PATH] = {};
      ULONG lineNo;

      if (g_symbols->GetNameByOffset(stackFrames[i].InstructionOffset,
                                     functionName, MAX_PATH, NULL,
                                     &offset) == S_OK) {

        if (g_symbols->GetLineByOffset(stackFrames[i].InstructionOffset,
                                       &lineNo, fileName, MAX_PATH, NULL,
                                       NULL) == S_OK) {
          g_control->Output(DEBUG_OUTPUT_NORMAL, "Function: %s\n",
                            functionName);
          g_control->Output(DEBUG_OUTPUT_NORMAL, "File: %s, Line: %d\n",
                            fileName, lineNo);
	  int hr;
	  if (FAILED(hr = g_symbols->SetScope(stackFrames[i].InstructionOffset, &stackFrames[i], NULL, 0))) {
	  }
	  IDebugSymbolGroup* symbolGroup = nullptr;
	  if (FAILED(hr = g_symbols->GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_LOCALS, nullptr, &symbolGroup))) {
	    g_control->Output(DEBUG_OUTPUT_NORMAL, "no symbol group\n");
	  }
	  IDebugSymbolGroup2* symbolGroup2 = nullptr;
	  if (FAILED(hr = g_symbols3->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, nullptr, &symbolGroup2))) {
	    g_control->Output(DEBUG_OUTPUT_NORMAL, "no symbol group2\n");
	  }
	  ULONG numSymbols = 100;
	  if (FAILED(hr = symbolGroup->GetNumberSymbols(&numSymbols))) {
	  }
          g_control->Output(DEBUG_OUTPUT_NORMAL, "Num Symbols: %lu\n",
                            numSymbols);
	  if (numSymbols > 0) {

	    for (ULONG i = 0; i < numSymbols; i++)
	      {
		char name[MaxNameLength] = {};
		ULONG nameSize;
		if (FAILED(hr = symbolGroup->GetSymbolName(i, name, sizeof(name), &nameSize)))
		  continue;
		
		ULONG typeId;
		ULONG64 module;
		if (SUCCEEDED(hr = g_symbols->GetSymbolTypeId(name, &typeId, &module)))
		  {
		    char typeName[MaxNameLength] = {};
		    ULONG typeNameSize;
		    if (SUCCEEDED(hr = g_symbols->GetTypeName(module, typeId, typeName, sizeof(typeName), &typeNameSize)))
		      g_control->Output(DEBUG_OUTPUT_NORMAL, "Symbol Name: %s, Type: %s\n", name, typeName);
		    char varValue[MaxNameLength] = {};
		    ULONG varValueNameSize;
		    if (SUCCEEDED(hr = symbolGroup2->GetSymbolValueText(i, varValue, MaxNameLength, &varValueNameSize))) {
		      g_control->Output(DEBUG_OUTPUT_NORMAL, "Value: %s\n", varValue);
		    }
		  }
	      }
	    // Release the symbolGroup for this frame.
	    if (symbolGroup) symbolGroup->Release();
	    
	  }
          hasDebugSymbols = true;
        } else {
          // g_control->Output(DEBUG_OUTPUT_NORMAL, "File: Unknown, Line:
          // Unknown\n");
        }
      } else {
        // g_control->Output(DEBUG_OUTPUT_NORMAL, "Function: Unknown\n");
      }
    }
  }
  if (!hasDebugSymbols) {
    g_control->Output(DEBUG_OUTPUT_NORMAL,
                      "ChatDBG needs debug information to work properly. "
                      "Recompile your code with the /Zi flag.\n");
  }

  return S_OK;
}

extern "C" __declspec(dllexport) HRESULT CALLBACK
    why2(PDEBUG_CLIENT4 Client, PCSTR Args) {
  IDebugControl *localControl = nullptr;
  HRESULT hr =
      Client->QueryInterface(__uuidof(IDebugControl), (void **)&localControl);
  if (FAILED(hr) || localControl == nullptr) {
    OutputDebugString("QueryInterface for IDebugControl in why failed\n");
    return E_UNEXPECTED;
  }

  if (g_control == nullptr || g_symbols == nullptr)
    return E_UNEXPECTED;

  char nameBuffer[MAX_PATH] = {};
  ULONG64 moduleBase = 0;

  // Get the base address of the first loaded module
  if (g_symbols->GetModuleByIndex(0, &moduleBase) == S_OK) {
    ULONG nameSize;
    // Get the base name of the module
    if (g_symbols->GetModuleNames(DEBUG_ANY_ID, moduleBase, NULL, 0, NULL,
                                  nameBuffer, MAX_PATH, &nameSize, NULL, 0,
                                  NULL) == S_OK) {
      g_control->Output(DEBUG_OUTPUT_NORMAL, "hello world %s\n", nameBuffer);
      return S_OK;
    }
  }

  // If unable to get module name, print unknown
  g_control->Output(DEBUG_OUTPUT_NORMAL, "hello world unknown\n");
  return S_OK;
}
