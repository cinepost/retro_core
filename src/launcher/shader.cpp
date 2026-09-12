#include "shader.h"
#include "os.h"
#include "IconsFontAwesome6.h"

#include <imgui.h>
#include <cstring>
#include <regex>


namespace RetroLauncher {

static bool checkShaderCompileErrors(GLuint shader, const std::string& type, std::string_view programName) {
    GLint success;
    GLchar infoLog[1024];
    
    // Check the compilation status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        // Retrieve the error log message
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "ERROR::SHADER:: " << type << "::COMPILATION_ERROR of program: " << programName << "\n" 
                  << infoLog << "\n -- --------------------------------------------------- -- " 
                  << std::endl;
        return false;
    }
    return true;
}

static void checkProgramLinkErrors(GLuint program) {
    GLint success;
    GLchar infoLog[1024];
    
    // Check the linking status
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        // Retrieve the program link log message
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR:\n" 
                  << infoLog << "\n -- --------------------------------------------------- -- " 
                  << std::endl;
    }
}

static void printSourceForDebug(const std::string& shader_str, std::string_view programName) {
    std::stringstream ss(shader_str);
    std::string line;
    int lineNumber = 1;

    std::cout << "\n--- " << programName << " SHADER SOURCE WITH LINE NUMBERS ---\n";
    while (std::getline(ss, line)) {
        // setw(4) pads the numbers so lines 1-999 align beautifully with your code
        std::cout << std::setw(4) << lineNumber << ": " << line << "\n";
        lineNumber++;
    }
    std::cout << "---------------------------------------\n\n";
}

static GLuint compileShaderProgram(const std::string& vert_src_str, const std::string& frag_src_str, const std::string& geom_src_str, std::string_view programName) {
    const char* vertCodeRaw = vert_src_str.c_str();
    const char* fragCodeRaw = frag_src_str.c_str();
    const char* geomCodeRaw = geom_src_str.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLint vertShaderLength = static_cast<GLint>(vert_src_str.length());
    glShaderSource(vertexShader, 1, &vertCodeRaw, &vertShaderLength); 
    glCompileShader(vertexShader);
    if(!checkShaderCompileErrors(vertexShader, "VERTEX", programName)) {
        printSourceForDebug(vert_src_str, programName);
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLint fragShaderLength = static_cast<GLint>(frag_src_str.length());
    glShaderSource(fragmentShader, 1, &fragCodeRaw, &fragShaderLength); 
    glCompileShader(fragmentShader);
    if(!checkShaderCompileErrors(fragmentShader, "FRAGMENT", programName)) {
        printSourceForDebug(frag_src_str, programName);
    }

    const bool has_geometry_shader = !geom_src_str.empty();
     GLuint geometryShader = 0;
    if(has_geometry_shader) {
        geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        GLint geomShaderLength = static_cast<GLint>(geom_src_str.length());
        glShaderSource(geometryShader, 1, &geomCodeRaw, &geomShaderLength); 
        glCompileShader(geometryShader);
        if(!checkShaderCompileErrors(geometryShader, "GEOMETRY", programName)) {
            printSourceForDebug(geom_src_str, programName);
        }
    }
    
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);

    if(has_geometry_shader) {
        glAttachShader(programID, geometryShader);
    }

    glLinkProgram(programID);
    checkProgramLinkErrors(programID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);
    return programID;
}

Shader::Shader(const std::string& name): 
    mName(name), 
    mLastCheckTime(std::chrono::steady_clock::now()),
    mProgramID(0),
    mInitialized(false),
    mNeedRecompile(true)
{
    clearShaderParameters();
    clearUniformCache();
    clearDefines();
}

Shader::Shader(const std::string& name, const std::string& vertPath, const std::string& fragPath): Shader(name) {
    mInitialized = init(vertPath, fragPath);
}


Shader::Shader(const std::string& name, const std::string& vertPath, const std::string& fragPath, const std::string& pGeomPath): Shader(name) {
    mInitialized = init(vertPath, fragPath, pGeomPath);
}

