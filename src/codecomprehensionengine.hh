/*
 * Copyright (c) 2021, Itamar S. <itamar8910@gmail.com>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>

#include "filedb.hh"
#include "types.hh"
#include "cpp_parser/parser.hh"

namespace GUI {
    struct TextPosition {
        size_t line_, column_;
        size_t line() const {return line_;}
        size_t column() const {return column_;}
    };
}

namespace CodeComprehension {

class CodeComprehensionEngine {
    CodeComprehensionEngine(CodeComprehensionEngine const&) = delete;
    CodeComprehensionEngine(CodeComprehensionEngine&&) = delete;

public:
    CodeComprehensionEngine(FileDB const& filedb, bool store_all_declarations = false);
    virtual ~CodeComprehensionEngine() = default;

    virtual std::vector<AutocompleteResultEntry> get_suggestions(std::string const& file, GUI::TextPosition const& autocomplete_position) = 0;

    // TODO: In the future we can pass the range that was edited and only re-parse what we have to.
    virtual void on_edit([[maybe_unused]] std::string const& file) {};
    virtual void file_opened([[maybe_unused]] std::string const& file) {};

    virtual std::optional<ProjectLocation> find_declaration_of(std::string const&, GUI::TextPosition const&) { return {}; }

    struct FunctionParamsHint {
        std::vector<std::string> params;
        size_t current_index { 0 };
    };
    virtual std::optional<FunctionParamsHint> get_function_params_hint(std::string const&, GUI::TextPosition const&) { return {}; }

    virtual std::vector<TokenInfo> get_tokens_info(std::string const&) { return {}; }

    std::function<void(std::string const&, std::vector<Declaration>&&)> set_declarations_of_document_callback;
    std::function<void(std::string const&, std::vector<TodoEntry>&&)> set_todo_entries_of_document_callback;

protected:
    FileDB const& filedb() const { return m_filedb; }
    void set_declarations_of_document(std::string const&, std::vector<Declaration>&&);
    void set_todo_entries_of_document(std::string const&, std::vector<TodoEntry>&&);
    std::unordered_map<std::string, std::vector<Declaration>> const& all_declarations() const { return m_all_declarations; }

private:
    std::unordered_map<std::string, std::vector<Declaration>> m_all_declarations;
    FileDB const& m_filedb;
    bool m_store_all_declarations { false };
};
}
