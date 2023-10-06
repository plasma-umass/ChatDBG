// The MIT License (MIT)
// 
// Copyright (c) 2023 Olrea, Florian Dang
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef OPENAI_HPP_
#define OPENAI_HPP_


#if OPENAI_VERBOSE_OUTPUT
#pragma message ("OPENAI_VERBOSE_OUTPUT is ON")
#endif

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <cstdlib>
#include <map>

#ifndef CURL_STATICLIB
#include <curl/curl.h>
#else 
#include "curl/curl.h"
#endif

#include <nlohmann/json.hpp>  // nlohmann/json

namespace openai {

namespace _detail {

// Json alias
using Json = nlohmann::json;

struct Response {
    std::string text;
    bool        is_error;
    std::string error_message;
};

// Simple curl Session inspired by CPR
class Session {
public:
    Session(bool throw_exception) : throw_exception_{throw_exception} {
        initCurl();
        ignoreSSL();
    }

    Session(bool throw_exception, std::string proxy_url) : throw_exception_{ throw_exception } {
        initCurl();
        ignoreSSL();
        setProxyUrl(proxy_url);
    }

    ~Session() { 
        curl_easy_cleanup(curl_); 
        curl_global_cleanup();
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
        }
    }

    void initCurl() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
        if (curl_ == nullptr) {
            throw std::runtime_error("curl cannot initialize"); // here we throw it shouldn't happen
        }
    }

    void ignoreSSL() {
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    }
 
    void setUrl(const std::string& url) { url_ = url; }

    void setToken(const std::string& token, const std::string& organization) {
        token_ = token;
        organization_ = organization;
    }
    void setProxyUrl(const std::string& url) {
        proxy_url_ = url; 
        curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_url_.c_str());
        
    }

    void setBody(const std::string& data);
    void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath, const std::map<std::string, std::string>& fields);
    
    Response getPrepare();
    Response postPrepare(const std::string& contentType = "");
    Response deletePrepare();
    Response makeRequest(const std::string& contentType = "");
    std::string easyEscape(const std::string& text);

private:
    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append((char*) ptr, size * nmemb);
        return size * nmemb;
    }

private:
    CURL*       curl_;
    CURLcode    res_;
    curl_mime   *mime_form_ = nullptr;
    std::string url_;
    std::string proxy_url_;
    std::string token_;
    std::string organization_;

    bool        throw_exception_;
    std::mutex  mutex_request_;
};

inline void Session::setBody(const std::string& data) { 
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    }
}

inline void Session::setMultiformPart(const std::pair<std::string, std::string>& fieldfield_and_filepath, const std::map<std::string, std::string>& fields) {
    // https://curl.se/libcurl/c/curl_mime_init.html
    if (curl_) {
        if (mime_form_ != nullptr) {
            curl_mime_free(mime_form_);
            mime_form_ = nullptr;
        }
        curl_mimepart *field = nullptr;

        mime_form_ = curl_mime_init(curl_);
    
        field = curl_mime_addpart(mime_form_);
        curl_mime_name(field, fieldfield_and_filepath.first.c_str());
        curl_mime_filedata(field, fieldfield_and_filepath.second.c_str());

        for (const auto &field_pair : fields) {
            field = curl_mime_addpart(mime_form_);
            curl_mime_name(field, field_pair.first.c_str());
            curl_mime_data(field, field_pair.second.c_str(), CURL_ZERO_TERMINATED);
        }
        
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, mime_form_);
    }
}

inline Response Session::getPrepare() {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_POST, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
    }
    return makeRequest();
}

inline Response Session::postPrepare(const std::string& contentType) {
    return makeRequest(contentType);
}

