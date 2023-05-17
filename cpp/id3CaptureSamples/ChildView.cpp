
// ChildView.cpp : CChildView class implementation
//

#include "pch.h"
#include "framework.h"
#include "id3CaptureSamples.h"
#include "ChildView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


std::string format(const char *format, ...)
{
    va_list	list;
    std::string str(256, '\0');
    va_start(list, format);
    int result = vsnprintf_s((char *)str.data(), str.capacity(), 256, format, list);
    if (result < (int)str.capacity())
        str.resize(result);
    va_end(list);
    return str;
}

std::wstring wformat(const wchar_t *format, ...)
{
    va_list	list;
    std::wstring wstr(256, '\0');
    va_start(list, format);
    int result = _vsnwprintf_s((wchar_t *)wstr.data(), wstr.capacity(), 256, format, list);
    if (result < (int)wstr.capacity())
        wstr.resize(result);
    va_end(list);
    return wstr;
}

std::wstring getCameraName(int32_t device_id)
{
    ID3_CAMERA_STRING_WIDE hStringWide{};
    id3CameraStringWide_Initialize(&hStringWide);
    id3CameraManager_GetDeviceName(device_id, hStringWide);
    int count = id3CameraStringWide_GetLength(hStringWide);
    std::wstring wstr(count, '\0');
    id3CameraStringWide_CopyToString(hStringWide, (wchar_t *)wstr.data());
    id3CameraStringWide_Dispose(hStringWide);
    return wstr;
}

int findNearestResolution(int32_t device_id, int width, int height, float fps, std::wstring &wstr)
{
    ID3_CAMERA_RESOLUTION_LIST hResolutionList{};
    id3CameraResolutionList_Initialize(&hResolutionList);
    id3CameraManager_GetDeviceResolutionList(device_id, hResolutionList);
    int result = id3CameraManager_FindNearestResolution(hResolutionList, width, height, fps);
    if (result >= 0)
    {
        ID3_CAMERA_RESOLUTION hCameraResolution{};
        id3CameraResolution_Initialize(&hCameraResolution, true);
        id3CameraResolutionList_Get(hResolutionList, result, hCameraResolution);

        id3CameraResolution_GetValue(hCameraResolution, &result);

        id3CameraResolution_GetWidth(hCameraResolution, &width);
        id3CameraResolution_GetHeight(hCameraResolution, &height);
        id3CameraResolution_GetFramerate(hCameraResolution, &fps);
        wstr = wformat(L"%d x %d @ %4.2f", width, height, fps);

        id3CameraResolution_Dispose(hCameraResolution);
    }
    id3CameraResolutionList_Dispose(hResolutionList);
    return result;
}

// C Camera Event Handler
void CameraManager_PlugAndPlayEventHandler(void* context, id3CameraPlugAndPlayEventType type, int32_t device_id)
{
    ((CChildView *)context)->PlugAndPlayEventHandler(type, device_id);
}

void CameraManager_DeviceEventHandler(void* context, id3CameraDeviceEventType type, int32_t device_id)
{
    ((CChildView *)context)->DeviceEventHandler(type, device_id);
}

void CameraManager_DeviceSelectedEventHandler(void* context, int camera_slot, int32_t device_id)
{
    ((CChildView *)context)->DeviceSelectedEventHandler(camera_slot, device_id);
}

void CameraManager_ImageCapturedEventHandler(void* context, int32_t device_id)
{
    ((CChildView *)context)->ImageCapturedEventHandler(device_id);
}

// CChildView

CChildView::CChildView()
{
    m_statusBar = nullptr;
    m_cameraSlot = 0;
    hCurrentPicture = nullptr;
}

CChildView::~CChildView()
{
}

// C++ Camera Event Handler
void CChildView::CameraAdded(int32_t device_id)
{
    auto camera_name = getCameraName(device_id);
    std::wstring msg = L"Found camera " + camera_name;
    m_statusBar->SetPaneText(0, msg.c_str());
    // Auto select first camera
    int currentDeviceId = 0;
    id3CameraManager_GetSelectedDevice(m_cameraSlot, &currentDeviceId);
    if (currentDeviceId == 0)
    {
        id3CameraManager_SelectDevice(m_cameraSlot, device_id);
    }
}

