#include "sdk/sdk.h"
#include <filesystem>
#include <set>
#include <string_view>

namespace {
    using namespace std::string_view_literals;

    constexpr std::initializer_list<fnv32::hash> kStringMetadataEntries = {
        FNV32("MNetworkChangeCallback"),  FNV32("MPropertyFriendlyName"), FNV32("MPropertyDescription"),
        FNV32("MPropertyAttributeRange"), FNV32("MPropertyStartGroup"),   FNV32("MPropertyAttributeChoiceName"),
        FNV32("MPropertyGroupName"),      FNV32("MNetworkUserGroup"),     FNV32("MNetworkAlias"),
        FNV32("MNetworkTypeAlias"),       FNV32("MNetworkSerializer"),    FNV32("MPropertyAttributeEditor"),
        FNV32("MPropertySuppressExpr"),   FNV32("MKV3TransferName"),      FNV32("MNetworkEncoder"),
        FNV32("MNetworkSendProxyRecipientsFilter")
    };

    constexpr std::initializer_list<fnv32::hash> kIntegerMetadataEntries = {
        FNV32("MNetworkVarEmbeddedFieldOffsetDelta"),
        FNV32("MNetworkBitCount"),
        FNV32("MNetworkPriority"),
        FNV32("MPropertySortPriority"),
        FNV32("MNetworkEncodeFlags")
    };

    constexpr std::initializer_list<fnv32::hash> kFloatMetadataEntries = {
        FNV32("MNetworkMinValue"),
        FNV32("MNetworkMaxValue"),
    };

    inline bool ends_with(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }
} // namespace

namespace sdk {
    namespace {
        void AssembleEnums(codegen::generator_t::self_ref builder, CUtlTSHash<CSchemaEnumBinding*> enums) {
            builder.json_key("enums").begin_json_object_value();

            for (auto schema_enum_binding : enums.GetElements()) {
                // @note: @es3n1n: get type name by align size
                //
                const auto get_type_name = [schema_enum_binding]() [[msvc::forceinline]] {
                    std::string type_storage;

                    switch (schema_enum_binding->m_align_) {
                    case 1:
                        type_storage = "byte";
                        break;
                    case 2:
                        type_storage = "ushort";
                        break;
                    case 4:
                        type_storage = "uint";
                        break;
                    case 8:
                        type_storage = "ulong";
                        break;
                    default:
                        type_storage = "INVALID_TYPE";
                    }

                    return type_storage;
                };

                // @todo: @es3n1n: assemble flags
                //
                // if (schema_enum_binding->m_flags_) out.print("// Flags: MEnumFlagsWithOverlappingBits\n");

                // @note: @es3n1n: begin enum class
                //
                builder.json_key(schema_enum_binding->m_binding_name_).begin_json_object_value();

                builder.json_key("align").json_literal(schema_enum_binding->m_align_);

                // @note: @es3n1n: assemble enum items
                //
                builder.json_key("items").begin_json_array_value();
                for (auto l = 0; l < schema_enum_binding->m_size_; l++) {
                    auto& field = schema_enum_binding->m_enum_info_[l];

                    builder.begin_json_object()
                        .json_key("name")
                        .json_string(field.m_name)
                        .json_key("value")
                        .json_literal(field.m_value == std::numeric_limits<std::size_t>::max() ? -1 : field.m_value)
                        .end_json_object();
                }
                builder.end_json_array();

                // @note: @es3n1n: we are done with this enum
                //
                builder.end_json_object();
            }

            builder.end_json_object();
        }

        void WriteTypeJson(codegen::generator_t::self_ref builder, CSchemaType* current_type) {
            builder.begin_json_object_value().json_key("name").json_string(current_type->m_name_);
            
            builder.json_key("category").json_literal(current_type->type_category);

            if (current_type->type_category == Schema_Atomic) {
                builder.json_key("atomic").json_literal(current_type->atomic_category);

                if (current_type->atomic_category == Atomic_T && current_type->m_atomic_t_.generic_type != nullptr) {
                    builder.json_key("outer").json_string(current_type->m_atomic_t_.generic_type->m_name_);
                }

                if (current_type->atomic_category == Atomic_T || current_type->atomic_category == Atomic_CollectionOfT) {
                    builder.json_key("inner");
                    WriteTypeJson(builder, current_type->m_atomic_t_.template_type);
                }
            } else if (current_type->type_category == Schema_FixedArray) {
                builder.json_key("arraySize").json_literal(current_type->m_array_.array_size);
                builder.json_key("inner");
                WriteTypeJson(builder, current_type->m_array_.element_type_);
            } else if (current_type->type_category == Schema_Ptr) {
                builder.json_key("inner");
                WriteTypeJson(builder, current_type->m_schema_type_);
            }

            builder.end_json_object();
        }