inline Response Session::deletePrepare() {
    if (curl_) {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    return makeRequest();
}

inline Response Session::makeRequest(const std::string& contentType) {
    std::lock_guard<std::mutex> lock(mutex_request_);
    
    struct curl_slist* headers = NULL;
    if (!contentType.empty()) {
        headers = curl_slist_append(headers, std::string{"Content-Type: " + contentType}.c_str());
        if (contentType == "multipart/form-data") {
            headers = curl_slist_append(headers, "Expect:");
        }
    }
    headers = curl_slist_append(headers, std::string{"Authorization: Bearer " + token_}.c_str());
    if (!organization_.empty()) {
        headers = curl_slist_append(headers, std::string{"OpenAI-Organization: " + organization_}.c_str());
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
    
    std::string response_string;
    std::string header_string;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &header_string);

    res_ = curl_easy_perform(curl_);

    bool is_error = false;
    std::string error_msg{};
    if(res_ != CURLE_OK) {
        is_error = true;
        error_msg = "OpenAI curl_easy_perform() failed: " + std::string{curl_easy_strerror(res_)};
        if (throw_exception_) {
            throw std::runtime_error(error_msg);
        }
        else {
            std::cerr << error_msg << '\n';
        }
    }

    return { response_string, is_error, error_msg };
}

inline std::string Session::easyEscape(const std::string& text) {
    char *encoded_output = curl_easy_escape(curl_, text.c_str(), static_cast<int>(text.length()));
    const auto str = std::string{ encoded_output };
    curl_free(encoded_output);
    return str;
}

// forward declaration for category structures
class  OpenAI;

// https://platform.openai.com/docs/api-reference/models
// List and describe the various models available in the API. You can refer to the Models documentation to understand what models are available and the differences between them.
struct CategoryModel {
    Json list();
    Json retrieve(const std::string& model);

    CategoryModel(OpenAI& openai) : openai_{openai} {}
private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/completions
// Given a prompt, the model will return one or more predicted completions, and can also return the probabilities of alternative tokens at each position.
struct CategoryCompletion {
    Json create(Json input);

    CategoryCompletion(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/chat
// Given a prompt, the model will return one or more predicted chat completions.
struct CategoryChat {
    Json create(Json input);

    CategoryChat(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/audio
// Learn how to turn audio into text.
struct CategoryAudio {
    Json transcribe(Json input);
    Json translate(Json input);

    CategoryAudio(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/edits
// Given a prompt and an instruction, the model will return an edited version of the prompt.
struct CategoryEdit {
    Json create(Json input);

    CategoryEdit(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};


// https://platform.openai.com/docs/api-reference/images
// Given a prompt and/or an input image, the model will generate a new image.
struct CategoryImage {
    Json create(Json input);
    Json edit(Json input);
    Json variation(Json input);

    CategoryImage(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/embeddings
// Get a vector representation of a given input that can be easily consumed by machine learning models and algorithms.
struct CategoryEmbedding {
    Json create(Json input);
    CategoryEmbedding(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

struct FileRequest {
    std::string file;
    std::string purpose;
};

// https://platform.openai.com/docs/api-reference/files
// Files are used to upload documents that can be used with features like Fine-tuning.
struct CategoryFile {
    Json list();
    Json upload(Json input);
    Json del(const std::string& file); // TODO
    Json retrieve(const std::string& file_id);
    Json content(const std::string& file_id);

    CategoryFile(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/fine-tunes
// Manage fine-tuning jobs to tailor a model to your specific training data.
struct CategoryFineTune {
    Json create(Json input);
    Json list();
    Json retrieve(const std::string& fine_tune_id);
    Json content(const std::string& fine_tune_id);
    Json cancel(const std::string& fine_tune_id);
    Json events(const std::string& fine_tune_id);
    Json del(const std::string& model);

    CategoryFineTune(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};

// https://platform.openai.com/docs/api-reference/moderations
// Given a input text, outputs if the model classifies it as violating OpenAI's content policy.
struct CategoryModeration {
    Json create(Json input);

    CategoryModeration(OpenAI& openai) : openai_{openai} {}

private:
    OpenAI& openai_;
};


// OpenAI
class OpenAI {
public:
    OpenAI(const std::string& token = "", const std::string& organization = "", bool throw_exception = true, const std::string& api_base_url = "") 
        : session_{throw_exception}, token_{token}, organization_{organization}, throw_exception_{throw_exception} {
            if (token.empty()) {
                if(const char* env_p = std::getenv("OPENAI_API_KEY")) {
                    token_ = std::string{env_p};
                }
            }
            if (api_base_url.empty()) {
                if(const char* env_p = std::getenv("OPENAI_API_BASE")) {
                    base_url = std::string{env_p} + "/";
                }
                else {
                    base_url = "https://api.openai.com/v1/";
                }
            }
            else {
                base_url = api_base_url;
            }
            session_.setUrl(base_url);
            session_.setToken(token_, organization_);
        }
    
    OpenAI(const OpenAI&)               = delete;
    OpenAI& operator=(const OpenAI&)    = delete;
    OpenAI(OpenAI&&)                    = delete;
    OpenAI& operator=(OpenAI&&)         = delete;

    void setProxy(const std::string& url) { session_.setProxyUrl(url); }

    // void change_token(const std::string& token) { token_ = token; };
    void setThrowException(bool throw_exception) { throw_exception_ = throw_exception; }

    void setMultiformPart(const std::pair<std::string, std::string>& filefield_and_filepath, const std::map<std::string, std::string>& fields) { session_.setMultiformPart(filefield_and_filepath, fields); }

    Json post(const std::string& suffix, const std::string& data, const std::string& contentType) {
        setParameters(suffix, data, contentType);
        auto response = session_.postPrepare(contentType);
        if (response.is_error){ 
            trigger_error(response.error_message);
        }

        Json json{};
        if (isJson(response.text)){

            json = Json::parse(response.text); 
            checkResponse(json);
        }
        else{
          #if OPENAI_VERBOSE_OUTPUT
            std::cerr << "Response is not a valid JSON";
            std::cout << "<< " << response.text << "\n";
          #endif
        }
       
        return json;
    }

    Json get(const std::string& suffix, const std::string& data = "") {
        setParameters(suffix, data);
        auto response = session_.getPrepare();
        if (response.is_error) { trigger_error(response.error_message); }

        Json json{};
        if (isJson(response.text)) {
            json = Json::parse(response.text);
            checkResponse(json);
        }
        else {
          #if OPENAI_VERBOSE_OUTPUT
            std::cerr << "Response is not a valid JSON\n";
            std::cout << "<< " << response.text<< "\n";
          #endif
        }
        return json;
    }

    Json post(const std::string& suffix, const Json& json, const std::string& contentType="application/json") {
        return post(suffix, json.dump(), contentType);
    }

    Json del(const std::string& suffix) {
        setParameters(suffix, "");
        auto response = session_.deletePrepare();
        if (response.is_error) { trigger_error(response.error_message); }

        Json json{};
        if (isJson(response.text)) {
            json = Json::parse(response.text);
            checkResponse(json);
        }
        else {
          #if OPENAI_VERBOSE_OUTPUT
            std::cerr << "Response is not a valid JSON\n";
            std::cout << "<< " << response.text<< "\n";
          #endif
        }
        return json;
    }

    std::string easyEscape(const std::string& text) { return session_.easyEscape(text); }

    void debug() const { std::cout << token_ << '\n'; }

    void setBaseUrl(const std::string &url) {
        base_url = url;
    }

    std::string getBaseUrl() const {
        return base_url;
    }

private:
    std::string base_url;

    void setParameters(const std::string& suffix, const std::string& data, const std::string& contentType = "") {
        auto complete_url =  base_url+ suffix;
        session_.setUrl(complete_url);

        if (contentType != "multipart/form-data") {
            session_.setBody(data);
        }

        #if OPENAI_VERBOSE_OUTPUT
            std::cout << "<< request: "<< complete_url << "  " << data << '\n';
        #endif
    }

    void checkResponse(const Json& json) {
        if (json.count("error")) {
            auto reason = json["error"].dump();
            trigger_error(reason);

            #if OPENAI_VERBOSE_OUTPUT
                std::cerr << ">> response error :\n" << json.dump(2) << "\n";
            #endif
        } 
    }

    // as of now the only way
    bool isJson(const std::string &data){
        bool rc = true;
        try {
            auto json = Json::parse(data); // throws if no json 
        }
        catch (std::exception &){
            rc = false;
        }
        return(rc);
    }

    void trigger_error(const std::string& msg) {
        if (throw_exception_) {
            throw std::runtime_error(msg);
        }
        else {
            std::cerr << "[OpenAI] error. Reason: " << msg << '\n';
        }
    }

public:
    CategoryModel           model     {*this};
    CategoryCompletion      completion{*this};
    CategoryEdit            edit      {*this};
    CategoryImage           image     {*this};
    CategoryEmbedding       embedding {*this};
    CategoryFile            file      {*this};
    CategoryFineTune        fine_tune {*this};
    CategoryModeration      moderation{*this};
    CategoryChat            chat      {*this};
    CategoryAudio           audio     {*this};
    // CategoryEngine          engine{*this}; // Not handled since deprecated (use Model instead)

private:
    Session                 session_;
    std::string             token_;
    std::string             organization_;
    bool                    throw_exception_;
};

inline std::string bool_to_string(const bool b) {
    std::ostringstream ss;
    ss << std::boolalpha << b;
    return ss.str();
}

inline OpenAI& start(const std::string& token = "", const std::string& organization = "", bool throw_exception = true)  {
    static OpenAI instance{token, organization, throw_exception};
    return instance;
}

inline OpenAI& instance() {
    return start();
}

inline Json post(const std::string& suffix, const Json& json) {
    return instance().post(suffix, json);
}

inline Json get(const std::string& suffix/*, const Json& json*/) {
    return instance().get(suffix);
}

// Helper functions to get category structures instance()

inline CategoryModel& model() {
    return instance().model;
}

inline CategoryCompletion& completion() {
    return instance().completion;
}

inline CategoryChat& chat() {
    return instance().chat;
}

inline CategoryAudio& audio() {
    return instance().audio;
}

inline CategoryEdit& edit() {
    return instance().edit;
}

inline CategoryImage& image() {
    return instance().image;
}

inline CategoryEmbedding& embedding() {
    return instance().embedding;
}

inline CategoryFile& file() {
    return instance().file;
}

inline CategoryFineTune& fineTune() {
    return instance().fine_tune;
}

inline CategoryModeration& moderation() {
    return instance().moderation;
}

// Definitions of category methods

// GET https://api.openai.com/v1/models
// Lists the currently available models, and provides basic information about each one such as the owner and availability.
inline Json CategoryModel::list() {
    return openai_.get("models");
}

// GET https://api.openai.com/v1/models/{model}
// Retrieves a model instance, providing basic information about the model such as the owner and permissioning.
inline Json CategoryModel::retrieve(const std::string& model) {
    return openai_.get("models/" + model);
}

// POST https://api.openai.com/v1/completions
// Creates a completion for the provided prompt and parameters
inline Json CategoryCompletion::create(Json input) {
    return openai_.post("completions", input);
}

// POST https://api.openai.com/v1/chat/completions
// Creates a chat completion for the provided prompt and parameters
inline Json CategoryChat::create(Json input) {
    return openai_.post("chat/completions", input);
}

// POST https://api.openai.com/v1/audio/transcriptions
// Transcribes audio into the input language.
inline Json CategoryAudio::transcribe(Json input) {
    openai_.setMultiformPart({"file", input["file"].get<std::string>()}, 
        std::map<std::string, std::string>{{"model", input["model"].get<std::string>()}}
    );

    return openai_.post("audio/transcriptions", std::string{""}, "multipart/form-data"); 
}

// POST https://api.openai.com/v1/audio/translations
// Translates audio into into English..
inline Json CategoryAudio::translate(Json input) {
    openai_.setMultiformPart({"file", input["file"].get<std::string>()}, 
        std::map<std::string, std::string>{{"model", input["model"].get<std::string>()}}
    );

    return openai_.post("audio/translations", std::string{""}, "multipart/form-data"); 
}

// POST https://api.openai.com/v1/translations
// Creates a new edit for the provided input, instruction, and parameters
inline Json CategoryEdit::create(Json input) {
    return openai_.post("edits", input);
}

// POST https://api.openai.com/v1/images/generations
// Given a prompt and/or an input image, the model will generate a new image.
inline Json CategoryImage::create(Json input) {
    return openai_.post("images/generations", input);
}

// POST https://api.openai.com/v1/images/edits
// Creates an edited or extended image given an original image and a prompt.
inline Json CategoryImage::edit(Json input) {
    std::string prompt = input["prompt"].get<std::string>(); // required
    // Default values
    std::string mask = "";
    int n = 1;
    std::string size = "1024x1024";
    std::string response_format = "url";
    std::string user = "";
    
    if (input.contains("mask")) {
        mask = input["mask"].get<std::string>();
    }
    if (input.contains("n")) {
        n = input["n"].get<int>();
    }
    if (input.contains("size")) {
        size = input["size"].get<std::string>();
    }
    if (input.contains("response_format")) {
        response_format = input["response_format"].get<std::string>();
    }
    if (input.contains("user")) {
        user = input["user"].get<std::string>();
    }
    openai_.setMultiformPart({"image",input["image"].get<std::string>()}, 
        std::map<std::string, std::string>{
            {"prompt", prompt},
            {"mask", mask},
            {"n", std::to_string(n)},
            {"size", size},
            {"response_format", response_format},
            {"user", user}
        }
    );

    return openai_.post("images/edits", std::string{""}, "multipart/form-data"); 
}

// POST https://api.openai.com/v1/images/variations
// Creates a variation of a given image.
inline Json CategoryImage::variation(Json input) {
    // Default values
    int n = 1;
    std::string size = "1024x1024";
    std::string response_format = "url";
    std::string user = "";
    
    if (input.contains("n")) {
        n = input["n"].get<int>();
    }
    if (input.contains("size")) {
        size = input["size"].get<std::string>();
    }
    if (input.contains("response_format")) {
        response_format = input["response_format"].get<std::string>();
    }
    if (input.contains("user")) {
        user = input["user"].get<std::string>();
    }
    openai_.setMultiformPart({"image",input["image"].get<std::string>()}, 
        std::map<std::string, std::string>{
            {"n", std::to_string(n)},
            {"size", size},
            {"response_format", response_format},
            {"user", user}
        }
    );

    return openai_.post("images/variations", std::string{""}, "multipart/form-data"); 
}

inline Json CategoryEmbedding::create(Json input) { 
    return openai_.post("embeddings", input); 
}

inline Json CategoryFile::list() { 
    return openai_.get("files"); 
}

inline Json CategoryFile::upload(Json input) {
    openai_.setMultiformPart({"file", input["file"].get<std::string>()}, 
        std::map<std::string, std::string>{{"purpose", input["purpose"].get<std::string>()}}
    );

    return openai_.post("files", std::string{""}, "multipart/form-data"); 
}

inline Json CategoryFile::del(const std::string& file_id) { 
    return openai_.del("files/" + file_id); 
}

inline Json CategoryFile::retrieve(const std::string& file_id) { 
    return openai_.get("files/" + file_id); 
}

inline Json CategoryFile::content(const std::string& file_id) { 
    return openai_.get("files/" + file_id + "/content"); 
}

inline Json CategoryFineTune::create(Json input) { 
    return openai_.post("fine-tunes", input); 
}

inline Json CategoryFineTune::list() { 
    return openai_.get("fine-tunes"); 
}

inline Json CategoryFineTune::retrieve(const std::string& fine_tune_id) { 
    return openai_.get("fine-tunes/" + fine_tune_id); 
}

inline Json CategoryFineTune::content(const std::string& fine_tune_id) { 
    return openai_.get("fine-tunes/" + fine_tune_id + "/content"); 
}

inline Json CategoryFineTune::cancel(const std::string& fine_tune_id) { 
    return openai_.post("fine-tunes/" + fine_tune_id + "/cancel", Json{}); 
}

inline Json CategoryFineTune::events(const std::string& fine_tune_id) { 
    return openai_.get("fine-tunes/" + fine_tune_id + "/events"); 
}

inline Json CategoryFineTune::del(const std::string& model) { 
    return openai_.del("models/" + model); 
}

inline Json CategoryModeration::create(Json input) { 
    return openai_.post("moderations", input); 
}

} // namespace _detail

// Public interface

using _detail::OpenAI;

// instance
using _detail::start;
using _detail::instance;

// Generic methods
using _detail::post;
using _detail::get;

// Helper categories access
using _detail::model;
using _detail::completion;
using _detail::edit;
using _detail::image;
using _detail::embedding;
using _detail::file;
using _detail::fineTune;
using _detail::moderation;
using _detail::chat;
using _detail::audio;

using _detail::Json;

} // namespace openai

#endif // OPENAI_HPP_