bool Shader::init(const std::string& vertPath, const std::string& fragPath) {
    static const std::string sEmptyPath;
    return init(vertPath, fragPath, sEmptyPath);
}

bool Shader::init(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath) {
    if(mInitialized) return true;

    mInitialized = false;

    fs::path finalVertPath(vertPath);
    fs::path finalFragPath(fragPath);
    fs::path finalGeomPath(geomPath);

    static const auto sExecutableDir = getExecutableDir();

    if(!finalVertPath.empty() && finalVertPath.is_relative()) {
        finalVertPath = sExecutableDir / finalVertPath;
    }

    if(!finalFragPath.empty() && finalFragPath.is_relative()) {
        finalFragPath = sExecutableDir / finalFragPath;
    }

    if(!finalGeomPath.empty() && finalGeomPath.is_relative()) {
        finalGeomPath = sExecutableDir / finalGeomPath;
    }

    if (!finalFragPath.empty() && !fs::exists(finalVertPath)) {
        std::cerr << "ERROR::VERT_SHADER::FILE_NOT_FOUND: " << vertPath << std::endl;
    }

    if (!finalFragPath.empty() && !fs::exists(finalFragPath)) {
        std::cerr << "ERROR::FRAG_SHADER::FILE_NOT_FOUND: " << fragPath << std::endl;
    }

    if (!finalGeomPath.empty() && !fs::exists(finalGeomPath)) {
        std::cerr << "ERROR::FRAG_SHADER::FILE_NOT_FOUND: " << geomPath << std::endl;
    }

    std::error_code ec;

    mVertSourcePath = finalVertPath;
    mFragSourcePath = finalFragPath;
    mGeomSourcePath = finalGeomPath;

    // Track initial files modification time
    mLastVertShaderSourceWriteTime = fs::last_write_time(mVertSourcePath);
    mLastFragShaderSourceWriteTime = fs::last_write_time(mFragSourcePath);
    
    if (!mGeomSourcePath.empty()) {
        mLastGeomShaderSourceWriteTime = fs::last_write_time(mGeomSourcePath, ec);
    }

    if (mGeomSourcePath.empty() || ec) {
        mLastGeomShaderSourceWriteTime = fs::file_time_type::min();
    }
            
    // Initial compilation
    compileAndLink();

    mInitialized = true;
    return mInitialized;
}

bool Shader::checkAndReload(bool force) {
    auto now = std::chrono::steady_clock::now();
        
    // Skip disk I/O if the interval hasn't passed AND we are not forcing it
    static const std::chrono::milliseconds sCheckInterval{500}; // Throttle disk reads to every 500ms
    now = std::chrono::steady_clock::now();

    if (!force && (now - mLastCheckTime < sCheckInterval)) {
        return true;
    }
    mLastCheckTime = now;

    if (!fs::exists(mVertSourcePath)) {
        return false;
    }

    if (!fs::exists(mFragSourcePath)) {
        return false;
    }

    if (!mGeomSourcePath.empty() && !fs::exists(mGeomSourcePath)) {
        return false;
    }

    try {
        // Get the current modification timestamp on disk
        std::error_code ec;

        auto currentVertShaderSourceWriteTime = fs::last_write_time(mVertSourcePath);
        auto currentFragShaderSourceWriteTime = fs::last_write_time(mFragSourcePath);
        auto currentGeomShaderSourceWriteTime = fs::last_write_time(mGeomSourcePath, ec);

        if (mGeomSourcePath.empty() || ec) {
            currentGeomShaderSourceWriteTime = fs::file_time_type::min();
        }

        bool recompile = false;

        if(!force) {
            // If the timestamps don't match, the file was modified!
            if (currentVertShaderSourceWriteTime != mLastVertShaderSourceWriteTime) {
                std::cout << "\n[Shader] Vertex shader change detected in: " << mVertSourcePath;
                mLastVertShaderSourceWriteTime = currentVertShaderSourceWriteTime;
                recompile = true;
            }

            if (currentFragShaderSourceWriteTime != mLastFragShaderSourceWriteTime) {
                std::cout << "\n[Shader] Fragment shader change detected in: " << mFragSourcePath;
                mLastFragShaderSourceWriteTime = currentFragShaderSourceWriteTime;
                recompile = true;
            }

            if(!mGeomSourcePath.empty() && currentGeomShaderSourceWriteTime != mLastGeomShaderSourceWriteTime) {
                std::cout << "\n[Shader] Geometry shader change detected in: " << mGeomSourcePath;
                mLastGeomShaderSourceWriteTime = currentGeomShaderSourceWriteTime;
                recompile = true;
            }
        }

        if(recompile || force) {
            // Reload and compile
            compileAndLink();
        }
    } catch (const fs::filesystem_error& e) {
        // Catches edge-cases where the file is temporarily locked by your code editor during a save
        std::cerr << "[Shader] Warning: Failed to check file status: " << e.what() << std::endl;
    }

    return true;
}

