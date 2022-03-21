#include "processor.h"
#include "controller.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "version.h"

#define stringPluginName "paukn"

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)

	DEF_CLASS2 (INLINE_UID_FROM_FUID(paukn::Processor::cid),
				PClassInfo::kManyInstances,
				kVstAudioEffectClass,
				stringPluginName,
               0, // for running processor and controller on different hosts use Vst::kDistributable,
            Vst::PlugType::kFxInstrument,
				FULL_VERSION_STR,
				kVstVersionString,
				paukn::Processor::createInstance)

	DEF_CLASS2 (INLINE_UID_FROM_FUID(paukn::Controller::cid),
				PClassInfo::kManyInstances,
				kVstComponentControllerClass,
				stringPluginName,
				0,						// not used here
				"",						// not used here
				FULL_VERSION_STR,
				kVstVersionString,
				paukn::Controller::createInstance)

END_FACTORY
