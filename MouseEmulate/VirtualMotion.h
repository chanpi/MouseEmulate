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

	// ���z�L�[������
	__declspec(dllexport) UINT VMVirtualKeyDown(WORD wVirtualKey);
	__declspec(dllexport) UINT VMVirtualKeyUp(WORD wVirtualKey);
	
	// �}�E�X�h���b�O
	__declspec(dllexport) BOOL VMMouseDrag(const VMMouseMessage* mouseMessage);

#ifdef __cplusplus
}
#endif

#endif /* _VIRTUAL_MOTION_ */