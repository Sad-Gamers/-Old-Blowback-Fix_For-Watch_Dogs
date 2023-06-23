#pragma once
#include <windows.h>

class SlideFix
{
public:
	static void Initialize();
	static void PatchProceduralAnimation();
	inline static DWORD64 pSetProceduralBonePos, pSetBonePos;
};