void CChildView::CameraRemoved(int32_t device_id)
{
    int currentDeviceId = 0;
    id3CameraManager_GetSelectedDevice(m_cameraSlot, &currentDeviceId);
    if (device_id == currentDeviceId)
    {
        auto camera_name = getCameraName(device_id);
        std::wstring msg = L"Lost camera " + camera_name;
        m_statusBar->SetPaneText(0, msg.c_str());
    }
}

void CChildView::PlugAndPlayEventHandler(id3CameraPlugAndPlayEventType type, int32_t device_id)
{
    switch (type)
    {
    case CameraPlugAndPlayEventType_CameraAdded: CameraAdded(device_id); break;
    case CameraPlugAndPlayEventType_CameraRemoved: CameraRemoved(device_id); break;
    }
}

void CChildView::DeviceEventHandler(id3CameraDeviceEventType type, int32_t device_id)
{
    int currentDeviceId = 0;
    id3CameraManager_GetSelectedDevice(m_cameraSlot, &currentDeviceId);
    if (device_id == currentDeviceId)
    {
        if (type == CameraDeviceEventType_DeviceReady)
        {
            m_statusBar->SetPaneText(0, wformat(L"Start camera %d", device_id).c_str());
            id3CameraManager_StartCamera(device_id);
            Invalidate();
        }
        else if (type == CameraDeviceEventType_DeviceError)
        {
            m_statusBar->SetPaneText(0, wformat(L"camera %d error", device_id).c_str());
        }
    }
}

void CChildView::DeviceSelectedEventHandler(int camera_slot, int32_t device_id)
{
    if ((camera_slot == m_cameraSlot) && (device_id != 0))
    {
        // auto select 720p
        std::wstring wstr;
        int res = findNearestResolution(device_id, 1280, 720, 30, wstr);
        if (res < 0)
        {
            res = findNearestResolution(device_id, 640, 480, 20, wstr);
        }
        if (res >= 0)
        {
            m_statusBar->SetPaneText(0, wformat(L"Set camera resolution to %s", wstr.c_str()).c_str());
            id3CameraParameterData param{};
            param.Automatic = false;
            param.Value = res;
            id3CameraManager_SetDeviceParameter(device_id, "Resolution", &param);
        }
        else
        {
            m_statusBar->SetPaneText(0, L"Unable to found camera resolution.");
        }
    }
}

void CChildView::ImageCapturedEventHandler(int32_t device_id)
{
    int currentDeviceId = 0;
    id3CameraManager_GetSelectedDevice(m_cameraSlot, &currentDeviceId);
    if ((device_id == currentDeviceId) && IsWindowVisible())
    {
        ID3_CAMERA_MEASUREMENT_DATA hCameraMeasurementData{};
        id3CameraMeasurementData_Initialize(&hCameraMeasurementData);
        id3CameraManager_GetCameraMeasurementData(device_id, hCameraMeasurementData);
        float fps = 0;
        id3CameraMeasurementData_GetDataFloat(hCameraMeasurementData, "AcquireFrameRate", &fps);
        int frameCount = 0;
        id3CameraMeasurementData_GetDataInt(hCameraMeasurementData, "AcquireFrameCount", &frameCount);
        id3CameraMeasurementData_Dispose(hCameraMeasurementData);

        int sdk_err = id3CameraManager_GetCurrentFrame(device_id, hCurrentPicture);
        if (sdk_err  == 0)
        {
            int height = 0;
            id3Image_GetHeight(hCurrentPicture, &height);
            if (height > 0)
            {
                Invalidate(FALSE); // FALSE because we don't want to erase background
            }
        }

        m_statusBar->SetPaneText(0,wformat(L"F = %4.2f fps %08d", fps, frameCount).c_str());
    }
}

void CChildView::Initialize(CStatusBar *statusBar)
{
    m_statusBar = statusBar;

    id3Image_Initialize(&hCurrentPicture);

    int sdk_err;
    sdk_err = id3CameraManager_Initialize();
    sdk_err = id3CameraManager_SetParameterInt(CameraManagerParameter_InvokeLater, 1);

    sdk_err = id3CameraManager_SetPlugAndPlayEvent(CameraManager_PlugAndPlayEventHandler, this);
    sdk_err = id3CameraManager_SetDeviceEvent(CameraManager_DeviceEventHandler, this);
    sdk_err = id3CameraManager_SetDeviceSelectedEvent(CameraManager_DeviceSelectedEventHandler, this);
    sdk_err = id3CameraManager_SetImageCapturedEvent(CameraManager_ImageCapturedEventHandler, this);

    sdk_err = id3CameraManager_LoadPlugin("id3CameraWebcam");
    sdk_err = id3CameraManager_EnableDeviceModel("Webcam", true);
    sdk_err = id3CameraManager_Start();
    sdk_err = id3CameraManager_StartPlugAndPlay();

    m_statusBar->SetPaneText(0,L"Initialization done. camera plug & play process started.");
}

