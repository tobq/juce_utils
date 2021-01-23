//
// Created by tobia on 23/01/2021.
//

#pragma once

#include <juce_data_structures/juce_data_structures.h>


void load_tree_exact(juce::ValueTree &target_tree, const juce::ValueTree &source_tree);

void load_tree_required(juce::ValueTree &target_tree, const juce::ValueTree &source_tree);

void load_tree_loose(juce::ValueTree &target_tree, const juce::ValueTree &source_tree);