        void AssembleClasses(CSchemaSystemTypeScope* current, codegen::generator_t::self_ref builder, CUtlTSHash<CSchemaClassBinding*> classes) {
            struct class_t {
                CSchemaClassInfo* target_;
                std::set<CSchemaClassInfo*> refs_;

                CSchemaClassInfo* GetParent() {
                    if (!target_->m_schema_parent)
                        return nullptr;

                    return target_->m_schema_parent->m_class;
                }

                void AddRefToClass(CSchemaType* type) {
                    if (type->type_category == Schema_DeclaredClass) {
                        refs_.insert(type->m_class_info);
                        return;
                    }

                    // auto ptr = type->GetRefClass();
                    // if (ptr && ptr->type_category == Schema_DeclaredClass)
                    // {
                    // 	refs_.insert(ptr->m_class_info);
                    // 	return;
                    // }
                }

                bool IsDependsOn(const class_t& other) {
                    // if current class inherit other.
                    auto parent = this->GetParent();
                    if (parent == other.target_)
                        return true;

                    // if current class contains ref to other.
                    if (this->refs_.contains(other.target_))
                        return true;

                    // otherwise, order doesn`t matter.
                    return false;
                }

                SchemaClassFieldData_t* GetFirstField() {
                    if (target_->m_align)
                        return &target_->m_fields[0];
                    return nullptr;
                }

                // @note: @es3n1n: Returns the struct size without its parent's size
                //
                std::ptrdiff_t ClassSizeWithoutParent() {
                    if (CSchemaClassInfo* class_parent = this->GetParent(); class_parent)
                        return this->target_->m_size - class_parent->m_size;
                    return this->target_->m_size;
                }
            };

            // @note: @soufiw:
            // sort all classes based on refs and inherit, and then print it.
            // ==================
            std::list<class_t> classes_to_dump;

            for (const auto schema_class_binding : classes.GetElements()) {
                const auto class_info = current->FindDeclaredClass(schema_class_binding->m_binary_name);

                auto& class_dump = classes_to_dump.emplace_back();
                class_dump.target_ = class_info;

                for (auto k = 0; k < class_info->m_align; k++) {
                    const auto field = &class_info->m_fields[k];
                    if (!field)
                        continue;

                    // forward declare all classes.
                    // @todo: maybe we need to forward declare only pointers to classes?
                    auto ptr = field->m_type->GetRefClass();
                    auto actual_type = ptr ? ptr : field->m_type;

                    class_dump.AddRefToClass(field->m_type);
                }
            }

            bool did_change = false;
            do {
                did_change = false;

                // swap until we done.
                for (auto first = classes_to_dump.begin(); first != classes_to_dump.end(); ++first) {
                    bool second_below_first = false;

                    for (auto second = classes_to_dump.begin(); second != classes_to_dump.end(); ++second) {
                        if (second == first) {
                            second_below_first = true;
                            continue;
                        }

                        // swap if second class below first, and first depends on second.
                        bool first_depend = first->IsDependsOn(*second);

                        // swap if first class below second, and second depends on first.
                        bool second_depend = second->IsDependsOn(*first);

                        if (first_depend && second_depend) {
                            // classes depends on each other, forward declare them.
                            // @todo: verify that cyclic dependencies is a pointers.
                            continue;
                        }

                        bool swap = second_below_first ? first_depend : second_depend;
                        if (swap) {
                            std::iter_swap(first, second);
                            did_change = true;
                        }
                    }
                }
            } while (did_change);
            // ==================

            builder.json_key("classes").begin_json_object_value();

            for (auto& class_dump : classes_to_dump) {
                // @note: @es3n1n: get class info, assemble it
                //
                const auto class_parent = class_dump.GetParent();
                const auto class_info = class_dump.target_;

                // @note: @es3n1n: get parent name
                //
                std::string parent_cls_name;
                if (auto parent = class_info->m_schema_parent ? class_info->m_schema_parent->m_class : nullptr; parent) {
                    parent_cls_name = parent->m_name;
                }

                // @note: @es3n1n: start class
                //
                builder.json_key(class_info->m_name).begin_json_object_value();

                if (!parent_cls_name.empty())
                    builder.json_key("parent").json_string(parent_cls_name);

                builder.json_key("fields").begin_json_array_value();

                // @note: @es3n1n: begin public members
                //
                for (auto k = 0; k < class_info->m_align; k++) {
                    const auto field = &class_info->m_fields[k];
                    if (!field)
                        continue;

                    // @note: @es3n1n: some more utils
                    //
                    auto get_metadata_type = [&](SchemaMetadataEntryData_t metadata_entry) -> std::string {
                        std::string value;

                        const auto value_hash_name = fnv32::hash_runtime(metadata_entry.m_name);

                        // clang-format off
                        if (std::find(kStringMetadataEntries.begin(), kStringMetadataEntries.end(), value_hash_name) != kStringMetadataEntries.end() && metadata_entry.m_value->m_sz_value != nullptr)
                            value = metadata_entry.m_value->m_sz_value;
                        else if (std::find(kIntegerMetadataEntries.begin(), kIntegerMetadataEntries.end(), value_hash_name) != kIntegerMetadataEntries.end())
                            value = std::to_string(metadata_entry.m_value->m_n_value);
                        else if (std::find(kFloatMetadataEntries.begin(), kFloatMetadataEntries.end(), value_hash_name) != kFloatMetadataEntries.end())
                            value = std::to_string(metadata_entry.m_value->m_f_value);
                        // clang-format on

                        return value;
                    };

                    // @note: @es3n1n: obtaining size
                    //
                    int field_size = 0;
                    if (!field->m_type->GetSize(&field_size)) // @note: @es3n1n: should happen if we are attempting to get a size of the bitfield
                        field_size = 0;

                    // @note: @es3n1n: dump metadata
                    //
                    auto is_network_enable = strcmp(class_info->m_name, "ServerAuthoritativeWeaponSlot_t") == 0;
                    for (auto j = 0; j < field->m_metadata_size; j++) {
                        auto field_metadata = field->m_metadata[j];

                        if (strcmp(field_metadata.m_name, "MNetworkDisable") == 0) {
                            is_network_enable = false;
                            break;
                        }

                        if (strstr(field_metadata.m_name, "MNetwork") == field_metadata.m_name) {
                            is_network_enable = true;
                            break;
                        }
                    }

                    if (!is_network_enable) {
                        continue;
                    }

                    builder.begin_json_object().json_key("name").json_string(field->m_name);

                    builder.json_key("type");

                    WriteTypeJson(builder, field->m_type);

                    builder.json_key("metadata").begin_json_object_value();

                    for (auto j = 0; j < field->m_metadata_size; j++) {
                        auto field_metadata = field->m_metadata[j];

                        if (strcmp(field_metadata.m_name, "MNetworkEnable") == 0)
                            continue;

                        auto data = get_metadata_type(field_metadata);
                        builder.json_key(field_metadata.m_name).json_string(data);
                    }

                    builder.end_json_object();

                    builder.end_json_object();
                }

                builder.end_json_array();

                builder.end_json_object();
            }

            builder.end_json_object();
        }
    } // namespace

