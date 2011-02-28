#ifndef _VIRTUAL_MOTION_
#define _VIRTUAL_MOTION_

typedef enum {
	LButtonDrag,
	MButtonDrag,
	RButtonDrag,
} VMDragButton;

typedef struct {
	HWND hTargetWnd;
	VMDragButton dragButton;
	UINT uKeyState;
	POINT dragStartPos;
	POINT dragEndPos;
} VMMouseMessage;


#ifdef __cplusplus
extern "C" {
#endif

	// 仮想キーを押す
	__declspec(dllexport) UINT VMVirtualKeyDown(WORD wVirtualKey);
	__declspec(dllexport) UINT VMVirtualKeyUp(WORD wVirtualKey);
	
	// マウスドラッグ
	__declspec(dllexport) BOOL VMMouseDrag(const VMMouseMessage* mouseMessage);

#ifdef __cplusplus
}
#endif

#endif /* _VIRTUAL_MOTION_ */