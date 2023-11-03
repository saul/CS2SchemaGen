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

    // @note: @es3n1n: a list of possible integral types for bitfields (would be used in `guess_bitfield_type`)
    //
    // clang-format off
    constexpr std::initializer_list<std::pair<std::size_t, std::string_view>> kBitfieldIntegralTypes = {
        {8, "uint8_t"},
        {16, "uint16_t"},
        {32, "uint32_t"},
        {64, "uint64_t"},

        // @todo: @es3n1n: define uint128_t/uint256_t/... as custom structs in the very beginning of the file
        {128, "uint128_t"},
        {256, "uint256_t"},
        {512, "uint512_t"},
    };
    // clang-format on

    inline std::string guess_bitfield_type(const std::size_t bits_count) {
        for (auto p : kBitfieldIntegralTypes) {
            if (bits_count > p.first)
                continue;

            return p.second.data();
        }

        throw std::runtime_error(std::format("{} : Unable to guess bitfield type with size {}", __FUNCTION__, bits_count));
    }

    struct generator_t {
        using self_ref = std::add_lvalue_reference_t<generator_t>;
    public:
        constexpr generator_t() = default;
        constexpr ~generator_t() = default;
        constexpr self_ref operator=(self_ref v) {
            return v;
        }
    public:
        self_ref next_line() {
            return push_line("");
        }

        self_ref begin_json_object() {
            return begin_block("");
        }

        self_ref end_json_object() {
            return end_block();
        }

        self_ref begin_json_array(bool increment_tabs_count = true, bool move_cursor_to_next_line = true) {
            // @note: @es3n1n: we should reset tabs count if we aren't moving cursor to
            // the next line
            auto backup_tabs_count = _tabs_count;
            if (!move_cursor_to_next_line)
                _tabs_count = 0;

            push_line("[", move_cursor_to_next_line);

            // @note: @es3n1n: restore tabs count
            if (!move_cursor_to_next_line)
                _tabs_count = backup_tabs_count;

            if (increment_tabs_count)
                inc_tabs_count(kTabsPerBlock);

            return *this;
        }

        self_ref end_json_array(bool decrement_tabs_count = true, bool move_cursor_to_next_line = true) {
            if (decrement_tabs_count)
                dec_tabs_count(kTabsPerBlock);

            push_line("],");
            if (move_cursor_to_next_line)
                next_line();

            return *this;
        }

        self_ref begin_block(const std::string& text, bool increment_tabs_count = true,
                             bool move_cursor_to_next_line = true) {
            push_line(text, move_cursor_to_next_line);

            // @note: @es3n1n: we should reset tabs count if we aren't moving cursor to
            // the next line
            auto backup_tabs_count = _tabs_count;
            if (!move_cursor_to_next_line)
                _tabs_count = 0;

            push_line("{", move_cursor_to_next_line);

            // @note: @es3n1n: restore tabs count
            if (!move_cursor_to_next_line)
                _tabs_count = backup_tabs_count;

            if (increment_tabs_count)
                inc_tabs_count(kTabsPerBlock);

            return *this;
        }

        self_ref end_block(bool decrement_tabs_count = true, bool move_cursor_to_next_line = true) {
            if (decrement_tabs_count)
                dec_tabs_count(kTabsPerBlock);

            push_line("},");
            if (move_cursor_to_next_line)
                next_line();

            return *this;
        }

        self_ref begin_class(const std::string& class_name) {
            return begin_block(std::format("public partial class {}", class_name));
        }

        self_ref begin_class_with_base_type(const std::string& class_name, const std::string& base_type) {
            if (base_type.empty())
                return begin_class(std::cref(class_name));

            return begin_block(std::format("public partial class {} : {}", class_name, base_type));
        }

        self_ref end_class() {
            return end_block();
        }

        self_ref push_using(const std::string& namespace_name) {
            return push_line(std::format("using {};", namespace_name));
        }

        self_ref push_namespace(const std::string& namespace_name) {
            return push_line(std::format("namespace {};", namespace_name))
                .next_line();
        }

        self_ref begin_enum(const std::string& enum_name, const std::string& base_typename = "") {
            return begin_block(std::format("public enum {}{}", escape_name(enum_name), base_typename.empty() ? base_typename : (" : " + base_typename)));
        }

        self_ref end_enum() {
            return end_block();
        }

        template <typename T>
        self_ref enum_item(const std::string& name, T value) {
            return push_line(std::vformat(sizeof(T) >= 4 ? "{} = {:#x}," : "{} = {},", std::make_format_args(name, value)));
        }

        self_ref comment(const std::string& text, const bool move_cursor_to_next_line = true) {
            return push_line(std::format("// {}", text), move_cursor_to_next_line);
        }

        self_ref prop(const std::string& type_name, const std::string& name) {
            return push_line(std::format("public {} {} {{ get; internal set; }}", type_name, name));
        }

        self_ref json_key(const std::string& str) {
            return push_line("\"" + escape_json_string(str) + "\": ", false);
        }

        self_ref json_string(const std::string& str) {
            return push_line("\"" + escape_json_string(str) + "\",");
        }

        template <typename T>
        self_ref json_literal(T value) {
            return push_line(std::format("{},", value));
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
