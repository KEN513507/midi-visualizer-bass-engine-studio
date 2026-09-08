#include "TestFramework.h"
#include "midi/MidiInput.h"
#include "midi/MidiTypes.h"

using namespace midi;

TEST_CASE(Midi_NoteOn_Decodes) {
    MidiShortMessage msg{0, 0x90, 60, 102};
    REQUIRE(decodeCommand(msg) == MidiCommand::NoteOn);
    REQUIRE_EQ(msg.channel(), 1);
}

TEST_CASE(Midi_NoteOnVelocityZero_NormalizesToNoteOff) {
    // docs/USB_MIDI_BRINGUP_TEST.md section 5.
    MidiShortMessage msg{0, 0x90, 60, 0};
    REQUIRE(decodeCommand(msg) == MidiCommand::NoteOff);
}

TEST_CASE(Midi_NoteOff_Decodes) {
    MidiShortMessage msg{0, 0x80, 60, 0};
    REQUIRE(decodeCommand(msg) == MidiCommand::NoteOff);
}

TEST_CASE(Midi_ControlChange_Decodes) {
    MidiShortMessage msg{0, 0xB0, 64, 127};
    REQUIRE(decodeCommand(msg) == MidiCommand::ControlChange);
}

TEST_CASE(Midi_Channel_DecodesAllSixteen) {
    for (int ch = 0; ch < 16; ++ch) {
        MidiShortMessage msg{0, static_cast<uint8_t>(0x90 | ch), 60, 100};
        REQUIRE_EQ(msg.channel(), ch + 1);
    }
}

TEST_CASE(Midi_NoteName_MiddleCIs60) {
    // MIDI 60 = C4 (docs/SYSTEM_REQUIREMENTS.md section 4,
    // docs/USB_MIDI_BRINGUP_TEST.md section 6).
    REQUIRE_EQ(noteName(60), std::string("C4"));
    REQUIRE_EQ(noteName(48), std::string("C3"));
    REQUIRE_EQ(noteName(72), std::string("C5"));
}

TEST_CASE(Midi_Sustain_BinaryThreshold) {
    REQUIRE(isSustainOn(127));
    REQUIRE(isSustainOn(64));
    REQUIRE_FALSE(isSustainOn(63));
    REQUIRE_FALSE(isSustainOn(0));
}

TEST_CASE(Midi_IsLikelyVirtualEndpointName_MatchesKnownSoftwareSynths) {
    REQUIRE(isLikelyVirtualEndpointName("Microsoft GS Wavetable Synth"));
    REQUIRE(isLikelyVirtualEndpointName("loopMIDI Port 1"));
    REQUIRE(isLikelyVirtualEndpointName("LoopBe Internal MIDI"));
    REQUIRE(isLikelyVirtualEndpointName("rtpMIDI Session"));
    REQUIRE(isLikelyVirtualEndpointName("Midi Through Port-0"));
}

TEST_CASE(Midi_IsLikelyVirtualEndpointName_LeavesPhysicalDevicesAlone) {
    REQUIRE_FALSE(isLikelyVirtualEndpointName("CASIO USB-MIDI"));
    REQUIRE_FALSE(isLikelyVirtualEndpointName("Yamaha MOTIF XS"));
}