void Shader::compileAndLink() {
    clearShaderParameters();
    clearUniformCache();

    std::string vertSource = loadShaderSourceWithIncludes(mVertSourcePath);
    std::string fragSource = loadShaderSourceWithIncludes(mFragSourcePath);
    std::string geomSource = loadShaderSourceWithIncludes(mGeomSourcePath);

    parseParameters(vertSource);
    parseParameters(fragSource);
    parseParameters(geomSource);

    injectDefines(vertSource, mDefines, mVertSourcePath);
    injectDefines(fragSource, mDefines, mFragSourcePath);
    injectDefines(geomSource, mDefines, mGeomSourcePath);

    mProgramID = compileShaderProgram(vertSource, fragSource, geomSource, mName);
    assert(mProgramID != 0);
    mNeedRecompile = false;
}

std::string Shader::loadShaderSourceWithIncludes(const std::string& providedPath, int depth) {
    if(providedPath.empty()) return "";
    // Prevent infinite recursion loops if shaders accidentally include each other
    if (depth > 10) {
        std::cerr << "[Shader] ERROR: Max include depth exceeded! Loop detected.\n";
        return "";
    }

    static const std::string sExecutableDir = getExecutableDir();

    fs::path finalPath(providedPath);
    if (finalPath.is_relative()) {
        finalPath = sExecutableDir / finalPath;
    }
    finalPath = fs::weakly_canonical(finalPath);

    std::ifstream file(finalPath);
    if (!file.is_open()) {
        std::cerr << "[Shader] ERROR: Could not open file: " << finalPath.string() << "\n";
        return "";
    }

    std::string fullSource = "";
    std::string line;
    
    // Regex to match: #include "filename.glsl" or #include <filename.glsl>
    std::regex includeRegex(R"(\s*#include\s+["<]([^">]+)[">])");

    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_match(line, match, includeRegex)) {
            // Extracted include file name (e.g., "common_functions.glsl")
            std::string includeFileName = match[1].str();
            
            // Look for the include file relative to the CURRENT shader's directory
            fs::path includePath = finalPath.parent_path() / includeFileName;

            // Recursively load the include file
            std::string includeSource = loadShaderSourceWithIncludes(includePath.string(), depth + 1);
            fullSource += includeSource + "\n";
        } else {
            fullSource += line + "\n";
        }
    }

    return fullSource;
}

void Shader::injectDefines(std::string& source, const DefinesList& defines, const std::string& shaderPath) {
    if(defines.empty()) return;

    size_t versionPos = source.find("#version");
    if (versionPos == std::string::npos) {
        std::cerr << "[Shader] no #version string found in " << shaderPath << "! Injections skipped !!!" << std::endl;
        return;
    }    

    size_t endOfLine = source.find("\n", versionPos);
    
    // Inject the exact numeric enum defines cleanly into the pipeline
    std::string definesString("/* --- INJECTED DEFINES START --- */\n\n");

    for(const auto& entry: defines) {
        definesString += "#define " + entry.first + " " + entry.second + "\n";
    }

    definesString += "\n/* --- INJECTED DEFINES END --- */\n\n";
    source.insert(endOfLine + 1, definesString);
}

