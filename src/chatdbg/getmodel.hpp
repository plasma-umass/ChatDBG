#include <cstdlib>
#include <iostream>
#include <vector>

std::string get_model() {
    std::vector<std::string> all_models = {"gpt-4", "gpt-3.5-turbo"};
    const char *model_env = std::getenv("OPENAI_API_MODEL");
    
    std::string model = model_env ? model_env : "gpt-4";
    
    if(std::find(all_models.begin(), all_models.end(), model) == all_models.end()) {
      return std::string("");
    }
    
    return model;
}
