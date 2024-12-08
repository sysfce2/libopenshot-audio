/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class PlatformTimer final
{
public:
    explicit PlatformTimer (PlatformTimerListener& ptl)
        : listener { ptl } {}

    void startTimer (int newIntervalMs)
    {
        jassert (newIntervalMs > 0);

        const auto callback = [] (UINT, UINT, DWORD_PTR context, DWORD_PTR, DWORD_PTR)
        {
            reinterpret_cast<PlatformTimerListener*> (context)->onTimerExpired();
        };

        timerId = timeSetEvent ((UINT) newIntervalMs, 1, callback, (DWORD_PTR) &listener, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
        const auto timerStarted = timerId != 0;

        if (timerStarted)
        {
            intervalMs = newIntervalMs;
            return;
        }

        if (fallbackTimer == nullptr)
        {
            // This assertion indicates that the creation of a high-resolution timer
            // failed, and the timer is falling back to a less accurate implementation.
            // Timer callbacks will still fire but the timing precision of the callbacks
            // will be significantly compromised!
            // The most likely reason for this is that more than the system limit of 16
            // HighResolutionTimers are trying to run simultaneously in the same process.
            // You may be able to reduce the number of HighResolutionTimer instances by
            // only creating one instance that is shared (see SharedResourcePointer).
            //
            // However, if this is a plugin running inside a host, other plugins could
            // be creating timers in the same process. In most cases it's best to find
            // an alternative approach than relying on the precision of any timer!
           #if ! JUCE_UNIT_TESTS
            jassertfalse;
           #endif

            fallbackTimer = std::make_unique<GenericPlatformTimer> (listener);
        }

        fallbackTimer->startTimer (newIntervalMs);
        intervalMs = fallbackTimer->getIntervalMs();
    }

    void cancelTimer()
    {
        jassert (timerId != 0);

        timeKillEvent (timerId);
        timerId = 0;
        intervalMs = 0;
    }

    int getIntervalMs() const
    {
        return intervalMs;
    }

private:
    PlatformTimerListener& listener;
    UINT timerId { 0 };
    int intervalMs { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlatformTimer)
    JUCE_DECLARE_NON_MOVEABLE (PlatformTimer)
};

} // namespace juce
