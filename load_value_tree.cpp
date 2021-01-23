//
// Created by tobia on 23/01/2021.
//

#include "load_value_tree.h"
#include <fmt/core.h>

static const char *const DEFAULT_LOAD_TARGET_TREE_NAME = "target";
static const char *const DEFAULT_LOAD_SOURCE_TREE_NAME = "source";

struct ValueTreePath {
    const juce::ValueTree &tree;
    const std::string &path;

    ValueTreePath(const juce::ValueTree &tree, const std::string &path) : tree(tree), path(path) {}

    ValueTreePath getChild(int index) const {
        return {
                tree.getChild(index),
                fmt::format("{}[{}]", path, index)
        };
    }
};

/**
 * todo: implement more sophisticated child update
 *       i.e. support re-ordering of children
 *       This function would become deprecated, as we don't need to assert
 *       children with the same index are the same type
 */
void assert_tree_types_match(const ValueTreePath &target_tree, const ValueTreePath &source_tree) {
    const auto &target_type = target_tree.tree.getType();
    const auto &source_type = source_tree.tree.getType();

    if (target_type != source_type)
        throw std::invalid_argument(
                fmt::format("invalid load_tree arguments: mismatched types: {}: {}, {}: {}",
                            target_tree.path,
                            target_type.toString().toStdString(),
                            source_tree.path,
                            source_type.toString().toStdString()));
}

void assert_source_has_target_props(const ValueTreePath &target_tree, const ValueTreePath &source_tree) {
    auto target_prop_count = target_tree.tree.getNumProperties();
    for (int i = 0; i < target_prop_count; ++i) {
        const auto &target_property_name = target_tree.tree.getPropertyName(i);
        if (!source_tree.tree.hasProperty(target_property_name))
            throw std::invalid_argument(
                    fmt::format(
                            "invalid load_tree arguments: property {} in {} missing from {}",
                            target_property_name.toString().toStdString(),
                            target_tree.path,
                            source_tree.path));
    }
}

/**
 * @see load_tree_required
 */
void
assert_load_tree_required_compatible(const ValueTreePath &target_tree, const ValueTreePath &source_tree) {
    assert_tree_types_match(target_tree, source_tree);
    assert_source_has_target_props(target_tree, source_tree);

    auto target_child_count = target_tree.tree.getNumChildren();
    auto source_child_count = source_tree.tree.getNumChildren();
    auto target_prop_count = target_tree.tree.getNumProperties();
    auto source_prop_count = source_tree.tree.getNumProperties();

    if (target_child_count > source_child_count)
        throw std::invalid_argument(
                fmt::format(
                        "invalid load_tree_required arguments: source tree missing children from target tree: {} children: {}, {} children: {}",
                        target_tree.path,
                        target_child_count,
                        source_tree.path,
                        source_child_count));


    if (target_prop_count > source_prop_count)
        throw std::invalid_argument(
                fmt::format(
                        "invalid load_tree_required arguments: {} (with {} properties) is missing properties from {} (with {} properties)",
                        source_tree.path,
                        source_prop_count,
                        target_tree.path,
                        target_prop_count));


    for (int i = 0; i < target_child_count; ++i) {
        assert_load_tree_required_compatible(target_tree.getChild(i), source_tree.getChild(i));
    }
}

/**
 * @see load_tree_loose
 */
void assert_load_tree_loose_compatible(const ValueTreePath &target_tree, const ValueTreePath &source_tree) {
    assert_tree_types_match(target_tree, source_tree);

    int min_child_count = std::min(target_tree.tree.getNumChildren(), source_tree.tree.getNumChildren());

    for (int i = 0; i < min_child_count; ++i)
        assert_load_tree_loose_compatible(
                target_tree.getChild(i),
                source_tree.getChild(i));
}

/**
 * @see load_tree_exact
 */
