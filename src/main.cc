#include <iostream>
#include <filesystem>
#include <fstream>
#include "filedb.hh"
#include "cpp/cppcomprehensionengine.hh"

using namespace CodeComprehension;

static bool s_some_test_failed = false;

#define I_TEST(name)                     \
    {                                    \
        printf("Testing " #name "... "); \
        fflush(stdout);                  \
    }

#define PASS              \
    do {                  \
        printf("PASS\n"); \
        fflush(stdout);   \
        return;           \
    } while (0)

#define FAIL(reason)                   \
    do {                               \
        printf("FAIL: " #reason "\n"); \
        s_some_test_failed = true;     \
        return;                        \
    } while (0)

#define RUN(function)         \
    function;                 \
    if (s_some_test_failed) { \
        return 1;             \
    }

class LocalFileDB : public CodeComprehension::FileDB {
public:
    LocalFileDB() = default;

    void add(std::string filename, std::string content)
    {
        m_map.emplace(filename, content);
    }

    virtual std::optional<std::string> get_or_read_from_filesystem(std::string_view filename) const override
    {
        std::string target_filename = std::string{filename};
        if (project_root().has_value() && filename.starts_with(*project_root())) {
            target_filename = std::filesystem::relative(filename, *project_root());
        }

        auto result = m_map.find(target_filename);
        if(result == m_map.end())
            return std::nullopt;

        return result->second;
    }

private:
    std::unordered_map<std::string, std::string> m_map;
};

std::string TESTS_ROOT_DIR = "";

static std::string read_all(std::filesystem::path path) {
    std::ifstream file(path);
    if(!file)  {
        dbgln("Unable to load {}", path.c_str());
        throw std::runtime_error("unable to load file");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

static void add_file(LocalFileDB& filedb, std::string const& name)
{
    std::filesystem::path root_dir_path{TESTS_ROOT_DIR};
    std::filesystem::path name_path{name};
    auto final_path = root_dir_path / name_path;
    auto file_content = read_all(final_path);
    filedb.add(name, file_content);
}

void test_complete_local_args()
{
    I_TEST(Complete Local Args)
    LocalFileDB filedb;
    add_file(filedb, "complete_local_args.cc");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto suggestions = engine.get_suggestions("complete_local_args.cc", { 2, 6 });
    if (suggestions.size() != 2)
        FAIL(bad size);

    if (suggestions[0].completion == "argc" && suggestions[1].completion == "argv")
        PASS;

    FAIL("wrong results");
}


void test_complete_local_vars()
{
    I_TEST(Complete Local Vars)
    LocalFileDB filedb;
    add_file(filedb, "complete_local_vars.cc");
    CodeComprehension::Cpp::CppComprehensionEngine autocomplete(filedb);
    auto suggestions = autocomplete.get_suggestions("complete_local_vars.cc", { 3, 7 });
    if (suggestions.size() != 1)
        FAIL(bad size);

    if (suggestions[0].completion == "myvar1")
        PASS;

    FAIL("wrong results");
}

void test_complete_type()
{
    I_TEST(Complete Type)
    LocalFileDB filedb;
    add_file(filedb, "complete_type.cc");
    CodeComprehension::Cpp::CppComprehensionEngine autocomplete(filedb);
    auto suggestions = autocomplete.get_suggestions("complete_type.cc", { 5, 7 });
    if (suggestions.size() != 1)
        FAIL(bad size);

    if (suggestions[0].completion == "MyStruct")
        PASS;

    FAIL("wrong results");
}

void test_find_function_declaration()
{
    I_TEST("Find Function Declaration");
    LocalFileDB filedb;
    add_file(filedb, "find_function_declaration.cc");
    add_file(filedb, "sample_header.hh");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);

    // Find function declaration in the same file
    auto position = engine.find_declaration_of("find_function_declaration.cc", { 10, 6 });
    if (!position.has_value())
        FAIL("declaration not found (1)");

    if (position.value().file != "find_function_declaration.cc" || position.value().line != 1)
        FAIL("wrong declaration location (1)");

    // Find function declaration in header
    position = engine.find_declaration_of("find_function_declaration.cc", { 11, 6 });
    if (!position.has_value())
        FAIL("declaration not found (2)");

    if (position.value().file != "sample_header.hh" && position.value().line != 2)
        FAIL("wrong declaration location (2)");

    // Find member function declaration
    position = engine.find_declaration_of("find_function_declaration.cc", { 13, 8 });
    if (!position.has_value())
        FAIL("declaration not found (3)");

    if (position.value().file != "find_function_declaration.cc" || position.value().line != 4)
        FAIL("wrong declaration location (3)");

    // Find member function declaration from within a function declaration
    position = engine.find_declaration_of("find_function_declaration.cc", { 17, 6 });
    if (!position.has_value())
        FAIL("declaration not found (4)");

    if (position.value().file != "find_function_declaration.cc" || position.value().line != 5)
        FAIL("wrong declaration location (4)");
    PASS;
}

void test_find_variable_definition()
{
    I_TEST(Find Variable Declaration)
    LocalFileDB filedb;
    add_file(filedb, "find_variable_declaration.cc");
    add_file(filedb, "sample_header.hh");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);

    // Find local variable
    auto position = engine.find_declaration_of("find_variable_declaration.cc", { 4, 5 });
    if (!position.has_value())
        FAIL("declaration not found (1)");

    if (position.value().file != "find_variable_declaration.cc" || position.value().line != 2 || position.value().column < 19)
        FAIL("wrong declaration location (1)");;

    // Find variable in header file
    position = engine.find_declaration_of("find_variable_declaration.cc", { 5, 4 });
    if (!position.has_value())
        FAIL("declaration not found (2)");

    if (position.value().file != "sample_header.hh" || position.value().line != 4)
        FAIL("wrong declaration location (2)");;

    // Find variable in struct from member function
    position = engine.find_declaration_of("find_variable_declaration.cc", { 9, 4 });
    if (!position.has_value())
        FAIL("declaration not found (3)");

    if (position.value().file != "sample_header.hh" || position.value().line != 8)
        FAIL("wrong declaration location (3)");;

    PASS;
}

void test_namespace()
{
    I_TEST(Find Variable Declaration)
    LocalFileDB filedb;
    add_file(filedb, "find_symbol_in_namespace.cc");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);

    // Find local variable
    auto position = engine.find_declaration_of("find_symbol_in_namespace.cc", { 8, 7 });
    if (!position.has_value())
        FAIL("declaration not found (1)");


    if (position.value().file != "find_symbol_in_namespace.cc" || position.value().line != 3 || position.value().column != 4)
        FAIL("wrong declaration location (1)");

    position = engine.find_declaration_of("find_symbol_in_namespace.cc", { 13, 8 });
    if (!position.has_value())
        FAIL("declaration not found (2)");


    if (position.value().file != "find_symbol_in_namespace.cc" || position.value().line != 3 || position.value().column != 4)
        FAIL("wrong declaration location (2)");

    PASS;
}

void test_find_array_variable_declaration_single()
{
    I_TEST(Find 1D Array as a Variable Declaration)
    LocalFileDB filedb;
    auto filename = "find_array_variable_declaration.cc";
    add_file(filedb, filename);
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto position = engine.find_declaration_of(filename, { 3, 6 });
    if (!position.has_value())
        FAIL("declaration not found");

    if (position.value().file == filename && position.value().line == 2 && position.value().column >= 4)
        PASS;

    printf("Found at position %zu %zu\n", position.value().line, position.value().column);
    FAIL("wrong declaration location");
}



void test_find_array_variable_declaration_single_empty()
{
    I_TEST(Find 1D Empty size Array as a Variable Declaration)
    LocalFileDB filedb;
    auto filename = "find_array_variable_declaration.cc";
    add_file(filedb, filename);
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto position = engine.find_declaration_of(filename, { 6, 6 });
    if (!position.has_value())
        FAIL("declaration not found");

    if (position.value().file == filename && position.value().line == 5 && position.value().column >= 4)
        PASS;

    printf("Found at position %zu %zu\n", position.value().line, position.value().column);
    FAIL("wrong declaration location");
}

void test_find_array_variable_declaration_double()
{
    I_TEST(Find 2D Array as a Variable Declaration)
    LocalFileDB filedb;
    auto filename = "find_array_variable_declaration.cc";
    add_file(filedb, filename);
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto position = engine.find_declaration_of(filename, { 9, 6 });
    if (!position.has_value())
        FAIL("declaration not found");

    if (position.value().file == filename && position.value().line == 8 && position.value().column >= 4)
        PASS;

    printf("Found at position %zu %zu\n", position.value().line, position.value().column);
    FAIL("wrong declaration location");
}

void test_complete_includes()
{
    I_TEST("Complete include statements")
    LocalFileDB filedb;
    filedb.set_project_root(TESTS_ROOT_DIR);
    add_file(filedb, "complete_includes.cc");
    add_file(filedb, "sample_header.hh");
    CodeComprehension::Cpp::CppComprehensionEngine autocomplete(filedb);

    auto suggestions = autocomplete.get_suggestions("complete_includes.cc", { 0, 22 });
    if (suggestions.size() != 1)
        FAIL("project include - bad size");

    if (suggestions[0].completion != "\"sample_header.hh\"")
        FAIL("project include - wrong results");

    suggestions = autocomplete.get_suggestions("complete_includes.cc", { 1, 18 });
    if (suggestions.size() != 1)
        FAIL("global include - bad size");

    if (suggestions[0].completion != "<sys/asoundlib.h>")
        FAIL("global include - wrong results");

    PASS;
}

void test_parameters_hint()
{
    I_TEST("Function Parameters hint")
    LocalFileDB filedb;
    filedb.set_project_root(TESTS_ROOT_DIR);
    add_file(filedb, "parameters_hint1.cc");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);

    auto result = engine.get_function_params_hint("parameters_hint1.cc", { 4, 9 });
    if (!result.has_value())
        FAIL("failed to get parameters hint (1)");
    if (result->params != std::vector<std::string> { "int x", "char y" } || result->current_index != 0)
        FAIL("bad result (1)");

    result = engine.get_function_params_hint("parameters_hint1.cc", { 5, 15 });
    if (!result.has_value())
        FAIL("failed to get parameters hint (2)");
    if (result->params != std::vector<std::string> { "int x", "char y" } || result->current_index != 1)
        FAIL("bad result (2)");

    result = engine.get_function_params_hint("parameters_hint1.cc", { 6, 8 });
    if (!result.has_value())
        FAIL("failed to get parameters hint (3)");
    if (result->params != std::vector<std::string> { "int x", "char y" } || result->current_index != 0)
        FAIL("bad result (3)");

    PASS;
}

