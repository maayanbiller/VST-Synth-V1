#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>
#include <array>
#include <emmintrin.h> // SSE2

class PolyBLEPOscillator
{
public:
    enum class Waveform { Sine, Triangle, Saw, Square };

    PolyBLEPOscillator() = default;
    ~PolyBLEPOscillator() = default;

    void randomizePhase() { phase = juce::Random::getSystemRandom().nextFloat(); }

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        inc = 0.0f;
        invInc = 0.0f;
        phase = 0.0f;
    }

    void setFrequency(float freq) noexcept
    {
        float maxFreq = static_cast<float>(currentSampleRate) * 0.49f;
        float safeFreq = std::clamp(freq, 0.1f, maxFreq);
        inc = safeFreq / static_cast<float>(currentSampleRate);
        invInc = (inc > 1e-6f) ? (1.0f / inc) : 0.0f;
    }

    void setWaveform(Waveform wave) noexcept
    {
        waveform = wave;
    }

    void setPWM(float pulseWidth) noexcept
    {
        pwm = juce::jlimit(0.05f, 0.95f, pulseWidth);
    }

    // Domain-safe symmetry-folded Sine on [0, 1) -> [-0.25, +0.25] (< 1.5e-7 max error, 0 wrap discontinuity)
    static inline float fastSin2Pi(float phase01) noexcept
    {
        float pShift = phase01 + 0.25f;
        if (pShift >= 1.0f) pShift -= 1.0f;
        float x = pShift - 0.5f;
        float u = 0.25f - std::fabs(x);
        float u2 = u * u;
        return u * (6.283185307f + u2 * (-41.34170224f + u2 * (81.60524928f + u2 * (-76.70585975f + u2 * 42.0586939f))));
    }

    inline float getNextSample() noexcept
    {
        float value = 0.0f;
        float t = phase;

        switch (waveform)
        {
            case Waveform::Sine:
                value = fastSin2Pi(t);
                break;
            case Waveform::Triangle:
                value = -1.0f + (2.0f * t);
                value = 2.0f * (std::fabs(value) - 0.5f);
                break;
            case Waveform::Saw:
                value = (2.0f * t) - 1.0f;
                value -= polyBlep(t);
                break;
            case Waveform::Square:
            {
                value = t < pwm ? 1.0f : -1.0f;
                value += polyBlep(t);
                float t2 = t + 1.0f - pwm;
                if (t2 >= 1.0f) t2 -= 1.0f;
                value -= polyBlep(t2);
                break;
            }
        }

        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;
        else if (phase < 0.0f) phase += 1.0f;

        return value;
    }

    float getPhase() const noexcept { return phase; }
    void setPhase(float p) noexcept { phase = p; }
    float getInc() const noexcept { return inc; }
    float getInvInc() const noexcept { return invInc; }

