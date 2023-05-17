
// ChildView.h : interface de la classe CChildView
//


#pragma once
#include <id3CameraManager.h>
#include <string>
#include <vector>


// fenêtre de CChildView

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

// Attributs
public:

protected:
    CStatusBar *m_statusBar;
    int m_cameraSlot;
    CImage m_image;
    ID3_IMAGE hCurrentPicture;
    std::vector<uint8_t> m_pixels;

// Opérations
public:
    void Initialize(CStatusBar *statusBar);
    void Dispose();
    void CameraAdded(int32_t device_id);
    void CameraRemoved(int32_t device_id);
    void PlugAndPlayEventHandler(id3CameraPlugAndPlayEventType type, int32_t device_id);
    void DeviceEventHandler(id3CameraDeviceEventType type, int32_t device_id);
    void DeviceSelectedEventHandler(int camera_slot, int32_t device_id);
    void ImageCapturedEventHandler(int32_t device_id);

    void LoadImage(CString &sFilePath);

// Substitutions
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implémentation
public:
	virtual ~CChildView();

	// Fonctions générées de la table des messages
protected:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};