void assert_load_tree_exact_compatible(const ValueTreePath &target_tree, const ValueTreePath &source_tree) {
    assert_tree_types_match(target_tree, source_tree);
    assert_source_has_target_props(target_tree, source_tree);

    auto target_child_count = target_tree.tree.getNumChildren();
    auto source_child_count = source_tree.tree.getNumChildren();
    auto target_prop_count = target_tree.tree.getNumProperties();
    auto source_prop_count = source_tree.tree.getNumProperties();

    if (target_prop_count != source_prop_count) {
        throw std::invalid_argument(
                fmt::format(
                        "invalid load_tree_exact arguments: property count mismatch: {} properties: {}, {} properties: {}",
                        target_tree.path,
                        target_prop_count,
                        source_tree.path,
                        source_prop_count));
    }


    if (target_child_count != source_child_count)
        throw std::invalid_argument(
                fmt::format(
                        "invalid load_tree_exact arguments: children count mismatch: {} children: {}, {} children: {}",
                        target_tree.path,
                        target_child_count,
                        source_tree.path,
                        source_child_count));

    for (int i = 0; i < target_child_count; ++i) {
        assert_load_tree_exact_compatible(
                target_tree.getChild(i),
                source_tree.getChild(i));
    }
}

inline void load_tree_properties(juce::ValueTree &target_tree,
                                 const juce::ValueTree &source_tree,
                                 const juce::ValueTree &prop_source_tree) {
    int prop_count = prop_source_tree.getNumProperties();
    for (int i = 0; i < prop_count; ++i) {
        auto prop_id = prop_source_tree.getPropertyName(i);
        target_tree.setProperty(prop_id, source_tree.getProperty(prop_id), nullptr);
    }
}

inline void load_tree_children(const juce::ValueTree &target_tree, const juce::ValueTree &source_tree, int load_count,
                               const std::function<void(juce::ValueTree &,
                                                        const juce::ValueTree &)> &loading_strategy) {
    for (int i = 0; i < load_count; ++i) {
        auto target_child = target_tree.getChild(i);
        loading_strategy(target_child, source_tree.getChild(i));
    }
}

inline void assert_compatible(const juce::ValueTree &target_tree,
                              const juce::ValueTree &source_tree,
                              const std::function<void(const ValueTreePath &,
                                                       const ValueTreePath &)> &assert_strategy) {
    assert_strategy(ValueTreePath(target_tree, DEFAULT_LOAD_TARGET_TREE_NAME),
                    ValueTreePath{(source_tree), DEFAULT_LOAD_SOURCE_TREE_NAME});
}

/**
 * Loads tree with exact same schema - preserving Values (and their listeners)
 */
void load_tree_exact(juce::ValueTree &target_tree, const juce::ValueTree &source_tree) {
    assert_compatible(target_tree, source_tree, assert_load_tree_exact_compatible);

    load_tree_properties(target_tree, source_tree, source_tree);
    load_tree_children(target_tree,
                       source_tree,
                       target_tree.getNumProperties(),
                       load_tree_exact);
}

/**
 * Loads tree - preserving Values (and their listeners) <br/>
 *
 * The source_tree is guaranteed to hold all children/properties from target_tree <br/>
 *
 * Adds properties if missing from target <br/>
 * Adds child trees if missing from target
 */
void load_tree_required(juce::ValueTree &target_tree, const juce::ValueTree &source_tree) {
    assert_compatible(target_tree, source_tree, assert_load_tree_required_compatible);

    load_tree_properties(target_tree, source_tree, target_tree);
    load_tree_children(target_tree,
                       source_tree,
                       target_tree.getNumProperties(),
                       load_tree_required);
}

/**
 * Loads tree - preserving Values (and their listeners) <br/>
 *
 * The source_tree does not need to contain all values in the target_tree <br/>
 *
 * Adds properties if missing from target <br/>
 * Adds child trees if missing from target <br/>
 * Any target_properties missing from the source_tree will remain unchanged <br/>
 * Any target children missing from the source_tree will remain unchanged
 *
 */
void load_tree_loose(juce::ValueTree &target_tree, const juce::ValueTree &source_tree) {
    assert_compatible(target_tree, source_tree, assert_load_tree_loose_compatible);

    load_tree_properties(target_tree, source_tree, source_tree);

    auto target_child_count = source_tree.getNumChildren();
    auto source_child_count = source_tree.getNumChildren();
    if (target_child_count < source_child_count) {
        load_tree_children(target_tree,
                           source_tree,
                           target_child_count,
                           load_tree_loose);
        for (int i = target_child_count; i < source_child_count; ++i) {
            target_tree.appendChild(source_tree.getChild(i), nullptr);
        }
    } else {
        load_tree_children(target_tree,
                           source_tree,
                           source_child_count,
                           load_tree_loose);
    }
}