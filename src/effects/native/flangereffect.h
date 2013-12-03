#ifndef FLANGEREFFECT_H
#define FLANGEREFFECT_H

#include <QMap>

#include "util.h"
#include "effects/effect.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "effects/native/nativebackend.h"
#include "effects/effectprocessor.h"

const unsigned int kMaxDelay = 5000;
const unsigned int kLfoAmplitude = 240;
const unsigned int kAverageDelayLength = 250;

struct FlangerState {
    CSAMPLE delayBuffer[kMaxDelay];
    unsigned int delayPos;
    unsigned int time;
};

class FlangerProcessor : public EffectProcessor {
  public:
    FlangerProcessor(const EffectManifest& manifest);
    virtual ~FlangerProcessor();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void initialize(EngineEffect* pEffect);

    // See effectprocessor.h
    void process(const QString& group,
                 const CSAMPLE* pInput, CSAMPLE* pOutput,
                 const unsigned int numSamples);

  private:
    QString debugString() const {
        return "FlangerProcessor";
    }
    FlangerState* getStateForGroup(const QString& group);

    EngineEffectParameter* m_pPeriodParameter;
    EngineEffectParameter* m_pDepthParameter;
    EngineEffectParameter* m_pDelayParameter;

    QMap<QString, FlangerState*> m_flangerStates;

    DISALLOW_COPY_AND_ASSIGN(FlangerProcessor);
};

#endif /* FLANGEREFFECT_H */
