
#include "types.h"

mat4 Mat4RemoveTranslate(mat4 in)
{
	mat4 result;

	result.e[0] = in.e[0]; result.e[1] = in.e[1]; result.e[2] = in.e[2]; result.e[3] = 0.f;
	result.e[4] = in.e[4]; result.e[5] = in.e[5]; result.e[6] = in.e[6]; result.e[7] = 0.f;
	result.e[8] = in.e[8]; result.e[9] = in.e[9]; result.e[10] = in.e[10]; result.e[11] = 0.f;
	result.e[12] = in.e[12]; result.e[13] = in.e[13]; result.e[14] = in.e[14]; result.e[15] = 0.f;

	return result;
}