ShaderParameter::ParamType Shader::stringToType(const std::string& str, int& outChannels) const {
    if (str == "float") { outChannels = 1; return ShaderParameter::ParamType::Float; }
    if (str == "vec2")  { outChannels = 2; return ShaderParameter::ParamType::Vec2; }
    if (str == "vec3")  { outChannels = 3; return ShaderParameter::ParamType::Vec3; }
    if (str == "vec4")  { outChannels = 4; return ShaderParameter::ParamType::Vec4; }
    if (str == "int")   { outChannels = 1; return ShaderParameter::ParamType::Int; }
    if (str == "int2")  { outChannels = 2; return ShaderParameter::ParamType::Int2; }
    if (str == "int3")  { outChannels = 3; return ShaderParameter::ParamType::Int3; }
    if (str == "int4")  { outChannels = 4; return ShaderParameter::ParamType::Int4; }
    if (str == "uint")  { outChannels = 1; return ShaderParameter::ParamType::Uint; }
    if (str == "uint2") { outChannels = 2; return ShaderParameter::ParamType::Uint2; }
    if (str == "uint3") { outChannels = 3; return ShaderParameter::ParamType::Uint3; }
    if (str == "uint4") { outChannels = 4; return ShaderParameter::ParamType::Uint4; }
    if (str == "bool")  { outChannels = 1; return ShaderParameter::ParamType::Bool; }

    std::cerr << "[Shader] Warning uknown glsl type " << str << " found!" << std::endl;
    outChannels = 1; return ShaderParameter::ParamType::Float; // Fallback
}

void Shader::parseParameters(const std::string& fullSourceCode) {
    if(fullSourceCode.empty()) return;

    // Matches: #pragma parameter <type> <id> "<label>" <values...>
    // Group 1: Type, Group 2: Identifier, Group 3: Label, Group 4: The remaining numbers string
    std::regex pragmaRegex("#pragma\\s+parameter\\s+([a-zA-Z0-9]+)\\s+([a-zA-Z0-9_]+)\\s+\"([^\"]+)\"\\s+([^\\r\\n]+)");


    std::stringstream ss(fullSourceCode);
    std::string line;

    while (std::getline(ss, line)) {
        std::smatch match;
        if (std::regex_search(line, match, pragmaRegex)) {
            std::string typeStr = match[1].str();
            std::string id = match[2].str();

            if(mShaderParameters.find(id) != mShaderParameters.end()) {
                std::cerr << "[Shader] Error parsing parameter \"" << id << "\"! Already exist!" << std::endl;
                continue;
            }

            std::string label = match[3].str();
            std::string valuesRaw = match[4].str();

            // If already tracking and tweaked via ImGui, preserve live state!
            if (mShaderParameters.find(id) != mShaderParameters.end()) continue;

            int channels = 1;
            if(channels < 1 || channels > 4) {
                std::cerr << "[Shader System] Error parsing " << id << ". Unsupported channels count " << channels << "\n";
                continue;
            }

            ShaderParameter::ParamType pType = stringToType(typeStr, channels);

            // Parse out all floats from the tail string sequentially
            std::vector<float> tokens;
            std::stringstream valStream(valuesRaw);
            float val;
            while (valStream >> val) {
                tokens.push_back(val);
            }

            // Check structure: Expected format requires (channels * 3) numbers: 
            // [defaults...], [mins...], [maxs...]
            if (tokens.size() < static_cast<size_t>(channels * 3)) {
                std::cerr << "[Shader System] Error parsing " << id << ". Not enough min/max bounds specified.\n";
                continue;
            }

            auto& param = mShaderParameters[id];
            param.mIdentifier = id;
            param.mLabel = label;
            param.mType = pType;

            for (int i = 0; i < channels; ++i) {
                param.mDefaultValue[i] = tokens[i];
                param.mValue[i] = tokens[i];
                param.mMin[i]   = tokens[channels + i];
                param.mMax[i]   = tokens[(channels * 2) + i];
            }
        }
    }
}

