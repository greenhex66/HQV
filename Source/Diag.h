#pragma once
#include <juce_core/juce_core.h>

// Diagnostic trace: appends one line per step to  %TEMP%\HEX-Q-VANTA-log.txt
// (the file is opened and closed on every call, so the last line survives a crash).
// Never called from the audio thread except once on the very first block.
namespace vanta
{
inline void trace (const juce::String& message)
{
    static juce::CriticalSection lock;
    const juce::ScopedLock sl (lock);

    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("HEX-Q-VANTA-log.txt");

    file.appendText (juce::Time::getCurrentTime().toString (false, true, true, true)
                         + "  [thread " + juce::String ((juce::int64) (juce::pointer_sized_int) juce::Thread::getCurrentThreadId()) + "]  "
                         + message + "\n");
}
} // namespace vanta
