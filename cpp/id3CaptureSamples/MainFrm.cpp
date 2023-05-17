
// MainFrm.cpp : implémentation de la classe CMainFrame
//

#include "pch.h"
#include "framework.h"
#include "id3CaptureSamples.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_INDICATOR_TEXT,
	ID_SEPARATOR,           // indicateur de la ligne d'état
	ID_INDICATOR_CAPS
};

// construction/destruction de CMainFrame

CMainFrame::CMainFrame() noexcept
{
	// TODO: ajoutez ici le code d'une initialisation de membre
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// créer une vue de manière qu'elle occupe la zone cliente du frame
	if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, nullptr))
	{
		TRACE0("Impossible de créer la fenêtre d'affichage\n");
		return -1;
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Impossible de créer la barre d'état\n");
		return -1;      // échec de la création
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
    m_wndStatusBar.SetPaneInfo(0, ID_INDICATOR_TEXT, SBPS_STRETCH , 0);

    m_wndView.Initialize(&m_wndStatusBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: changez ici la classe ou les styles Window en modifiant
	//  CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

void CMainFrame::DisposeCapture()
{
    m_wndView.Dispose();
}

void CMainFrame::LoadImage(CString &sFilePath)
{
    m_wndView.LoadImage(sFilePath);
}

// diagnostics de CMainFrame

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// gestionnaires de messages de CMainFrame

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// passe le focus à la fenêtre d'affichage
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// laisser la priorité à la vue pour cette commande
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// sinon, la gestion par défaut est utilisée
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

