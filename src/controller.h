#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstnoteexpressiontypes.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstmidilearn.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"
#include "pluginterfaces/vst/ivstphysicalui.h"
#include <array>
#include <limits>

namespace paukn {

using namespace Steinberg;
using namespace Steinberg::Vst;

class Controller : public EditController,
                   public INoteExpressionController,
                   public IMidiMapping,
                   public INoteExpressionPhysicalUIMapping
{
public:
	//--- EditController -----------------------------
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;

	//--- IMidiMapping -------------------------------
	tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id/*out*/) SMTG_OVERRIDE;

	//--- INoteExpressionController ------------------
	int32 PLUGIN_API getNoteExpressionCount (int32 busIndex, int16 channel) SMTG_OVERRIDE;
	tresult PLUGIN_API getNoteExpressionInfo (int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info /*out*/) SMTG_OVERRIDE;
	tresult PLUGIN_API getNoteExpressionStringByValue (int32 busIndex, int16 channel, NoteExpressionTypeID id, NoteExpressionValue valueNormalized /*in*/, String128 string /*out*/) SMTG_OVERRIDE;
	tresult PLUGIN_API getNoteExpressionValueByString (int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string /*in*/, NoteExpressionValue& valueNormalized /*out*/) SMTG_OVERRIDE;

	//--- INoteExpressionPhysicalUIMapping ------------
  tresult PLUGIN_API getPhysicalUIMapping (int32 busIndex, int16 channel, PhysicalUIMapList& list) SMTG_OVERRIDE;

	//--- ---------------------------------------------
	static FUnknown* createInstance (void*) { return (IEditController*)new Controller (); }

	static FUID cid;

	OBJ_METHODS (Controller, EditController)
	DEFINE_INTERFACES
		DEF_INTERFACE (INoteExpressionController)
		DEF_INTERFACE (IMidiMapping)
		DEF_INTERFACE (INoteExpressionPhysicalUIMapping)
	END_DEFINE_INTERFACES (EditController)
	REFCOUNT_METHODS (EditController)

protected:
	NoteExpressionTypeContainer noteExpressionTypes;

	static constexpr ParamID InvalidParamID = std::numeric_limits<ParamID>::max ();

	std::array<ParamID, ControllerNumbers::kCountCtrlNumber> midiCCMapping;
};

//------------------------------------------------------------------------
static constexpr auto MsgIDEvent = "Event";

//------------------------------------------------------------------------
} // paukn
