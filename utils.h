//
// Created by Tobi on 07/03/2021.
//
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_formats/juce_audio_formats.h>

static inline juce::FlacAudioFormat FLAC_FORMAT;

template<class T>
struct PropertyListener : public juce::ValueTree::Listener {
    using FT = std::function<void(const T &)>;

    std::vector<FT> call_backs;
    juce::ValueTree &target_tree;
    const juce::Identifier target_property;
    const juce::CachedValue<T> &value;

    PropertyListener(
            juce::ValueTree &target_tree,
            const juce::Identifier target_property, // TODO: does this need to be copied
            const juce::CachedValue<T> &value) : target_tree(target_tree),
                                                 target_property(target_property),
                                                 value(value) {}

    void valueTreePropertyChanged(juce::ValueTree &changed_tree, const juce::Identifier &changed_property) override {
        if (changed_property == target_property && changed_tree == target_tree) {
            const auto &new_value = value.get();
            for (const auto &cb : call_backs)
                cb(new_value);
        }
    }

    void add_listener(const FT &call_back) {
        call_backs.push_back(call_back);
    }
};

class ByteArrayInputStream : public juce::MemoryInputStream {
    const ByteArray buffer_;

public:
    ByteArrayInputStream(ByteArray &&buffer) : buffer_(std::move(buffer)),
                                             juce::MemoryInputStream(buffer.c_data(), buffer.size(), false) {
    }
};


inline juce::FileInputStream *get_inputstream(const fs::path &path) {
    return new juce::FileInputStream(to_file(path));
}