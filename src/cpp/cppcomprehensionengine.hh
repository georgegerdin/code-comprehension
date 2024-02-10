/*
 * Copyright (c) 2021-2022, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_set>
#include <memory>

#include "../filedb.hh"
#include "cpp_parser/ast.hh"
#include "cpp_parser/parser.hh"
#include "cpp_parser/preprocessor.hh"
#include "cpp_parser/intrusive_ptr.hh"
#include "../codecomprehensionengine.hh"

namespace CodeComprehension::Cpp {

using namespace ::Cpp;

class CppComprehensionEngine : public CodeComprehensionEngine {
public:
    CppComprehensionEngine(FileDB const& filedb);

    virtual std::vector<CodeComprehension::AutocompleteResultEntry> get_suggestions(std::string const& file, GUI::TextPosition const& autocomplete_position) override;
    virtual void on_edit(std::string const& file) override;
    virtual void file_opened([[maybe_unused]] std::string const& file) override;
    virtual std::optional<CodeComprehension::ProjectLocation> find_declaration_of(std::string const& filename, GUI::TextPosition const& identifier_position) override;
    virtual std::optional<FunctionParamsHint> get_function_params_hint(std::string const&, GUI::TextPosition const&) override;
    virtual std::vector<CodeComprehension::TokenInfo> get_tokens_info(std::string const& filename) override;

private:
    struct SymbolName {
        std::string_view name;
        std::vector<std::string_view> scope;

        static SymbolName create(std::string_view, std::vector<std::string_view>&&);
        static SymbolName create(std::string_view);
        std::string scope_as_string() const;
        std::string to_byte_string() const;

        bool operator==(SymbolName const&) const = default;
    };

    static constexpr unsigned int_hash(uint32_t key)
    {
        key += ~(key << 15);
        key ^= (key >> 10);
        key += (key << 3);
        key ^= (key >> 6);
        key += ~(key << 11);
        key ^= (key >> 16);
        return key;
    }

    static constexpr unsigned pair_int_hash(uint32_t key1, uint32_t key2)
    {
        return int_hash((int_hash(key1) * 209) ^ (int_hash(key2 * 413)));
    }

    static constexpr uint32_t string_hash(char const* characters, size_t length, uint32_t seed = 0)
    {
        uint32_t hash = seed;
        for (size_t i = 0; i < length; ++i) {
            hash += static_cast<uint32_t>(characters[i]);
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
        return hash;
    }

    class KeySymbolHash {
    public:
        size_t operator()(CodeComprehension::Cpp::CppComprehensionEngine::SymbolName const& key) const {
            unsigned hash = 0;
            hash = pair_int_hash(hash, string_hash(key.name.data(), key.name.length()));
            for (auto& scope_part : key.scope) {
                hash = pair_int_hash(hash, string_hash(scope_part.data(), scope_part.length()));
            }
            return hash;
        }
    };
    struct Symbol {
        SymbolName name;
        intrusive_ptr<Cpp::Declaration const> declaration;

        // Local symbols are symbols that should not appear in a global symbol search.
        // For example, a variable that is declared inside a function will have is_local = true.
        bool is_local { false };

        enum class IsLocal {
            No,
            Yes
        };
        static Symbol create(std::string_view name, std::vector<std::string_view> const& scope, intrusive_ptr<Cpp::Declaration const>, IsLocal is_local);
    };

    //friend Traits<SymbolName>;

    struct DocumentData {
        std::string const& filename() const { return m_filename; }
        std::string const& text() const { return m_text; }
        Preprocessor const& preprocessor() const
        {
            assert(m_preprocessor);
            return *m_preprocessor;
        }
        Preprocessor& preprocessor()
        {
            assert(m_preprocessor);
            return *m_preprocessor;
        }
        Parser const& parser() const
        {
            assert(m_parser);
            return *m_parser;
        }
        Parser& parser()
        {
            assert(m_parser);
            return *m_parser;
        }

        std::string m_filename;
        std::string m_text;
        std::unique_ptr<Preprocessor> m_preprocessor;
        std::unique_ptr<Parser> m_parser;

        std::unordered_map<SymbolName, Symbol, KeySymbolHash> m_symbols;
        std::unordered_set<std::string> m_available_headers;
    };

    std::vector<CodeComprehension::AutocompleteResultEntry> autocomplete_property(DocumentData const&, MemberExpression const&, const std::string partial_text) const;
    std::vector<AutocompleteResultEntry> autocomplete_name(DocumentData const&, ASTNode const&, std::string const& partial_text) const;
    std::string type_of(DocumentData const&, Expression const&) const;
    std::string type_of_property(DocumentData const&, Identifier const&) const;
    std::string type_of_variable(Identifier const&) const;
    bool is_property(ASTNode const&) const;
    intrusive_ptr<Cpp::Declaration const> find_declaration_of(DocumentData const&, ASTNode const&) const;
    intrusive_ptr<Cpp::Declaration const> find_declaration_of(DocumentData const&, SymbolName const&) const;
    intrusive_ptr<Cpp::Declaration const> find_declaration_of(DocumentData const&, const GUI::TextPosition& identifier_position);

    enum class RecurseIntoScopes {
        No,
        Yes
    };

    std::vector<Symbol> properties_of_type(DocumentData const& document, std::string const& type) const;
    std::vector<Symbol> get_child_symbols(ASTNode const&) const;
    std::vector<Symbol> get_child_symbols(ASTNode const&, std::vector<std::string_view> const& scope, Symbol::IsLocal) const;

    DocumentData const* get_document_data(std::string const& file) const;
    DocumentData const* get_or_create_document_data(std::string const& file);
    void set_document_data(std::string const& file, std::unique_ptr<DocumentData>&& data);

    std::unique_ptr<DocumentData> create_document_data_for(std::string const& file);
    std::string document_path_from_include_path(std::string_view include_path) const;
    void update_declared_symbols(DocumentData&);
    void update_todo_entries(DocumentData&);
    CodeComprehension::DeclarationType type_of_declaration(Cpp::Declaration const&);
    std::vector<std::string_view> scope_of_node(ASTNode const&) const;
    std::vector<std::string_view> scope_of_reference_to_symbol(ASTNode const&) const;

    std::optional<CodeComprehension::ProjectLocation> find_preprocessor_definition(DocumentData const&, const GUI::TextPosition&);
    std::optional<Cpp::Preprocessor::Substitution> find_preprocessor_substitution(DocumentData const&, Cpp::Position const&);

    std::unique_ptr<DocumentData> create_document_data(std::string text, std::string const& filename);
    std::optional<std::vector<CodeComprehension::AutocompleteResultEntry>> try_autocomplete_property(DocumentData const&, ASTNode const&, std::optional<Token> containing_token) const;
    std::optional<std::vector<CodeComprehension::AutocompleteResultEntry>> try_autocomplete_name(DocumentData const&, ASTNode const&, std::optional<Token> containing_token) const;
    std::optional<std::vector<CodeComprehension::AutocompleteResultEntry>> try_autocomplete_include(DocumentData const&, Token include_path_token, Cpp::Position const& cursor_position) const;
    static bool is_symbol_available(Symbol const&, std::vector<std::string_view> const& current_scope, std::vector<std::string_view> const& reference_scope);
    std::optional<FunctionParamsHint> get_function_params_hint(DocumentData const&, FunctionCall const&, size_t argument_index);

    template<typename Func>
    void for_each_available_symbol(DocumentData const&, Func) const;

    template<typename Func>
    void for_each_included_document_recursive(DocumentData const&, Func) const;

    CodeComprehension::TokenInfo::SemanticType get_token_semantic_type(DocumentData const&, Token const&);
    CodeComprehension::TokenInfo::SemanticType get_semantic_type_for_identifier(DocumentData const&, Position);

    std::unordered_map<std::string, std::unique_ptr<DocumentData>> m_documents;

    // A document's path will be in this set if we're currently processing it.
    // A document is added to this set when we start processing it (e.g because it was #included) and removed when we're done.
    // We use this to prevent circular #includes from looping indefinitely.
    std::unordered_set<std::string> m_unfinished_documents;
};

enum IterationDecision {
    Break,
    Continue
};

template<typename Func>
void CppComprehensionEngine::for_each_available_symbol(DocumentData const& document, Func func) const
{
    for (auto& item : document.m_symbols) {
        auto decision = func(item.second);
        if (decision == IterationDecision::Break)
            return;
    }

    for_each_included_document_recursive(document, [&](DocumentData const& document) {
        for (auto& item : document.m_symbols) {
            auto decision = func(item.second);
            if (decision == IterationDecision::Break)
                return IterationDecision::Break;
        }
        return IterationDecision::Continue;
    });
}

template<typename Func>
void CppComprehensionEngine::for_each_included_document_recursive(DocumentData const& document, Func func) const
{
    for (auto& included_path : document.m_available_headers) {
        auto* included_document = get_document_data(included_path);
        if (!included_document)
            continue;
        auto decision = func(*included_document);
        if (decision == IterationDecision::Break)
            continue;
    }
}
}

/*
namespace AK {

template<>
struct Traits<CodeComprehension::Cpp::CppComprehensionEngine::SymbolName> : public DefaultTraits<CodeComprehension::Cpp::CppComprehensionEngine::SymbolName> {
    static unsigned hash(CodeComprehension::Cpp::CppComprehensionEngine::SymbolName const& key)
    {
        unsigned hash = 0;
        hash = pair_int_hash(hash, string_hash(key.name.characters_without_null_termination(), key.name.length()));
        for (auto& scope_part : key.scope) {
            hash = pair_int_hash(hash, string_hash(scope_part.characters_without_null_termination(), scope_part.length()));
        }
        return hash;
    }
};

}
*/


