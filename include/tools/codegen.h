#pragma once
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>

#include "tools/fnv.h"

namespace codegen {
    constexpr char kTabSym = ' ';
    constexpr std::size_t kTabsPerBlock = 2; // @note: @es3n1n: how many characters shall we place per each block
    constexpr std::initializer_list<char> kBlacklistedCharacters = {':', ';', '\\', '/'};

    struct generator_t {
        using self_ref = std::add_lvalue_reference_t<generator_t>;
    public:
        constexpr generator_t() = default;
        constexpr ~generator_t() = default;
        constexpr self_ref operator=(self_ref v) {
            return v;
        }
    public:
        self_ref begin_json_object() {
            push_line("{");
            inc_tabs_count(kTabsPerBlock);
            return *this;
        }

        self_ref begin_json_object_value() {
            _stream << "{" << std::endl;
            inc_tabs_count(kTabsPerBlock);
            return *this;
        }

        self_ref end_json_object(bool write_comma = true) {
            dec_tabs_count(kTabsPerBlock);
            if (write_comma)
                push_line("},");
            else
                push_line("}");
            return *this;
        }

        self_ref begin_json_array() {
            push_line("[");
            inc_tabs_count(kTabsPerBlock);
            return *this;
        }

        self_ref begin_json_array_value() {
            _stream << "[" << std::endl;
            inc_tabs_count(kTabsPerBlock);
            return *this;
        }

        self_ref end_json_array() {
            dec_tabs_count(kTabsPerBlock);
            push_line("],");
            return *this;
        }

        self_ref comment(const std::string& text) {
            return push_line(std::format("// {}", text));
        }

        self_ref json_key(const std::string& str) {
            return push_line("\"" + escape_json_string(str) + "\": ", false);
        }

        self_ref json_string(const std::string& str) {
            _stream << "\"" + escape_json_string(str) + "\"," << std::endl;
            return *this;
        }

        template <typename T>
        self_ref json_literal(T value) {
            _stream << std::format("{},", value) << std::endl;
            return *this;
        }

    public:
        [[nodiscard]] std::string str() {
            return _stream.str();
        }

        self_ref push_line(const std::string& line, bool move_cursor_to_next_line = true) {
            for (std::size_t i = 0; i < _tabs_count; i++)
                _stream << kTabSym;
            _stream << line;
            if (move_cursor_to_next_line)
                _stream << std::endl;
            return *this;
        }
    private:
        std::string escape_json_string(const std::string& input) {
            std::ostringstream ss;
            for (auto c : input) {
                switch (c) {
                case '\\':
                    ss << "\\\\";
                    break;
                case '"':
                    ss << "\\\"";
                    break;
                case '/':
                    ss << "\\/";
                    break;
                case '\b':
                    ss << "\\b";
                    break;
                case '\f':
                    ss << "\\f";
                    break;
                case '\n':
                    ss << "\\n";
                    break;
                case '\r':
                    ss << "\\r";
                    break;
                case '\t':
                    ss << "\\t";
                    break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        ss << "\\u" << std::hex << (int)c;
                    } else {
                        ss << c;
                    }
                }
            }
            return ss.str();
        }

        std::string escape_name(const std::string& name) {
            std::string result;
            result.resize(name.size());

            for (std::size_t i = 0; i < name.size(); i++)
                result[i] =
                    std::find(kBlacklistedCharacters.begin(), kBlacklistedCharacters.end(), name[i]) == std::end(kBlacklistedCharacters) ? name[i] : '_';

            return result;
        }
    public:
        self_ref inc_tabs_count(std::size_t count = 1) {
            _tabs_count_backup = _tabs_count;
            _tabs_count += count;
            return *this;
        }

        self_ref dec_tabs_count(std::size_t count = 1) {
            _tabs_count_backup = _tabs_count;
            if (_tabs_count)
                _tabs_count -= count;
            return *this;
        }

        self_ref restore_tabs_count() {
            _tabs_count = _tabs_count_backup;
            return *this;
        }

        self_ref reset_tabs_count() {
            _tabs_count_backup = _tabs_count;
            _tabs_count = 0;
            return *this;
        }
    private:
        std::stringstream _stream = {};
        std::size_t _tabs_count = 0, _tabs_count_backup = 0;
        std::size_t _unions_count = 0;
        std::size_t _pads_count = 0;
        std::set<fnv32::hash> _forward_decls = {};
    };

    __forceinline generator_t get() {
        return generator_t{};
    }
} // namespace codegen