void Shader::setShaderParameters() const {
    for (const auto& [id, param] : mShaderParameters) {
        int loc = getUniformLocation(id);
        if (loc == -1) continue;

        const float* pParmFloatData = static_cast<const float*>(param.getValuePtr());
        
        uint32_t elem_count = param.parmTypeElementsCount();
        uint32_t uVal[4];
        int32_t  iVal[4];

        for (uint32_t i = 0; i < elem_count; ++i) {
            uVal[i] = static_cast<uint32_t>(pParmFloatData[i]);
            iVal[i] = static_cast<int32_t>(pParmFloatData[i]);
        }

        switch (param.mType) {
            case ShaderParameter::ParamType::Float: glUniform1fv(loc, 1, pParmFloatData); break;
            case ShaderParameter::ParamType::Vec2:  glUniform2fv(loc, 1, pParmFloatData); break;
            case ShaderParameter::ParamType::Vec3:  glUniform3fv(loc, 1, pParmFloatData); break;
            case ShaderParameter::ParamType::Vec4:  glUniform4fv(loc, 1, pParmFloatData); break;
            
            case ShaderParameter::ParamType::Int:   glUniform1iv(loc, 1, &iVal[0]); break;
            case ShaderParameter::ParamType::Int2:  glUniform2iv(loc, 1, &iVal[0]); break;
            case ShaderParameter::ParamType::Int3:  glUniform3iv(loc, 1, &iVal[0]); break;
            case ShaderParameter::ParamType::Int4:  glUniform4iv(loc, 1, &iVal[0]); break;
            
            case ShaderParameter::ParamType::Uint:  glUniform1uiv(loc, 1, &uVal[0]); break;
            case ShaderParameter::ParamType::Uint2: glUniform2uiv(loc, 1, &uVal[0]); break;
            case ShaderParameter::ParamType::Uint3: glUniform3uiv(loc, 1, &uVal[0]); break;
            case ShaderParameter::ParamType::Uint4: glUniform4uiv(loc, 1, &uVal[0]); break;

            case ShaderParameter::ParamType::Bool:  glUniform1i(loc, iVal[0]); break;
        }
    }
}


template<typename T>
T Shader::getShaderParameterValue(const std::string& parmName, const T& defaultValue) const {
    auto it = mShaderParameters.find(parmName);
    assert(it != mShaderParameters.end());
    if(it == mShaderParameters.end()) {
        std::cerr << "[Shader] Error! Shader parameter " << parmName << " not found !!!" << std::endl;
        return defaultValue;
    }

    return it->second;
}

void Shader::linkShaderParameter(const std::string& parmName, const uint32_t& source, uint32_t min_value, uint32_t max_value) {
    auto it = mShaderParameters.find(parmName);
    if (it != mShaderParameters.end()) {
        // TODO: handle if exist
        //std::cerr << "[Shader] parameter " << parmName << " already exists !!!" << std::endl;
        return;
    }

    auto& parm = mShaderParameters[parmName];

    std::lock_guard<std::mutex> lock(parm.mMutex);

    parm.mIdentifier = parmName;  // GLSL uniform name
    parm.mLabel = parmName;       // ImGui title
    parm.mMin[0] = static_cast<float>(min_value);
    parm.mMax[0] = static_cast<float>(max_value);   
    parm.mDefaultValue[0] = static_cast<float>(source);
    parm.mType = ShaderParameter::ParamType::Uint;
    parm.mpLinkedData = &source;
    parm.mLinkedElementCount = 1;
    parm.mLinked = true;
}

void Shader::resetShaderParameters() {
    if(mShaderParameters.empty()) return;

    for (auto& [id, param] : mShaderParameters) {
        param.resetToDefault();
    }
}

