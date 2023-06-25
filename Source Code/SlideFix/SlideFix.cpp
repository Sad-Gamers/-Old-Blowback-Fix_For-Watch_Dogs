#include "SlideFix.h"
#include "SadMemEdit.h"

void SlideFix::Initialize() {
	Sleep(5); //Give the game some time to start up
	SadMemEdit::PatchVMProtect();
	PatchProceduralAnimation();
}

void SlideFix::PatchProceduralAnimation() {
	/*
	* The basis of what we do, since what follows is mostly raw byte code, is simple.
	* When the procedural animation tries to move the slide, we cache the slide bone's address, and how much the 
	* procedural wanted to displace it. We then use the regular .MAB animation component to animate the slide.
	*/
	pSetProceduralBonePos = SadMemEdit::ByteScan({ 0x41, 0x0F, 0x29, 0x74, 0x04, 0x10 });
	pSetBonePos = SadMemEdit::ByteScan({ 0x0F, 0x29, 0x4C, 0x0B, 0x10, 0x48, 0x8B, 0x5c, 0x24, 0x58});

	DWORD64 pVirtualPage = SadMemEdit::AllocateMemory(512);
	int currentPosition = 0; //this is the "cursor" for writing byte code in pVirtualPage
	/*Just showing the pointers to "labels" that will be used*/
	memset(LPVOID(pVirtualPage + 0x180), 0x0, 4); //bProceduralMovementRequest
	memset(LPVOID(pVirtualPage + 0x184), 0x0, 8); //pProceduralBone1
	memset(LPVOID(pVirtualPage + 0x18C), 0x0, 8); //pProceduralBone2
	memset(LPVOID(pVirtualPage + 0x194), 0x0, 8); //fProceduralMovement
	/*
	* bProceduralMovementRequest: When the procedural animation is trying to move a slide, this is set 1. 
	* It is set back to zero when the regular animation component animates the slide.
	*
	* pProceduralBone1: The first register that forms a pointer to the address of the slide bone. 
	* 
	* pProceduralBone2: The second register, that is added to pProceduralBone1 to calculate the address of the slide bone.
	* 
	* fProceduralMovement: Float containing how much the procedural animation wants to displace the slide bone. 
	*/
	DWORD64 pSetProceduralBonePosDetour = pVirtualPage; //We will write the detour subproc at the start of our virtual page.

	currentPosition += SadMemEdit::WriteBytes({ 0x51 }, pVirtualPage);
	//push rcx
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x0D }, pVirtualPage + currentPosition);
	__int32 bProceduralMovementRequestOffset = (pVirtualPage + 0x180) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &bProceduralMovementRequestOffset, sizeof(bProceduralMovementRequestOffset));
	currentPosition +=  0x4;
	//lea rcx,[bProceduralMovementRequest]
	currentPosition += SadMemEdit::WriteBytes({ 0xC7, 0x01, 0x01, 0x0, 0x0, 0x0}, (pVirtualPage + currentPosition));
	//mov [rcx],1 #Request signal to animation component.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x0D }, (pVirtualPage + currentPosition));
	__int32 pProceduralBone1Offset = (pVirtualPage + 0x184) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &pProceduralBone1Offset, sizeof(pProceduralBone1Offset));
	currentPosition +=  0x4;
	//lea rcx,[pProceduralBone1]
	currentPosition += SadMemEdit::WriteBytes({ 0x4C, 0x89, 0x21 }, (pVirtualPage + currentPosition));
	//mov [rcx],r12 #Save the first register used to calculate the pointer to pProceduralBone.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x0D }, (pVirtualPage + currentPosition));
	__int32 pProceduralBone2Offset = (pVirtualPage + 0x18C) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &pProceduralBone2Offset, sizeof(pProceduralBone2Offset));
	currentPosition +=  0x4;
	//lea rcx,[pProceduralBone2]
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x89, 0x01 }, (pVirtualPage + currentPosition));
	//mov [rcx],rax #Save second register.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x0D }, (pVirtualPage + currentPosition));
	__int32 fProceduralMovementOffset = (pVirtualPage + 0x194) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &fProceduralMovementOffset, sizeof(fProceduralMovementOffset));
	currentPosition +=  0x4;
	//lea rcx,[fProceduralMovement]
	currentPosition += SadMemEdit::WriteBytes({ 0xF3, 0x0F, 0x11, 0x31 }, (pVirtualPage + currentPosition));
	//movss [rcx],xmm6 #Save the float value it wants to displace the slide by.
	currentPosition += SadMemEdit::WriteBytes({ 0x59 }, (pVirtualPage + currentPosition));
	//pop rcx
	currentPosition += SadMemEdit::WriteBytes({ 0x42, 0x0F, 0x29, 0x74, 0x20, 0x10 }, (pVirtualPage + currentPosition));
	//movaps[rax + r12 + 10], xmm6 #regular execution, this is the instruction that attempts to move the slide bone. 
	//#Hence why we saved rax, r12, and xmm6 at pVirtualPage+180, +18C, and +194 respectivley.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8B, 0x87, 0xC8, 0x0, 0x0, 0x0 }, (pVirtualPage + currentPosition));
	//mov rax,[rdi+000000C8] #...
	currentPosition += SadMemEdit::WriteBytes({ 0x4C, 0x8B, 0x64, 0x24, 0x70 }, (pVirtualPage + currentPosition));
	//mov r12,[rsp+70] #...
	SadMemEdit::WriteFarJump((pVirtualPage + currentPosition), pSetProceduralBonePos+0x12, 0);
	//jmp [pSetProceduralBonePos] #Resume normal execution.
	currentPosition += 0x14;

	DWORD64 pSetBonePosDetour = (pVirtualPage + currentPosition);
	//We will write the subproc to detour the animation component right after the one that detours the procedural animation.

	currentPosition += SadMemEdit::WriteBytes({ 0x50 }, (pVirtualPage + currentPosition));
	//push rax
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x05 }, pVirtualPage + currentPosition);
	__int32 pProceduralBoneOffset2 = (pVirtualPage + 0x184) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &pProceduralBoneOffset2, sizeof(pProceduralBoneOffset2));
	currentPosition += 0x4;
	//lea rax,[pProceduralBone1]
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x39, 0x18 }, (pVirtualPage + currentPosition));
	//cmp [rax],rbx #Is the animation component's first pointer currently the same as the procedural animation's?
	//#If so, this implies the animation may be animating the slide right now.
	currentPosition += SadMemEdit::WriteBytes({ 0x0F, 0x84, 0x1E, 0x0, 0x0, 0x0 }, (pVirtualPage + currentPosition));
	//je otherChecks
	//Break:
	currentPosition += SadMemEdit::WriteBytes({ 0x58 }, (pVirtualPage + currentPosition));
	//pop rax
	currentPosition += SadMemEdit::WriteBytes({ 0x0F, 0x29, 0x4C, 0x19, 0x10 }, (pVirtualPage + currentPosition));
	//movaps [rcx+rbx+10],xmm1 #Regular execution.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8B, 0x5C, 0x24, 0x58 }, (pVirtualPage + currentPosition));
	//mov rbx,[rsp+58] #...
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8B, 0x74, 0x24, 0x60 }, (pVirtualPage + currentPosition));
	//mov rsi,[rsp+60] #...
	SadMemEdit::WriteFarJump((pVirtualPage + currentPosition), pSetBonePos + 0x0F, 0);
	currentPosition += 0xE;
	//jmp [pSetBonePos] #Resume normal execution.
	//OtherChecks:
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x05 }, pVirtualPage + currentPosition);
	__int32 pProceduralBone2Offset2 = (pVirtualPage + 0x18C) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &pProceduralBone2Offset2, sizeof(pProceduralBone2Offset2));
	currentPosition += 0x4;
	//lea rax,[pProceduralBone2] 
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x39, 0x08 }, (pVirtualPage + currentPosition));
	//cmp[rax], rcx #Is the animation component's second pointer currently the same as the procedural animation's?
	//#If so, this means the animation is indeed animating the slide right now.
	currentPosition += SadMemEdit::WriteBytes({ 0x75, 0xD6 }, (pVirtualPage + currentPosition));
	//jne [Break] 
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x05 }, pVirtualPage + currentPosition);
	__int32 bProceduralMovementRequestOffset2 = (pVirtualPage + 0x180) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &bProceduralMovementRequestOffset2, sizeof(bProceduralMovementRequestOffset2));
	currentPosition += 0x4;
	//lea rax,[bProceduralMovementRequest] 
	currentPosition += SadMemEdit::WriteBytes({ 0x83, 0x38, 0x01 }, (pVirtualPage + currentPosition));
	//cmp dword ptr [rax],01 #Does the procedural component want to move the slide this instant?
	//#If so, we will execute the funtionality of pSetProceduralBonePos using pSetBonePos.
	currentPosition += SadMemEdit::WriteBytes({ 0x75 ,0xCA }, (pVirtualPage + currentPosition));
	//jne [Break] 
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x05 }, (pVirtualPage + currentPosition));
	__int32 fProceduralMovementOffset2 = (pVirtualPage + 0x194) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &fProceduralMovementOffset2, sizeof(fProceduralMovementOffset2));
	currentPosition += 0x4;
	//lea rax,[fProceduralMovement] #load the procedural component's slide displacment value.
	currentPosition += SadMemEdit::WriteBytes({ 0xF3, 0x0F, 0x10, 0x08 }, (pVirtualPage + currentPosition));
	//movss xmm1, [rax]
	currentPosition += SadMemEdit::WriteBytes({  0xF3, 0x0F, 0x11, 0x4C, 0x19, 0x10 }, (pVirtualPage + currentPosition));
	//movss[rcx + rbx + 10], xmm1 #Move the slide using pSetBonePos.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8D, 0x05 }, pVirtualPage + currentPosition);
	__int32 bProceduralMovementRequestOffset3 = (pVirtualPage + 0x180) - (pVirtualPage + currentPosition + 0x4);
	memcpy(LPVOID(pVirtualPage + currentPosition), &bProceduralMovementRequestOffset3, sizeof(bProceduralMovementRequestOffset3));
	currentPosition += 0x4;
	//lea rax, [bProceduralMovementRequest]
	currentPosition += SadMemEdit::WriteBytes({0xC7, 0x0, 0x0, 0x0, 0x0, 0x0}, (pVirtualPage + currentPosition));
	 //mov [bProceduralMovementRequest],0 #Set [bProceduralMovementRequest] signal to zero because we completed the request.
	currentPosition += SadMemEdit::WriteBytes({ 0x58 } , (pVirtualPage + currentPosition));
	//pop rax
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8B, 0x5C, 0x24, 0x58 }, (pVirtualPage + currentPosition));
	//mov rbx,[rsp+58] #regular execution.
	currentPosition += SadMemEdit::WriteBytes({ 0x48, 0x8B, 0x74, 0x24, 0x60 }, (pVirtualPage + currentPosition));
	//mov rsi,[rsp+60] #...
	SadMemEdit::WriteFarJump((pVirtualPage + currentPosition), pSetBonePos + 0x0F, 0);
	//jmp [pSetProceduralBonePos] #Resume normal execution.

	//Write detours to pVirtualPage at original address locations.
	SadMemEdit::WriteFarJump(pSetProceduralBonePos, pSetProceduralBonePosDetour, 0x12); 
	SadMemEdit::WriteFarJump(pSetBonePos, pSetBonePosDetour, 0x0F);
}