    void GenerateTypeScopeSdk(CSchemaSystemTypeScope* current, const char* outDirName) {
        // @note: @es3n1n: getting current scope name & formatting it
        //
        constexpr std::string_view dll_extension = ".dll";
        auto scope_name = current->GetScopeName();
        if (ends_with(scope_name.data(), dll_extension.data()))
            scope_name.remove_suffix(dll_extension.size());

        // @note: @es3n1n: build file path
        //
        if (!std::filesystem::exists(outDirName))
            std::filesystem::create_directories(outDirName);
        const std::string out_file_path = std::format("{}\\{}.json", outDirName, scope_name);

        // @note: @es3n1n: init codegen
        //
        auto builder = codegen::get();

        // @note: @es3n1n: get stuff from schema that we'll use later
        //
        const auto current_classes = current->GetClasses();
        const auto current_enums = current->GetEnums();

        // @note: @es3n1n: print banner
        //
        builder.begin_json_object();

        // @note: @es3n1n: assemble props
        //
        AssembleEnums(builder, current_enums);
        AssembleClasses(current, builder, current_classes);

        builder.end_json_object(false);

        // @note: @es3n1n: write generated data to output file
        //
        std::ofstream f(out_file_path, std::ios::out);
        f << builder.str();
        f.close();
    }
} // namespace sdk