void Shader::drawUI() {
    if (mShaderParameters.empty()) return;

    ImGui::PushID(this);

    int i = 0;

    for (auto& [id, param] : mShaderParameters) {
        bool valid_param = true;
        
        const char* pParmName = param.mLabel.c_str();

        const float* pParmFloatData = static_cast<const float*>(param.getValuePtr());

        const bool external = param.hasExternalSource();
        const bool linked = param.isLinked();

        bool disabled = false;
        if (linked) {
            ImGui::BeginDisabled();
            disabled = true; 
        }

        ImGui::PushID(i);
        switch (param.mType) {
            case ShaderParameter::ParamType::Float:
                ImGui::SliderFloat(pParmName, &param.mValue[0], param.mMin[0], param.mMax[0]);
                break;
            case ShaderParameter::ParamType::Vec2:
                ImGui::SliderFloat2(pParmName, param.mValue, param.mMin[0], param.mMax[0]);
                break;
            case ShaderParameter::ParamType::Vec3:
                // Treat vec3 as color wheels automatically if labeled "Color"
                if (param.mLabel.find("Color") != std::string::npos) {
                    ImGui::ColorEdit3(pParmName, param.mValue);
                } else {
                    ImGui::SliderFloat3(pParmName, param.mValue, param.mMin[0], param.mMax[0]);
                }
                break;
            case ShaderParameter::ParamType::Vec4:
                ImGui::SliderFloat4(pParmName, param.mValue, param.mMin[0], param.mMax[0]);
                break;
            case ShaderParameter::ParamType::Int: {
                int val = static_cast<int>(pParmFloatData[0]);
                if (ImGui::SliderInt(pParmName, &val, static_cast<int>(param.mMin[0]), static_cast<int>(param.mMax[0]))) {
                    param.mValue[0] = static_cast<float>(val);
                }
                break;
            }
            case ShaderParameter::ParamType::Uint: {
                int val = static_cast<unsigned int>(pParmFloatData[0]);
                if (ImGui::SliderInt(pParmName, &val, static_cast<float>(param.mMin[0]), static_cast<float>(param.mMax[0]))) {
                    param.mValue[0] = static_cast<float>(val);
                }
                break;
            }
            case ShaderParameter::ParamType::Bool: {
                bool val = static_cast<bool>(pParmFloatData[0]);
                if (ImGui::Checkbox(pParmName, &val)) {
                    param.mValue[0] = static_cast<float>(val);
                }
                break;
            }
            // Int2, Int3, Int4, and UInt formats can use ImGui::SliderScalarN 
            // or casting structures similarly as needed...
            case ShaderParameter::ParamType::Unknown:
            default:
                valid_param = false;
                ImGui::Text("%s (Unsupported Type UI)", pParmName);
                break;
        }

        if (disabled) {
            ImGui::EndDisabled(); 
        }

        if (valid_param) {
            // Link/Unlink button   
            ImGui::SameLine();
            if(external) {
                if (ImGui::Button(linked ? "U###linker" : "L###linker")) {
                    param.toggleLinkState();
                }

                if (ImGui::IsItemHovered()) {
                    if(linked) {
                        ImGui::SetTooltip("Unlink");
                    } else {
                        ImGui::SetTooltip("Link back");
                    }
                }
            }

            // Reset button
            if(!linked) {
                // Rest to default button
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_ROTATE_LEFT "###reset")) {
                    param.resetToDefault();
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Reset to default");
                }
            } 
        }

        if (linked) {
            ImGui::SameLine();
            ImGui::TextDisabled("(Linked to C++)"); // Visual anchor
        }

        ImGui::PopID();
        i++;
    }

    if(i > 0) {
        if (ImGui::Button("Reset All###reset")) {
            resetShaderParameters();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Reset all parameters to default");
        }
    }

    ImGui::PopID();
}

void Shader::clearShaderParameters() {
    // clear parameter but keep linked
    std::erase_if(mShaderParameters, [](const auto& item) {

        if(item.second.hasExternalSource()) {
            std::cout << "[Shader] Parameter " << item.first << " is linked to external data source !" << std::endl;
            return false;
        }

        return true; 
    });
}


void Shader::destroy() {
    if(mProgramID > 0) {
        glDeleteProgram(mProgramID);
    }
}

Shader::~Shader() {
    destroy();
}

}  // namespace RetroLauncher
