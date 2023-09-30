/*******

 ChatDBG extension for WinDBG.

 https://github.com/plasma-umass/ChatDBG
 @author Emery Berger <https://emeryberger.com>

 * build with `build-chatdbg.bat`
 * load into WinDBGX:
     View -> Command browser
       type ".load chatdbg.dll"
 * after running code and hitting an exception / signal:
     type "!why" in Command browser

 ******/

#include "openai.hpp"
#include <nlohmann/json.hpp>

#include <format>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>

#include <assert.h>
#include <dbgeng.h>
#include <iostream>

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include "appendlines.hpp"
#include "wordwrap.hpp"
#include "getmodel.hpp"

const auto MaxStackFrames =
    20; // maximum number of stack frames to use for a stack trace
const auto MaxNameLength = 1024;
const auto MaxTypeLength = 1024;
const auto MaxValueLength = 1024;

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

  if (g_client->QueryInterface(__uuidof(IDebugSymbols3),
                               (void **)&g_symbols3) != S_OK)
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

void outputGeneric(int outputType, std::vector<std::string>& output) {
  int hr;
    IDebugSymbolGroup *symbolGroup = nullptr;
    if (FAILED(hr = g_symbols->GetScopeSymbolGroup(outputType,
                                                   nullptr, &symbolGroup)))
      return;

    IDebugSymbolGroup2 *symbolGroup2 = nullptr;
    if (FAILED(hr = g_symbols3->GetScopeSymbolGroup2(outputType, // DEBUG_SCOPE_GROUP_ARGUMENTS
                                                     nullptr, &symbolGroup2)))
      return;

    ULONG numSymbols = 0;
    if (FAILED(hr = symbolGroup->GetNumberSymbols(&numSymbols)))
      return;

    std::vector<std::string> symbol_output;

    for (ULONG i = 0; i < numSymbols; i++) {
      char name[MaxNameLength] = {};
      ULONG nameSize;
      if (FAILED(hr = symbolGroup->GetSymbolName(i, name, sizeof(name),
                                                 &nameSize)))
        continue;

      ULONG typeId;
      ULONG64 module;
      if (FAILED(hr = g_symbols->GetSymbolTypeId(name, &typeId, &module)))
        continue;

      char typeName[MaxTypeLength] = {};
      ULONG typeNameSize;
      if (FAILED(hr = g_symbols->GetTypeName(module, typeId, typeName,
                                             sizeof(typeName), &typeNameSize)))
        continue;

      char varValue[MaxValueLength] = {};
      ULONG varValueNameSize;
      if (FAILED(hr = symbolGroup2->GetSymbolValueText(
                     i, varValue, MaxValueLength, &varValueNameSize)))
        continue;

      switch(outputType) {
      case DEBUG_SCOPE_GROUP_ARGUMENTS:
	symbol_output.push_back(std::format("{} = {}", name, varValue));
	break;
      case DEBUG_SCOPE_GROUP_LOCALS:
	symbol_output.push_back(std::format("{} {} = {}", typeName, name, varValue));
	break;
      default:
	assert(0);
      }
    }
    if (numSymbols > 0) {
      ///      output.push_back(preface); // std::string("Arguments: "));
      for (auto &symbol_out : symbol_output) {
        output.push_back(symbol_out);
      }
    }

    // Release the symbolGroup for this frame.
    if (symbolGroup)
      symbolGroup->Release();
}

void outputArguments(std::vector<std::string>& output) {
  std::vector<std::string> args;
  outputGeneric(DEBUG_SCOPE_GROUP_ARGUMENTS, args);
  std::string outString;
  for (const auto& a : args) {
    if (!outString.empty()) {
      outString += ",";
    }
    outString += a;
  }
  output.push_back(outString);
}

void outputLocals(std::vector<std::string>& output) {
  std::vector<std::string> locals;
  outputGeneric(DEBUG_SCOPE_GROUP_LOCALS, locals);
  if (!locals.empty()) {
    output.push_back("Local variables: ");
    for (const auto& l : locals) {
      output.push_back(l);
    }
  }
}

std::string call_openai_api(const std::string &user_prompt,
			    const std::string &key) {
    openai::start();

    auto completion = openai::completion().create(R"(
    {
        "model": get_model(),
        "messages": [{"role":"user", "content", user_prompt}],
    }
    )"_json);

    return std::string{ completion.dump(2) };
  

#if 0
    if (res && res->status == 200) {
        auto response_json = nlohmann::json::parse(res->body);
        std::string text = response_json["choices"][0]["message"]["content"];
        return word_wrap(text);
    } else {
        std::string error_msg = "Error occurred: ";
        if(res)
            error_msg += res->status == 0 ? "Unable to connect to the server." : std::to_string(res->status) + " - " + res->body;
        else
            error_msg += "Unable to receive a response.";
        return error_msg;
    }
#endif
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

  std::vector<std::string> output;
  // output.push_back(std::format("Hello {}", nameBuffer));

  auto framesOutput = 0;

  // Get up to MaxStackFrames stack frames from the current call stack
  if FAILED (g_control->GetStackTrace(0, 0, 0, stackFrames, MaxStackFrames,
                                      &framesFilled))
    return S_OK;

  for (ULONG i = 0; i < framesFilled; i++) {
    ULONG64 offset;
    char functionName[MAX_PATH] = {};
    char fileName[MAX_PATH] = {};
    ULONG lineNo;

    if FAILED (g_symbols->GetNameByOffset(stackFrames[i].InstructionOffset,
                                          functionName, MAX_PATH, NULL,
                                          &offset))
      continue;

    if FAILED (g_symbols->GetLineByOffset(stackFrames[i].InstructionOffset,
                                          &lineNo, fileName, MAX_PATH, NULL,
                                          NULL))
      continue;

    int hr;
    if (FAILED(hr = g_symbols->SetScope(stackFrames[i].InstructionOffset,
                                        &stackFrames[i], NULL, 0)))
      continue;

    framesOutput += 1;

    std::vector<std::string> args;
    outputArguments(args);
    
    output.push_back(std::format("frame {}: {}({}) at {}:{}", framesOutput,
                                 functionName, args.back(), fileName, lineNo));
    // outputArguments(output);
    outputLocals(output);

    auto startLine = lineNo < 10 ? 1 : lineNo - 10;
    output.push_back(std::format("/* frame {} in {} (lines {} to {}) */", framesOutput, fileName, startLine, lineNo));
    append_lines(output, std::string(fileName), startLine, lineNo);
    
    hasDebugSymbols = true;
  }
  g_control->Output(DEBUG_OUTPUT_NORMAL, get_model().c_str());
  g_control->Output(DEBUG_OUTPUT_NORMAL, "\n");
  for (auto &s : output) {
    g_control->Output(DEBUG_OUTPUT_NORMAL, s.c_str());
    g_control->Output(DEBUG_OUTPUT_NORMAL, "\n");
  }

  std::string prompt { "What is the capitol of France?" };
  auto openai_key_str = std::getenv("OPENAI_API_KEY");
  
  if (!openai_key_str) {
    g_control->Output(DEBUG_OUTPUT_WARNING, "You need to define the environment variable OPENAI_API_KEY.\n");
    return S_OK;
  }
  auto openai_key = std::string(openai_key_str);
  
  g_control->Output(DEBUG_OUTPUT_NORMAL, std::string(openai_key).c_str());
  auto result = call_openai_api(prompt, openai_key);
  g_control->Output(DEBUG_OUTPUT_NORMAL, result.c_str());
  
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
