#pragma once
#include <JuceHeader.h>
#include "SynthVoice.h"
#include <vector>
#include <algorithm>

class SynthEngine : public juce::Synthesiser
{
public:
    struct HeldNote
    {
        int channel;
        int noteNumber;
        float velocity;
    };

    SynthEngine() = default;

    void setPlayMode (int mode)
    {
        if (playMode != mode)
        {
            allNotesOff (0, false);
            heldNotes.clear();
            playMode = mode;
        }
    }

    int getPlayMode() const noexcept { return playMode; }

    void noteOn (const int midiChannel, const int midiNoteNumber, const float velocity) override
    {
        if (playMode == 0) // Poly
        {
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }

        // Monophonic modes: Mono (1) or Legato (2)
        const juce::ScopedLock sl (lock);

        // Remove if this note was already in the stack so it moves to top (last-note priority)
        auto it = std::find_if (heldNotes.begin(), heldNotes.end(), [midiNoteNumber](const HeldNote& n) {
            return n.noteNumber == midiNoteNumber;
        });
        if (it != heldNotes.end())
            heldNotes.erase (it);

        bool wasEmpty = heldNotes.empty();
        heldNotes.push_back ({ midiChannel, midiNoteNumber, velocity });

        if (voices.isEmpty()) return;
        auto* voice = dynamic_cast<SynthVoice*> (voices.getUnchecked (0));
        if (voice == nullptr) return;

        // Ensure voices 1..N are silent in mono/legato
        for (int i = 1; i < voices.size(); ++i)
        {
            if (auto* other = dynamic_cast<SynthVoice*> (voices.getUnchecked (i)))
                if (other->isVoiceActive())
                    other->stopNote (0.0f, false);
        }

        if (wasEmpty || !voice->isVoiceActive())
        {
            // First note from silence: trigger standard noteOn
            startVoice (voice, sounds.getUnchecked (0).get(), midiChannel, midiNoteNumber, velocity);
        }
        else
        {
            // Overlapping note transition
            bool retrigger = (playMode == 1); // Mono retriggers envelopes; Legato does not
            voice->retargetNote (midiNoteNumber, velocity, retrigger);
        }
    }

    void noteOff (const int midiChannel, const int midiNoteNumber, const float velocity, const bool allowTailOff) override
    {
        if (playMode == 0) // Poly
        {
            juce::Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
            return;
        }

        // Monophonic modes: Mono or Legato
        const juce::ScopedLock sl (lock);

        auto it = std::find_if (heldNotes.begin(), heldNotes.end(), [midiNoteNumber](const HeldNote& n) {
            return n.noteNumber == midiNoteNumber;
        });
        if (it != heldNotes.end())
            heldNotes.erase (it);

        if (voices.isEmpty()) return;
        auto* voice = dynamic_cast<SynthVoice*> (voices.getUnchecked (0));
        if (voice == nullptr) return;

        if (heldNotes.empty())
        {
            // All keys released: release the active voice
            stopVoice (voice, velocity, allowTailOff);
        }
        else
        {
            // Keys still held: return to the top held note
            const auto& top = heldNotes.back();
            bool retrigger = (playMode == 1); // Mono retriggers; Legato returns smoothly
            voice->retargetNote (top.noteNumber, top.velocity, retrigger);
        }
    }

    void allNotesOff (const int midiChannel, const bool allowTailOff) override
    {
        heldNotes.clear();
        juce::Synthesiser::allNotesOff (midiChannel, allowTailOff);
    }

private:
    int playMode = 0; // 0=Poly, 1=Mono, 2=Legato
    std::vector<HeldNote> heldNotes;
};
