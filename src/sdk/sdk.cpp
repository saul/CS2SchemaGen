#include "sdk/sdk.h"
#include <filesystem>
#include <set>
#include <string_view>

namespace {
    using namespace std::string_view_literals;

    struct CSchemaVarName {
        const char* m_name;
        const char* m_type;
    };

    struct CSchemaNetworkValue {
        union {
            const char* m_p_sz_value;
            int m_n_value;
            float m_f_value;
            std::uintptr_t m_p_value;
            CSchemaVarName m_var_value;
            std::array<char, 32> m_sz_value;
        };
    };

    constinit std::array string_metadata_entries = {FNV32("MNetworkChangeCallback"),
                                                    FNV32("MPropertyFriendlyName"),
                                                    FNV32("MPropertyDescription"),
                                                    FNV32("MPropertyAttributeRange"),
                                                    FNV32("MPropertyStartGroup"),
                                                    FNV32("MPropertyAttributeChoiceName"),
                                                    FNV32("MPropertyGroupName"),
                                                    FNV32("MNetworkUserGroup"),
                                                    FNV32("MNetworkAlias"),
                                                    FNV32("MNetworkTypeAlias"),
                                                    FNV32("MNetworkSerializer"),
                                                    FNV32("MPropertyAttributeEditor"),
                                                    FNV32("MPropertySuppressExpr"),
                                                    FNV32("MKV3TransferName"),
                                                    FNV32("MFieldVerificationName"),
                                                    FNV32("MVectorIsSometimesCoordinate"),
                                                    FNV32("MNetworkEncoder"),
                                                    FNV32("MPropertyCustomFGDType"),
                                                    FNV32("MVDataUniqueMonotonicInt"),
                                                    FNV32("MScriptDescription")};

    constinit std::array string_class_metadata_entries = {
        FNV32("MResourceTypeForInfoType"),
    };

    constinit std::array var_name_string_class_metadata_entries = {
        FNV32("MNetworkVarNames"),
        FNV32("MNetworkOverride"),
        FNV32("MNetworkVarTypeOverride"),
    };

    constinit std::array var_string_class_metadata_entries = {
        FNV32("MPropertyArrayElementNameKey"), FNV32("MPropertyFriendlyName"),      FNV32("MPropertyDescription"),
        FNV32("MNetworkExcludeByName"),        FNV32("MNetworkExcludeByUserGroup"), FNV32("MNetworkIncludeByName"),
        FNV32("MNetworkIncludeByUserGroup"),   FNV32("MNetworkUserGroupProxy"),     FNV32("MNetworkReplayCompatField"),
    };

    constinit std::array integer_metadata_entries = {
        FNV32("MNetworkVarEmbeddedFieldOffsetDelta"),
        FNV32("MNetworkBitCount"),
        FNV32("MNetworkPriority"),
        FNV32("MPropertySortPriority"),
        FNV32("MParticleMinVersion"),
        FNV32("MParticleMaxVersion"),
        FNV32("MNetworkEncodeFlags"),
    };

    constinit std::array float_metadata_entries = {
        FNV32("MNetworkMinValue"),
        FNV32("MNetworkMaxValue"),
    };

    inline bool ends_with(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }
} // namespace