void CChildView::Dispose()
{
    id3CameraManager_Stop();
    id3CameraManager_Dispose();
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_ERASEBKGND()
    ON_WM_PAINT()
END_MESSAGE_MAP()


void CChildView::LoadImage(CString &sFilePath)
{
    int currentDeviceId = 0;
    id3CameraManager_GetSelectedDevice(m_cameraSlot, &currentDeviceId);
    if (currentDeviceId != 0)
    {
        MessageBoxA(nullptr, "Unable to load an image when a camera is in used", "LoadImage", MB_OK);
        return;
    }

    CImage fileImage;
    auto result = fileImage.Load(sFilePath);
    if (result == 0)
    {
        int sdk_err = id3CameraError_Base;
        int bpp = fileImage.GetBPP();
        if (bpp == 24)
        {
            int w = fileImage.GetWidth();
            int h = fileImage.GetHeight();
            int s = fileImage.GetPitch();
            // Check the stride because it may not be pixel-width aligned and may be negative (bottom-up DIB).
            if (s < 0) 
            {
                s = -s;
            }

            int dst_stride = 3 * w;
            m_pixels.clear();
            m_pixels.resize(dst_stride * h);
            for (int y = 0; y < h; y++)
            {
                auto px_addr = fileImage.GetPixelAddress(0, y);
                memcpy(&m_pixels[y * dst_stride], px_addr, s);
            }
            sdk_err = id3Image_FromValues(hCurrentPicture, w, h, id3ImagePixelFormat_Bgr_24bits, m_pixels.data());
        }
        if (sdk_err == id3CameraError_Success)
        {
            m_image.Destroy();
            Invalidate(TRUE);
        }
    }
}

// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    RECT rect{};
    GetClientRect(&rect);

	// Create a black background
	CBrush backBrush;
	backBrush.CreateSolidBrush(RGB(0,0,0));
	pDC->FillRect(&rect, &backBrush);

    return TRUE;
}

int frameCount = 1;
void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device background for painting

    int height = 0;
    id3Image_GetHeight(hCurrentPicture, &height);
    if (height > 0)
    {
        int width = 0;
        id3Image_GetWidth(hCurrentPicture, &width);
        if (m_image.IsNull())
        {
            m_image.Create(width, -height, 24); // -height because need top-down DIB 
        }
        unsigned char *pixels_src{};
        id3Image_GetPixels(hCurrentPicture, &pixels_src);
        int stride_src = 0;
        id3Image_GetStride(hCurrentPicture, &stride_src);
        int stride_dst = m_image.GetPitch();
        if (stride_dst == stride_src)
        {
            void *pixels_dst = m_image.GetBits();
            memcpy(pixels_dst, pixels_src, stride_src * height);
        }
        else
        {
            // need to change the stride
            for (int y=0; y < height; y++)
            {
                void *pixels_dst = m_image.GetPixelAddress(0, y);
                memcpy(pixels_dst, pixels_src, stride_src);
                pixels_src += stride_src;
            }
        }
    }

    if (!m_image.IsNull())
    {
        RECT rect{};
        GetClientRect(&rect);

        int img_width = m_image.GetWidth();
        int img_height = m_image.GetHeight();
        if (img_height > 0)
        {
            RECT destRect{};
            float r1 = (float)img_width / img_height;
            float r2 = (float)rect.right / rect.bottom;
            int w = rect.right;
            int h = rect.bottom;
            // vertical or horizontal center to maintain aspect ratio
            if (r1 > r2)
            {
                w = rect.right;
                h = (int)(w / r1);
            } else if (r1 < r2)
            {
                h = rect.bottom;
                w = (int)(r1 * h);
            }
            destRect.left = rect.left + (rect.right - w) / 2;
            destRect.top = rect.top + (rect.bottom - h) / 2;
            destRect.right = destRect.left + w;
            destRect.bottom = destRect.top + h;

            dc.SetStretchBltMode(HALFTONE);
            m_image.StretchBlt(dc.GetSafeHdc(), destRect, SRCCOPY);
        }
    }
	// Ne pas appeler CWnd::OnPaint() pour la peinture des messages
}
