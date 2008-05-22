#include "IoState.h"
#include "IoObject.h"

IoObject *IoYAML_proto(void *state);

void IoYAMLInit(IoObject *context)
{
	IoState *self = IoObject_state((IoObject *)context);

	IoObject_setSlot_to_(context, SIOSYMBOL("YAML"), IoYAML_proto(self));

}