namespace sdk {
    namespace {
        void AssembleEnums(codegen::generator_t::self_ref builder, CUtlTSHash<CSchemaEnumInfo*, 256, uint>& enums) {
            builder.json_key("enums").begin_json_object_value();

            UtlTSHashHandle_t* handles = new UtlTSHashHandle_t[enums.Count()];
            enums.GetElements(0, enums.Count(), handles);

            for (int i = 0; i < enums.Count(); ++i) {
                auto schema_enum_binding = enums[handles[i]];

                // @note: @es3n1n: get type name by align size
                //
                const auto get_type_name = [schema_enum_binding]() [[msvc::forceinline]] {
                    std::string type_storage;

                    switch (schema_enum_binding->m_nAlignment) {
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
                builder.json_key(schema_enum_binding->m_pszName).begin_json_object_value();

                builder.json_key("align").json_literal(schema_enum_binding->m_nAlignment);

                // @note: @es3n1n: assemble enum items
                //
                builder.json_key("items").begin_json_array_value();
                for (int enumIdx = 0; enumIdx < schema_enum_binding->m_nEnumeratorCount; ++enumIdx) { 
                    const auto& field = schema_enum_binding->m_pEnumerators[enumIdx];
                    builder.begin_json_object()
                        .json_key("name")
                        .json_string(field.m_pszName)
                        .json_key("value")
                        .json_literal(field.m_nValue == std::numeric_limits<int64>::max() ? -1 : field.m_nValue)
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
            builder.begin_json_object_value().json_key("name").json_string(current_type->m_sTypeName.Get());
            
            builder.json_key("category").json_literal((int)current_type->m_eTypeCategory);

            if (current_type->m_eTypeCategory == SCHEMA_TYPE_ATOMIC) {
                builder.json_key("atomic").json_literal((int)current_type->m_eAtomicCategory);

                if (current_type->m_eAtomicCategory == SCHEMA_ATOMIC_T || current_type->m_eAtomicCategory == SCHEMA_ATOMIC_COLLECTION_OF_T) {
                    const auto atomic_t = (CSchemaType_Atomic_T*)current_type;
                    if (atomic_t->m_pAtomicInfo != nullptr) {
                        builder.json_key("outer").json_string(atomic_t->m_pAtomicInfo->m_pszName1);
                    }

                    if (atomic_t->m_pTemplateType != nullptr) {
                        builder.json_key("inner");
                        WriteTypeJson(builder, atomic_t->m_pTemplateType);
                    }
                }
            } else if (current_type->m_eTypeCategory == SCHEMA_TYPE_FIXED_ARRAY) {
                builder.json_key("arraySize").json_literal(((CSchemaType_FixedArray*)current_type)->m_nElementCount);
                builder.json_key("inner");
                WriteTypeJson(builder, ((CSchemaType_FixedArray*)current_type)->m_pElementType);
            } else if (current_type->m_eTypeCategory == SCHEMA_TYPE_PTR) {
                builder.json_key("inner");
                WriteTypeJson(builder, current_type->GetInnerType().Get());
            }

            builder.end_json_object();
        }

        void AssembleClasses(CSchemaSystemTypeScope* current, codegen::generator_t::self_ref builder, CUtlTSHash<CSchemaClassInfo*, 256, uint>& classes) {
            struct class_t {
                CSchemaClassInfo* target_;
                std::set<CSchemaClassInfo*> refs_;

                [[nodiscard]] CSchemaClassInfo* GetParent() const {
                    if (target_->m_nBaseClassCount >= 1) {
                        return target_->m_pBaseClasses[0].m_pClass;
                    }

                    return nullptr;
                }

                void AddRefToClass(CSchemaType* type) {
                    if (type->m_eTypeCategory == SCHEMA_TYPE_DECLARED_CLASS) {
                        refs_.insert(((CSchemaType_DeclaredClass*)type)->m_pClassInfo);
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
            };

            // @note: @soufiw:
            // sort all classes based on refs and inherit, and then print it.
            // ==================
            std::list<class_t> classes_to_dump;

            UtlTSHashHandle_t* handles = new UtlTSHashHandle_t[classes.Count()];
            classes.GetElements(0, classes.Count(), handles);

            for (int i = 0; i < classes.Count(); ++i) {
                auto class_info = classes[handles[i]];
                auto& class_dump = classes_to_dump.emplace_back();
                class_dump.target_ = class_info;

                for (auto k = 0; k < class_info->m_nFieldCount; k++) {
                    const auto field = &class_info->m_pFields[k];
                    if (!field)
                        continue;

                    class_dump.AddRefToClass(field->m_pType);
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

                builder.json_key(class_info->m_pszName).begin_json_object_value();

                if (class_info->m_nBaseClassCount >= 1) {
                    builder.json_key("parent").json_string(class_info->m_pBaseClasses[0].m_pClass->m_pszName);
                }

                auto write_metadata_json = [&](const SchemaMetadataEntryData_t metadata_entry) -> void {
                    std::string value;

                    const auto value_hash_name = fnv32::hash_runtime(metadata_entry.m_pszName);

                    builder.begin_json_object();
                    builder.json_key("name").json_string(metadata_entry.m_pszName);

                    if (strcmp(metadata_entry.m_pszName, "MResourceTypeForInfoType") == 0) {
                        builder.end_json_object();
                        return;
                    }

                    const auto metadata_value = ((CSchemaNetworkValue*)metadata_entry.m_pData);

                    // clang-format off
                    if (std::find(var_name_string_class_metadata_entries.begin(), var_name_string_class_metadata_entries.end(), value_hash_name) != var_name_string_class_metadata_entries.end())
                    {
                        builder.json_key("value").begin_json_object_value();

                        const auto &var_value = metadata_value->m_var_value;
                        if (var_value.m_type)
                            builder.json_key("type").json_string(var_value.m_type);
                        if (var_value.m_name)
                            builder.json_key("name").json_string(var_value.m_name);

                        builder.end_json_object();
                    }
                    else if (std::find(string_class_metadata_entries.begin(), string_class_metadata_entries.end(), value_hash_name) != string_class_metadata_entries.end())
                        builder.json_key("value").json_string(metadata_value->m_sz_value.data());
                    else if (std::find(var_string_class_metadata_entries.begin(), var_string_class_metadata_entries.end(), value_hash_name) != var_string_class_metadata_entries.end())
                        builder.json_key("value").json_string(metadata_value->m_p_sz_value);
                    else if (std::find(string_metadata_entries.begin(), string_metadata_entries.end(), value_hash_name) != string_metadata_entries.end())
                        builder.json_key("value").json_string(metadata_value->m_p_sz_value);
                    else if (std::find(integer_metadata_entries.begin(), integer_metadata_entries.end(), value_hash_name) != integer_metadata_entries.end())
                        builder.json_key("value").json_literal(metadata_value->m_n_value);
                    else if (std::find(float_metadata_entries.begin(), float_metadata_entries.end(), value_hash_name) != float_metadata_entries.end())
                        builder.json_key("value").json_literal(metadata_value->m_f_value);
                    // clang-format on

                    builder.end_json_object();
                };

                bool is_atomic = false;
                std::set<std::string> network_var_names;
                builder.json_key("metadata").begin_json_array_value();
                for (int metadataIdx = 0; metadataIdx < class_info->m_nStaticMetadataCount; ++metadataIdx) { 
                    const auto& metadata = class_info->m_pStaticMetadata[metadataIdx];
                    const auto metadata_value = ((CSchemaNetworkValue*)metadata.m_pData);
                    if (strcmp(metadata.m_pszName, "MNetworkVarNames") == 0) {
                        // Keep track of all network vars
                        network_var_names.insert(metadata_value->m_var_value.m_name);

                        // don't write var names - too verbose
                        continue;
                    }
                    
                    if (strcmp(metadata.m_pszName, "MNetworkVarsAtomic") == 0)
                    {
                        is_atomic = true;
                    }

                    write_metadata_json(metadata);
                }
                builder.end_json_array();

                builder.json_key("fields").begin_json_array_value();

                // @note: @es3n1n: begin public members
                //
                if (strcmp(class_info->m_pszName, "ServerAuthoritativeWeaponSlot_t") == 0)
                {
                    printf(".");
                }

                for (int fieldIdx = 0; fieldIdx < class_info->m_nFieldCount; ++fieldIdx) { 
                    const auto& field = class_info->m_pFields[fieldIdx];
                    if (!network_var_names.contains(field.m_pszName) && !is_atomic) {
                        bool is_network_enable = strcmp(class_info->m_pszName, "ServerAuthoritativeWeaponSlot_t") == 0;
                        for (auto j = 0; j < field.m_nStaticMetadataCount; j++) {
                            if (strcmp(field.m_pStaticMetadata[j].m_pszName, "MNetworkEnable") == 0) {
                                is_network_enable = true;
                                break;
                            }
                        }

                        if (!is_network_enable) {
                            continue;
                        }
                    }

                    builder.begin_json_object().json_key("name").json_string(field.m_pszName);

                    builder.json_key("type");

                    WriteTypeJson(builder, field.m_pType);

                    builder.json_key("metadata").begin_json_array_value();

                    for (auto j = 0; j < field.m_nStaticMetadataCount; j++) {
                        if (strcmp(field.m_pStaticMetadata[j].m_pszName, "MNetworkEnable")) {
                            write_metadata_json(field.m_pStaticMetadata[j]);
                        }
                    }

                    builder.end_json_array();

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
        std::string scope_name = current->GetScopeName();
        if (ends_with(scope_name.data(), dll_extension.data()))
            scope_name.erase(scope_name.length() - dll_extension.size());

        // @note: @es3n1n: build file path
        //
        if (!std::filesystem::exists(outDirName))
            std::filesystem::create_directories(outDirName);
        const std::string out_file_path = std::format("{}\\{}.json", outDirName, scope_name);

        auto builder = codegen::get();

        builder.begin_json_object();

        // @note: @es3n1n: assemble props
        //
        AssembleEnums(builder, current->m_EnumBindings);
        AssembleClasses(current, builder, current->m_ClassBindings);

        builder.end_json_object(false);

        // @note: @es3n1n: write generated data to output file
        //
        std::ofstream f(out_file_path, std::ios::out);
        f << builder.str();
        f.close();
    }
} // namespace sdk
