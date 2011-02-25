#ifndef _VIRTUAL_MOTION_
#define _VIRTUAL_MOTION_

typedef struct {
	UINT uMouseMessage;
	UINT uKeyState;
	int x;
	int y;
} VMMouseMessage;

typedef struct {
	HWND hTargetWnd;
	VMMouseMessage **pMouseMessage;
	int nMessageCount;
} VMMouseOperation;

#ifdef __cplusplus
extern "C" {
#endif

	// âºëzÉLÅ[ÇâüÇ∑
	__declspec(dllexport) UINT VMVirtualKeyDown(WORD wVirtualKey);
	__declspec(dllexport) UINT VMVirtualKeyUp(WORD wVirtualKey);
	
	__declspec(dllexport) BOOL VMMouseButtonOperation(const VMMouseOperation* pMouseOperation);

#ifdef __cplusplus
}
#endif

#endif /* _VIRTUAL_MOTION_ */