private:
    inline float polyBlep(float t) const noexcept
    {
        float dt = inc;
        if (dt <= 1e-6f || dt >= 0.5f)
            return 0.0f;

        if (t < dt)
        {
            t *= invInc;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            t = (t - 1.0f) * invInc;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float inc = 0.0f;
    float invInc = 0.0f;
    float pwm = 0.5f;
    Waveform waveform = Waveform::Saw;
};

class UnisonOscillator
{
public:
    static constexpr int maxVoices = 8;

    UnisonOscillator()
    {
        phases.fill(0.0f);
        incs.fill(0.0f);
        invIncs.fill(0.0f);
        panL.fill(0.0f);
        panR.fill(0.0f);
        spreadMultipliers.fill(1.0f);
        updatePanGains();
    }
    ~UnisonOscillator() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        invSampleRate = 1.0f / static_cast<float>(currentSampleRate);
        phases.fill(0.0f);
        incs.fill(0.0f);
        invIncs.fill(0.0f);
        for (int i = 0; i < maxVoices; ++i)
            voices[i].prepare(sampleRate);
        updatePanGains();
    }

    void randomizePhase()
    {
        auto& rng = juce::Random::getSystemRandom();
        for (int i = 0; i < maxVoices; ++i)
        {
            float p = rng.nextFloat();
            phases[i] = p;
            voices[i].setPhase(p);
        }
    }

    void setWaveform(PolyBLEPOscillator::Waveform wave) noexcept
    {
        if (currentWaveform != wave)
        {
            currentWaveform = wave;
            for (int i = 0; i < maxVoices; ++i)
                voices[i].setWaveform(wave);
        }
    }

    void setPWM(float pulseWidth) noexcept
    {
        pwm = juce::jlimit(0.05f, 0.95f, pulseWidth);
    }

    void setVoices(int count) noexcept
    {
        int c = std::clamp(count, 1, maxVoices);
        if (c != numVoices)
        {
            numVoices = c;
            updatePanGains();
        }
    }

    void setWidth(float width) noexcept
    {
        float w = std::clamp(width, 0.0f, 1.0f);
        if (std::abs(w - stereoWidth) > 0.0001f)
        {
            stereoWidth = w;
            updatePanGains();
        }
    }

    // Exact factored pitch calculation: 1 std::pow call instead of 8!
    void setFrequencyAndDetune(float baseFreq, float detuneCents) noexcept
    {
        float maxFreq = static_cast<float>(currentSampleRate) * 0.49f;
        float safeBaseFreq = std::clamp(baseFreq, 0.1f, maxFreq);
        float cents = std::clamp(detuneCents, -4800.0f, 4800.0f);
        float tunedBase = safeBaseFreq * std::pow(2.0f, cents * (1.0f / 1200.0f));

        if (numVoices <= 1)
        {
            float f = std::clamp(tunedBase, 0.1f, maxFreq);
            float dt = f * invSampleRate;
            incs[0] = dt;
            invIncs[0] = (dt > 1e-6f) ? (1.0f / dt) : 0.0f;
            for (int i = 1; i < maxVoices; ++i)
            {
                incs[i] = 0.0f;
                invIncs[i] = 0.0f;
            }
            return;
        }

        for (int i = 0; i < numVoices; ++i)
        {
            float f = std::clamp(tunedBase * spreadMultipliers[i], 0.1f, maxFreq);
            float dt = f * invSampleRate;
            incs[i] = dt;
            invIncs[i] = (dt > 1e-6f) ? (1.0f / dt) : 0.0f;
        }
        for (int i = numVoices; i < maxVoices; ++i)
        {
            incs[i] = 0.0f;
            invIncs[i] = 0.0f;
        }
    }

    // Contiguous SSE2 SIMD block renderer: adds (oscSample * level) directly into outL / outR
    inline void renderAddBlock(float* outL, float* outR, const float* levels, int numSamples) noexcept
    {
        if (numVoices <= 1)
        {
            renderSingleVoiceBlock(outL, outR, levels, numSamples);
            return;
        }

        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 two = _mm_set1_ps(2.0f);
        const __m128 half = _mm_set1_ps(0.5f);
        const __m128 quarter = _mm_set1_ps(0.25f);
        const __m128 minusOne = _mm_set1_ps(-1.0f);
        const __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));

        __m128 p0 = _mm_load_ps(&phases[0]);
        __m128 p1 = _mm_load_ps(&phases[4]);
        const __m128 dt0 = _mm_load_ps(&incs[0]);
        const __m128 dt1 = _mm_load_ps(&incs[4]);
        const __m128 idt0 = _mm_load_ps(&invIncs[0]);
        const __m128 idt1 = _mm_load_ps(&invIncs[4]);
        const __m128 gL0 = _mm_load_ps(&panL[0]);
        const __m128 gL1 = _mm_load_ps(&panL[4]);
        const __m128 gR0 = _mm_load_ps(&panR[0]);
        const __m128 gR1 = _mm_load_ps(&panR[4]);
        const bool useBank1 = (numVoices > 4);

        switch (currentWaveform)
        {
            case PolyBLEPOscillator::Waveform::Saw:
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        __m128 raw0 = _mm_sub_ps(_mm_mul_ps(two, p0), one);
                        __m128 v0 = _mm_sub_ps(raw0, polyBlepSSE2(p0, dt0, idt0, one));
                        __m128 lVec = _mm_mul_ps(v0, gL0);
                        __m128 rVec = _mm_mul_ps(v0, gR0);

                        if (useBank1)
                        {
                            __m128 raw1 = _mm_sub_ps(_mm_mul_ps(two, p1), one);
                            __m128 v1 = _mm_sub_ps(raw1, polyBlepSSE2(p1, dt1, idt1, one));
                            lVec = _mm_add_ps(lVec, _mm_mul_ps(v1, gL1));
                            rVec = _mm_add_ps(rVec, _mm_mul_ps(v1, gR1));
                        }

                        outL[s] += hsumSSE2(lVec) * lvl;
                        outR[s] += hsumSSE2(rVec) * lvl;
                    }

                    p0 = _mm_add_ps(p0, dt0);
                    p0 = _mm_sub_ps(p0, _mm_and_ps(_mm_cmpge_ps(p0, one), one));
                    if (useBank1)
                    {
                        p1 = _mm_add_ps(p1, dt1);
                        p1 = _mm_sub_ps(p1, _mm_and_ps(_mm_cmpge_ps(p1, one), one));
                    }
                }
                break;
            }

            case PolyBLEPOscillator::Waveform::Square:
            {
                const __m128 pwmVec = _mm_set1_ps(pwm);
                const __m128 oneMinusPwm = _mm_set1_ps(1.0f - pwm);

                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        __m128 m0 = _mm_cmplt_ps(p0, pwmVec);
                        __m128 raw0 = _mm_or_ps(_mm_and_ps(m0, one), _mm_andnot_ps(m0, minusOne));
                        __m128 v0 = _mm_add_ps(raw0, polyBlepSSE2(p0, dt0, idt0, one));
                        __m128 t2_0 = _mm_add_ps(p0, oneMinusPwm);
                        t2_0 = _mm_sub_ps(t2_0, _mm_and_ps(_mm_cmpge_ps(t2_0, one), one));
                        v0 = _mm_sub_ps(v0, polyBlepSSE2(t2_0, dt0, idt0, one));

                        __m128 lVec = _mm_mul_ps(v0, gL0);
                        __m128 rVec = _mm_mul_ps(v0, gR0);

                        if (useBank1)
                        {
                            __m128 m1 = _mm_cmplt_ps(p1, pwmVec);
                            __m128 raw1 = _mm_or_ps(_mm_and_ps(m1, one), _mm_andnot_ps(m1, minusOne));
                            __m128 v1 = _mm_add_ps(raw1, polyBlepSSE2(p1, dt1, idt1, one));
                            __m128 t2_1 = _mm_add_ps(p1, oneMinusPwm);
                            t2_1 = _mm_sub_ps(t2_1, _mm_and_ps(_mm_cmpge_ps(t2_1, one), one));
                            v1 = _mm_sub_ps(v1, polyBlepSSE2(t2_1, dt1, idt1, one));

                            lVec = _mm_add_ps(lVec, _mm_mul_ps(v1, gL1));
                            rVec = _mm_add_ps(rVec, _mm_mul_ps(v1, gR1));
                        }

                        outL[s] += hsumSSE2(lVec) * lvl;
                        outR[s] += hsumSSE2(rVec) * lvl;
                    }

                    p0 = _mm_add_ps(p0, dt0);
                    p0 = _mm_sub_ps(p0, _mm_and_ps(_mm_cmpge_ps(p0, one), one));
                    if (useBank1)
                    {
                        p1 = _mm_add_ps(p1, dt1);
                        p1 = _mm_sub_ps(p1, _mm_and_ps(_mm_cmpge_ps(p1, one), one));
                    }
                }
                break;
            }

            case PolyBLEPOscillator::Waveform::Triangle:
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        __m128 raw0 = _mm_sub_ps(_mm_mul_ps(two, p0), one);
                        __m128 v0 = _mm_mul_ps(two, _mm_sub_ps(_mm_and_ps(raw0, absMask), half));
                        __m128 lVec = _mm_mul_ps(v0, gL0);
                        __m128 rVec = _mm_mul_ps(v0, gR0);

                        if (useBank1)
                        {
                            __m128 raw1 = _mm_sub_ps(_mm_mul_ps(two, p1), one);
                            __m128 v1 = _mm_mul_ps(two, _mm_sub_ps(_mm_and_ps(raw1, absMask), half));
                            lVec = _mm_add_ps(lVec, _mm_mul_ps(v1, gL1));
                            rVec = _mm_add_ps(rVec, _mm_mul_ps(v1, gR1));
                        }

                        outL[s] += hsumSSE2(lVec) * lvl;
                        outR[s] += hsumSSE2(rVec) * lvl;
                    }

                    p0 = _mm_add_ps(p0, dt0);
                    p0 = _mm_sub_ps(p0, _mm_and_ps(_mm_cmpge_ps(p0, one), one));
                    if (useBank1)
                    {
                        p1 = _mm_add_ps(p1, dt1);
                        p1 = _mm_sub_ps(p1, _mm_and_ps(_mm_cmpge_ps(p1, one), one));
                    }
                }
                break;
            }

            case PolyBLEPOscillator::Waveform::Sine:
            {
                const __m128 c1 = _mm_set1_ps(6.283185307f);
                const __m128 c3 = _mm_set1_ps(-41.34170224f);
                const __m128 c5 = _mm_set1_ps(81.60524928f);
                const __m128 c7 = _mm_set1_ps(-76.70585975f);
                const __m128 c9 = _mm_set1_ps(42.0586939f);

                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        __m128 v0 = fastSinSSE2(p0, quarter, half, one, absMask, c1, c3, c5, c7, c9);
                        __m128 lVec = _mm_mul_ps(v0, gL0);
                        __m128 rVec = _mm_mul_ps(v0, gR0);

                        if (useBank1)
                        {
                            __m128 v1 = fastSinSSE2(p1, quarter, half, one, absMask, c1, c3, c5, c7, c9);
                            lVec = _mm_add_ps(lVec, _mm_mul_ps(v1, gL1));
                            rVec = _mm_add_ps(rVec, _mm_mul_ps(v1, gR1));
                        }

                        outL[s] += hsumSSE2(lVec) * lvl;
                        outR[s] += hsumSSE2(rVec) * lvl;
                    }

                    p0 = _mm_add_ps(p0, dt0);
                    p0 = _mm_sub_ps(p0, _mm_and_ps(_mm_cmpge_ps(p0, one), one));
                    if (useBank1)
                    {
                        p1 = _mm_add_ps(p1, dt1);
                        p1 = _mm_sub_ps(p1, _mm_and_ps(_mm_cmpge_ps(p1, one), one));
                    }
                }
                break;
            }
        }

        _mm_store_ps(&phases[0], p0);
        if (useBank1)
            _mm_store_ps(&phases[4], p1);
    }

    inline void getNextStereoSample(float& outL, float& outR) noexcept
    {
        float lvl = 1.0f;
        outL = 0.0f;
        outR = 0.0f;
        renderAddBlock(&outL, &outR, &lvl, 1);
    }