void test_ast_cpp() {
    I_TEST("Find Variable Declaration in AST.cpp")
    auto filename = "AST.cpp";
    LocalFileDB filedb;
    add_file(filedb, filename);
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto tokens_info = engine.get_tokens_info(filename);
    auto position = engine.find_declaration_of(filename, { 99, 13 });
    if (!position.has_value())
        FAIL("declaration not found");

    if (position.value().file != filename || position.value().line != 96 || position.value().column < 4)
        FAIL("wrong declaration location");

    PASS;
}

void test_parser_cpp() {
    I_TEST("Find Variable Declaration in Parser.cpp")


    auto filename = "Parser.cpp";
    LocalFileDB filedb;
    add_file(filedb, filename);
    add_file(filedb, "Parser.h");
    CodeComprehension::Cpp::CppComprehensionEngine engine(filedb);
    auto position = engine.find_declaration_of(filename, { 34, 42 });
    if (!position.has_value())
        FAIL("declaration not found");

    dbgln("{} {} {}", position.value().file, position.value().line, position.value().column);
    if (position.value().file != "Parser.h" || position.value().line != 195 || position.value().column != 4)
        FAIL("wrong declaration location");

    PASS;

/*
    auto c_string = read_all(std::filesystem::path{TESTS_ROOT_DIR} / "Parser.cpp");
    ::Cpp::Preprocessor preprocessor("test.cc", c_string);
    auto results = preprocessor.process_and_lex();
    ::Cpp::Parser parser(results, "test.cc");
    auto translation_unit = parser.parse();

    FILE* debug_output = fopen("debug_output.txt", "w");
    translation_unit->dump(debug_output);
    fclose(debug_output);
    */
}

std::string read_first_line(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Error opening file: {}", filePath));
    }

    std::string line;
    std::getline(file, line);

    return line;
}

int main(int argc, char* argv[]) {
    TESTS_ROOT_DIR = read_first_line("project_source_dir.txt") + "/test";

    test_complete_local_args();
    test_complete_local_vars();
    test_complete_type();
    test_find_function_declaration();
    test_find_variable_definition();
    test_namespace();
    test_find_array_variable_declaration_single();
    test_find_array_variable_declaration_single_empty();
    test_find_array_variable_declaration_double();
    test_complete_includes();
    test_parameters_hint();
    test_ast_cpp();
    test_parser_cpp();


    return 0;
}