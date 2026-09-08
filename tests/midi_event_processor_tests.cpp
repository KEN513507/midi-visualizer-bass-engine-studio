#include "TestFramework.h"
#include "midi/MidiEventProcessor.h"

using namespace midi;

TEST_CASE(EventProcessor_NoteOn_ProducesNoteOnEvent) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {{100, 0x90, 60, 96}};

    auto result = proc.process(messages);
    REQUIRE_EQ(result.noteOn.size(), static_cast<size_t>(1));
    REQUIRE_EQ(result.noteOff.size(), static_cast<size_t>(0));
    REQUIRE_EQ(result.noteOn[0].midiNote, 60);
    REQUIRE_EQ(result.noteOn[0].velocity, 96);
    REQUIRE_EQ(result.noteOn[0].channel, 1);
}

TEST_CASE(EventProcessor_NoteOnVelocityZero_ProducesNoteOffEvent) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {{100, 0x90, 60, 0}};

    auto result = proc.process(messages);
    REQUIRE_EQ(result.noteOn.size(), static_cast<size_t>(0));
    REQUIRE_EQ(result.noteOff.size(), static_cast<size_t>(1));
    REQUIRE_EQ(result.noteOff[0].midiNote, 60);
}

TEST_CASE(EventProcessor_NoteOff_ProducesNoteOffEvent) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {{100, 0x80, 60, 0}};

    auto result = proc.process(messages);
    REQUIRE_EQ(result.noteOff.size(), static_cast<size_t>(1));
}

TEST_CASE(EventProcessor_CC64_ProducesSustainEvent) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {{100, 0xB0, 64, 127}, {200, 0xB0, 64, 0}};

    auto result = proc.process(messages);
    REQUIRE_EQ(result.sustain.size(), static_cast<size_t>(2));
    REQUIRE(result.sustain[0].on);
    REQUIRE_FALSE(result.sustain[1].on);
}

TEST_CASE(EventProcessor_NonSustainCC_IsIgnored) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {{100, 0xB0, 88, 16}};

    auto result = proc.process(messages);
    REQUIRE_EQ(result.sustain.size(), static_cast<size_t>(0));
    REQUIRE_EQ(result.noteOn.size(), static_cast<size_t>(0));
    REQUIRE_EQ(result.noteOff.size(), static_cast<size_t>(0));
}

TEST_CASE(EventProcessor_MixedStream_PreservesOrder) {
    MidiEventProcessor proc;
    std::vector<MidiShortMessage> messages = {
        {100, 0x90, 60, 100},  // NoteOn C4
        {150, 0x90, 64, 80},   // NoteOn E4
        {200, 0x80, 60, 0},    // NoteOff C4
        {250, 0xB0, 64, 127},  // Sustain on
    };

    auto result = proc.process(messages);
    REQUIRE_EQ(result.noteOn.size(), static_cast<size_t>(2));
    REQUIRE_EQ(result.noteOn[0].midiNote, 60);
    REQUIRE_EQ(result.noteOn[1].midiNote, 64);
    REQUIRE_EQ(result.noteOff.size(), static_cast<size_t>(1));
    REQUIRE_EQ(result.noteOff[0].midiNote, 60);
    REQUIRE_EQ(result.sustain.size(), static_cast<size_t>(1));
}