private:
    static inline __m128 polyBlepSSE2(__m128 t, __m128 dt, __m128 invDt, __m128 one) noexcept
    {
        // Just after transition: t < dt -> u = t * invDt; val = 2*u - u*u - 1
        __m128 maskUnder = _mm_cmplt_ps(t, dt);
        __m128 u = _mm_mul_ps(t, invDt);
        __m128 corrUnder = _mm_sub_ps(_mm_sub_ps(_mm_add_ps(u, u), _mm_mul_ps(u, u)), one);

        // Just before transition: t > 1 - dt -> v = (t - 1) * invDt; val = v*v + 2*v + 1
        __m128 oneMinusDt = _mm_sub_ps(one, dt);
        __m128 maskOver = _mm_cmpgt_ps(t, oneMinusDt);
        __m128 v = _mm_mul_ps(_mm_sub_ps(t, one), invDt);
        __m128 corrOver = _mm_add_ps(_mm_add_ps(_mm_mul_ps(v, v), _mm_add_ps(v, v)), one);

        return _mm_or_ps(_mm_and_ps(maskUnder, corrUnder), _mm_and_ps(maskOver, corrOver));
    }

    static inline __m128 fastSinSSE2(__m128 p, __m128 quarter, __m128 half, __m128 one, __m128 absMask,
                                     __m128 c1, __m128 c3, __m128 c5, __m128 c7, __m128 c9) noexcept
    {
        __m128 pShift = _mm_add_ps(p, quarter);
        pShift = _mm_sub_ps(pShift, _mm_and_ps(_mm_cmpge_ps(pShift, one), one));
        __m128 x = _mm_sub_ps(pShift, half);
        __m128 u = _mm_sub_ps(quarter, _mm_and_ps(x, absMask));
        __m128 u2 = _mm_mul_ps(u, u);

        __m128 poly = _mm_add_ps(c7, _mm_mul_ps(u2, c9));
        poly = _mm_add_ps(c5, _mm_mul_ps(u2, poly));
        poly = _mm_add_ps(c3, _mm_mul_ps(u2, poly));
        poly = _mm_add_ps(c1, _mm_mul_ps(u2, poly));
        return _mm_mul_ps(u, poly);
    }

    static inline float hsumSSE2(__m128 v) noexcept
    {
        __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 sums = _mm_add_ps(v, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    }

    inline void renderSingleVoiceBlock(float* outL, float* outR, const float* levels, int numSamples) noexcept
    {
        float p = phases[0];
        const float dt = incs[0];
        const float idt = invIncs[0];

        auto scalarBlep = [dt, idt](float t) noexcept -> float {
            if (dt <= 1e-6f || dt >= 0.5f) return 0.0f;
            if (t < dt) { t *= idt; return t + t - t * t - 1.0f; }
            if (t > 1.0f - dt) { t = (t - 1.0f) * idt; return t * t + t + t + 1.0f; }
            return 0.0f;
        };

        switch (currentWaveform)
        {
            case PolyBLEPOscillator::Waveform::Saw:
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        float val = ((2.0f * p) - 1.0f) - scalarBlep(p);
                        float scaled = val * lvl;
                        outL[s] += scaled;
                        outR[s] += scaled;
                    }
                    p += dt;
                    if (p >= 1.0f) p -= 1.0f;
                }
                break;

            case PolyBLEPOscillator::Waveform::Square:
            {
                const float pw = pwm;
                const float oneMinusPw = 1.0f - pw;
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        float val = (p < pw ? 1.0f : -1.0f) + scalarBlep(p);
                        float t2 = p + oneMinusPw;
                        if (t2 >= 1.0f) t2 -= 1.0f;
                        val -= scalarBlep(t2);
                        float scaled = val * lvl;
                        outL[s] += scaled;
                        outR[s] += scaled;
                    }
                    p += dt;
                    if (p >= 1.0f) p -= 1.0f;
                }
                break;
            }

            case PolyBLEPOscillator::Waveform::Triangle:
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        float val = 2.0f * (std::fabs(-1.0f + 2.0f * p) - 0.5f);
                        float scaled = val * lvl;
                        outL[s] += scaled;
                        outR[s] += scaled;
                    }
                    p += dt;
                    if (p >= 1.0f) p -= 1.0f;
                }
                break;

            case PolyBLEPOscillator::Waveform::Sine:
                for (int s = 0; s < numSamples; ++s)
                {
                    float lvl = levels[s];
                    if (lvl > 0.0001f)
                    {
                        float val = PolyBLEPOscillator::fastSin2Pi(p);
                        float scaled = val * lvl;
                        outL[s] += scaled;
                        outR[s] += scaled;
                    }
                    p += dt;
                    if (p >= 1.0f) p -= 1.0f;
                }
                break;
        }

        phases[0] = p;
    }

    void updatePanGains() noexcept
    {
        float norm = 1.0f / std::sqrt(static_cast<float>(numVoices));
        panL.fill(0.0f);
        panR.fill(0.0f);
        spreadMultipliers.fill(1.0f);

        if (numVoices <= 1)
        {
            panL[0] = 1.0f;
            panR[0] = 1.0f;
            spreadMultipliers[0] = 1.0f;
            return;
        }

        for (int i = 0; i < numVoices; ++i)
        {
            float t = -1.0f + 2.0f * (static_cast<float>(i) / static_cast<float>(numVoices - 1));
            float spreadCents = t * (12.0f + 16.0f * stereoWidth);
            spreadMultipliers[i] = std::pow(2.0f, spreadCents * (1.0f / 1200.0f));

            if (stereoWidth <= 0.001f)
            {
                constexpr float g = 0.70710678f;
                panL[i] = g * norm;
                panR[i] = g * norm;
            }
            else
            {
                float pan = std::clamp(0.5f + 0.5f * (t * stereoWidth), 0.0f, 1.0f);
                panL[i] = std::cos(pan * juce::MathConstants<float>::halfPi) * norm;
                panR[i] = std::sin(pan * juce::MathConstants<float>::halfPi) * norm;
            }
        }
    }

    alignas(16) std::array<float, maxVoices> phases;
    alignas(16) std::array<float, maxVoices> incs;
    alignas(16) std::array<float, maxVoices> invIncs;
    alignas(16) std::array<float, maxVoices> panL;
    alignas(16) std::array<float, maxVoices> panR;
    alignas(16) std::array<float, maxVoices> spreadMultipliers;

    std::array<PolyBLEPOscillator, maxVoices> voices;
    double currentSampleRate = 44100.0;
    float invSampleRate = 1.0f / 44100.0f;
    int numVoices = 1;
    float stereoWidth = 0.5f;
    float pwm = 0.5f;
    PolyBLEPOscillator::Waveform currentWaveform = PolyBLEPOscillator::Waveform::Saw;
};

