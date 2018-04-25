#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0400

#include "UI.h"
#include "Constants.h"
#include "resource.h"
#include "PhotoshopHelpers.h"
#include <math.h>
#include <assert.h>
#include <commctrl.h>
#include <algorithm>
#include <windows.h>
using namespace std;

#include "PreviewRenderer.h"

struct UIData
{
    UIData();
    void PaintProxy(HWND hDlg);
    void UpdateDisplayAfterSettingsChange(HWND hDlg);
    void RedrawProxyItem(HWND hDlg);
    void PaintImage(HWND hDlg, const Image &image, const vector<uint32_t> &image_data);
    void SetPreviewPosition(HWND hDlg, int iX, int iY);

    PreviewRenderer *m_pFilter;
    bool bDraggingPreview;
    bool bDraggingSnapped;
    HCURSOR g_hCursorHand;
    HCURSOR g_hOldCursor;
    int iPreviewX, iPreviewY;
    int iDragAnchorX;
    int iDragAnchorY;
    int iDragOrigX;
    int iDragOrigY;
    bool iUIResult;
    string sExceptionMessage;
    int g_iFocusedEditControl;
};

UIData::UIData()
{
    m_pFilter = NULL;
    bDraggingPreview = false;
    bDraggingSnapped = false;
    g_hCursorHand = NULL;
    g_hOldCursor = NULL;
    iPreviewX = 0, iPreviewY = 0;
    iDragAnchorX = 0;
    iDragAnchorY = 0;
    iDragOrigX = 0;
    iDragOrigY = 0;
    iUIResult = false;
    sExceptionMessage.clear();
    g_iFocusedEditControl = -1;
}

void UIData::PaintImage(HWND hDlg, const Image &image, const vector<uint32_t> &image_data)
{
    RECT wRect;
    GetClientRect(hDlg, &wRect);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hDlg, &ps);	

    /* Paint the black frame. */
    FrameRect(hDC, &wRect, (HBRUSH) GetStockObject(BLACK_BRUSH));	
    InflateRect(&wRect, -1, -1);
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = image.width;
    bi.bmiHeader.biHeight = image.height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    /*
    * Render image.PreviewImage into hDC.  The draw area is wRect.  Zoom the display
    * by GetPreviewZoom().  The top-left source pixel to draw is iImageX/iImageY.
    *
    * StretchDIBits makes this a pain.  We can handle the scaling by scaling the source
    * dimensions, but then we won't know the exact bounds of what's being rendered if
    * it doesn't fill the area completely; we can calculate it, but the rounding isn't
    * clearly defined, so that's error-prone.
    *
    * If the zoom won't fill the area, then handle the scaling by adjusting wRect.  Otherwise,
    * handle it by adjusting iSourceWidth.
    */
    int iImageX = iPreviewX;
    int iImageY = iPreviewY;
    int iDestX = wRect.left;
    int iDestY = wRect.top;

    int iDestWidth = wRect.right - wRect.left;
    int iDestHeight = wRect.bottom - wRect.top;
    int iSourceImageWidth = image.width - iImageX;
    int iSourceImageHeight = image.height - iImageY;
    if(iPreviewX < 0)
    {
        iDestX += -iPreviewX;
        iImageX = 0;
        iDestWidth -= -iPreviewX;
        iSourceImageWidth -= -iPreviewX;
    }
    if(iPreviewY < 0)
    {
        iDestY += -iPreviewY;
        iImageY = 0;
        iDestHeight -= -iPreviewY;
        iSourceImageHeight -= -iPreviewY;
    }

    int iSourceWidth;
    if(iSourceImageWidth >= iDestWidth)
    {
        iSourceWidth = iDestWidth;
    }
    else
    {
        iSourceWidth = iSourceImageWidth;
        iDestWidth = iSourceImageWidth;
    }

    int iSourceHeight;
    if(iSourceImageHeight >= iDestHeight)
    {
        iSourceHeight = iDestHeight;
    }
    else
    {
        iSourceHeight = iSourceImageHeight;
        iDestHeight = iSourceImageHeight;
    }

    if(iDestWidth > 0 && iDestHeight > 0)
    {
        StretchDIBits(
            hDC,
            iDestX, iDestY,					// destination upper-left corner
            iDestWidth, iDestHeight,			// size of destination rectangle
            iImageX, iSourceHeight+1+iImageY,		// source upper-left corner
            iSourceWidth, -iSourceHeight,			// size of source rectangle
            image_data.data(),
            &bi, DIB_RGB_COLORS, SRCCOPY);
    }

    /* If we aren't filling the whole window, clear the unused area.  Don't just clear the
     * whole area before blitting; it'll cause flicker. */
    {
        RECT BottomEmptyRect(wRect);
        BottomEmptyRect.top = max(BottomEmptyRect.top, iDestY + iDestHeight);
        if(BottomEmptyRect.bottom > BottomEmptyRect.top)
            FillRect(hDC, &BottomEmptyRect, (HBRUSH) GetStockObject(BLACK_BRUSH));

        RECT RightEmptyRect(wRect);
        RightEmptyRect.left = max(RightEmptyRect.left, iDestX + iDestWidth);
        if(RightEmptyRect.right > RightEmptyRect.left)
            FillRect(hDC, &RightEmptyRect, (HBRUSH) GetStockObject(BLACK_BRUSH));	

        RECT LeftEmptyRect(wRect);
        LeftEmptyRect.right = min(LeftEmptyRect.right, iDestX);
        if(LeftEmptyRect.right > LeftEmptyRect.left)
            FillRect(hDC, &LeftEmptyRect, (HBRUSH) GetStockObject(BLACK_BRUSH));

        RECT TopEmptyRect(wRect);
        TopEmptyRect.bottom = min(TopEmptyRect.bottom , iDestY);
        if(TopEmptyRect.bottom > TopEmptyRect.top)
            FillRect(hDC, &TopEmptyRect, (HBRUSH) GetStockObject(BLACK_BRUSH));
    }

    EndPaint(hDlg, (LPPAINTSTRUCT) &ps);
}

void UIData::PaintProxy(HWND hDlg)
{
    /* If the mouse is down on the preview, or if the preview is still rendering, draw
     * the original image. */
    if(bDraggingPreview || m_pFilter->CurrentPreview->width == 0)
    {
        /* When we're dragging the image around, always draw the original image. */
        PaintImage(hDlg, *m_pFilter->SourceImage.get(), m_pFilter->SourceImage8BPP);
    }
    else if(m_pFilter->CurrentPreview->width != 0)
        PaintImage(hDlg, *m_pFilter->CurrentPreview.get(), m_pFilter->CurrentPreview8BPP);
}

void UIData::RedrawProxyItem(HWND hDlg)
{
    InvalidateRect(GetDlgItem(hDlg, ID_PROXY_ITEM), NULL, FALSE);
}

namespace
{
    void SetDlgItemFormat(HWND hDlg, int ID, const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        string s = StringUtil::vssprintf(fmt, va);
        va_end(va);

        SetDlgItemText(hDlg, ID, s.c_str());
    }

    float GetDlgItemFloat(HWND hDlg, int ID)
    {
        char buf[1024];
        GetDlgItemText(hDlg, ID, buf, sizeof(buf));

        float fRet = 0;
        sscanf(buf, "%f", &fRet);
        return fRet;
    }

    bool CoordInDlgItem(HWND hDlg, int ID, LPARAM lParam, bool bScreenCoords, int *iX = NULL, int *iY = NULL)
    {
        RECT wRect;
        if(bScreenCoords)
            GetWindowRect(GetDlgItem(hDlg, ID), &wRect);	
        else
            GetClientRect(GetDlgItem(hDlg, ID), &wRect);	

        short iXCoord = LOWORD(lParam);
        short iYCoord = HIWORD(lParam);
        if(iXCoord < wRect.left || iXCoord > wRect.right || iYCoord < wRect.top || iYCoord > wRect.bottom)
            return false;

        if(iX)
            *iX = iXCoord - wRect.left;
        if(iY)
            *iY = iYCoord - wRect.top;
        return true;
    }
}

void UIData::SetPreviewPosition(HWND hDlg, int iX, int iY)
{
    iPreviewX = iX;
    iPreviewY = iY;
}

void UIData::UpdateDisplayAfterSettingsChange(HWND hDlg)
{
    m_pFilter->UpdatePreview();
    RedrawProxyItem(hDlg);
}

namespace
{
    struct EditControlData
    {
        int iIDC;
        int offset;
        bool bIntegerControl;

        bool bClampBottom;
        float fBottom;

        bool bClampTop;
        float fTop;

        float fMouseWheelDelta;

        template<typename T>
        T *GetSetting(Mosaic::Options &options) const
        {
            uint8_t *raw = (uint8_t *) &options;
            raw += offset;
            return (T *) raw;
        }
    };

    EditControlData Controls[] =
    {
        { IDC_EDIT_BLOCK_SIZE,	offsetof(Mosaic::Options, block_size), true,	true, 2,	false, 0,	1 },
        { IDC_EDIT_ANGLE,	    offsetof(Mosaic::Options, angle), false,	false, 0,	false, 0,	1 },
        { IDC_EDIT_OFFSET_X,	offsetof(Mosaic::Options, offset_x), true,	false, 0,	false, 0,	1 },
        { IDC_EDIT_OFFSET_Y,	offsetof(Mosaic::Options, offset_y), true,	false, 0,	false, 0,	1 },
    };
    const int iNumControls = sizeof(Controls) / sizeof(*Controls);

    const EditControlData &GetControlDataFromID(int iIDC)
    {
        int iControl = -1;
        for(int i = 0; i < iNumControls; ++i)
        {
            if(Controls[i].iIDC == iIDC)
                return Controls[i];
        }
        throw runtime_error(StringUtil::ssprintf("Internal error: unknown IDC %i", iIDC).c_str());
    }

    void GetControlValueFromDialog(const EditControlData &control, HWND hDlg, int &iVal, float &fVal)
    {
        if(control.bIntegerControl)
            iVal = GetDlgItemInt(hDlg, control.iIDC, NULL, false);
        else
            fVal = GetDlgItemFloat(hDlg, control.iIDC);
    }

    void SaveControlValueToDialog(const EditControlData &control, HWND hDlg, int iVal, float fVal)
    {
        if(control.bIntegerControl)
            SetDlgItemFormat(hDlg, control.iIDC, "%i", iVal);
        else
            SetDlgItemFormat(hDlg, control.iIDC, "%.3g", fVal);
    }

    void SetDlgItems(HWND hDlg, Mosaic::Options &options)
    {
        for(const EditControlData &control: Controls)
        {
            if(control.bIntegerControl)
            {
                const int *p = control.GetSetting<int>(options);
                SaveControlValueToDialog(control, hDlg, *p, 0);
            }
            else
            {
                const float *p = control.GetSetting<float>(options);
                SaveControlValueToDialog(control, hDlg, 0, *p);
            }
        }
    }	

    void GetControlValue(const EditControlData &control, Mosaic::Options &options, int &iVal, float &fVal)
    {
        if(control.bIntegerControl)
            iVal = *control.GetSetting<int>(options);
        else
            fVal = *control.GetSetting<float>(options);
    }

    void SaveControlValue(const EditControlData &control, Mosaic::Options &options, int iVal, float fVal)
    {
        if(control.bIntegerControl)
            *control.GetSetting<int>(options) = iVal;
        else
            *control.GetSetting<float>(options) = fVal;
    }

    void ClampControlValue(const EditControlData &control, int &iVal, float &fVal)
    {
        if(control.bClampBottom)
        {
            if(control.bIntegerControl)
                iVal = max(iVal, (int) control.fBottom);
            else
                fVal = max(fVal, control.fBottom);
        }

        if(control.bClampTop)
        {
            if(control.bIntegerControl)
                iVal = min(iVal, (int) control.fTop);
            else
                fVal = min(fVal, control.fTop);
        }
    }

    WNDPROC hOldProxyWndProc;
    LRESULT ProxyWindowProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
    {
        UIData *pData = (UIData *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        switch(wMsg)
        {
        case WM_PAINT:
            pData->PaintProxy(hWnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        }
        return CallWindowProc(hOldProxyWndProc, hWnd, wMsg, wParam, lParam);
    }

    BOOL WINAPI UIProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
    {
        UIData *pData = (UIData *) GetWindowLongPtr(hDlg, GWLP_USERDATA);

        int item, cmd;
        switch(wMsg)
        {
        case WM_NCHITTEST:
        case WM_SETCURSOR:
        case WM_MOUSEFIRST:
            break;
        default:
            //printf("msg %x, %i %i\n", wMsg, wParam, lParam);
            break;
        }

        switch(wMsg)
        {
        case WM_DESTROY:
            DestroyCursor(pData->g_hCursorHand);
            return TRUE;
        case WM_INITDIALOG:
            pData = (UIData *) lParam;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            {
                HWND hItem = GetDlgItem(hDlg, ID_PROXY_ITEM);
                SetWindowLongPtr(hItem, GWLP_USERDATA, lParam);
                hOldProxyWndProc = (WNDPROC) GetWindowLongPtr(hItem, GWLP_WNDPROC);
                SetWindowLongPtr(hItem, GWLP_WNDPROC, (LONG_PTR) ProxyWindowProc);
            }

            pData->g_hCursorHand = LoadCursor(NULL, IDC_HAND); 

            SetFocus(GetDlgItem(hDlg, IDC_EDIT_BLOCK_SIZE));

            pData->SetPreviewPosition(hDlg, 0, 0);

            SetDlgItems(hDlg, pData->m_pFilter->CurrentSettings);
            pData->UpdateDisplayAfterSettingsChange(hDlg);
            return TRUE;

        case WM_COMMAND:
        {
            item = LOWORD (wParam);
            cmd = HIWORD (wParam);

            if(cmd == EN_SETFOCUS)
            {
                pData->g_iFocusedEditControl = LOWORD (wParam);
                return TRUE;
            }
            else if(cmd == EN_KILLFOCUS)
            {
                pData->g_iFocusedEditControl = -1;
                return TRUE;
            }

            if(cmd == EN_CHANGE)
            {
                const EditControlData &control = GetControlDataFromID(item);

                int iVal;
                float fVal;
                GetControlValueFromDialog(control, hDlg, iVal, fVal);
                ClampControlValue(control, iVal, fVal);
                SaveControlValue(control, pData->m_pFilter->CurrentSettings, iVal, fVal);
            }
            else if(cmd == BN_CLICKED)
            {
                switch(item)
                {
                case IDOK:
                case IDCANCEL:
                    if(item == IDOK)
                        pData->iUIResult = true;
                    else
                        pData->iUIResult = false;
                    EndDialog(hDlg, item);

                    break;
                }
            }

            pData->UpdateDisplayAfterSettingsChange(hDlg);

            return TRUE;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
        {
            if(wMsg == WM_LBUTTONDOWN || wMsg == WM_LBUTTONDBLCLK)
            {
                if(CoordInDlgItem(hDlg, ID_PROXY_ITEM, lParam, false))
                {
                    SetCapture(hDlg);
                    pData->g_hOldCursor = SetCursor(pData->g_hCursorHand);
                    pData->bDraggingPreview = true;
                    pData->bDraggingSnapped = false;
                    pData->iDragAnchorX = LOWORD(lParam);
                    pData->iDragAnchorY = HIWORD(lParam);
                    pData->iDragOrigX = pData->iPreviewX;
                    pData->iDragOrigY = pData->iPreviewY;

                    pData->RedrawProxyItem(hDlg);
                    return TRUE;
                }
            }

            if(wMsg == WM_LBUTTONUP && pData->bDraggingPreview)
            {
                ReleaseCapture();
                SetCursor(pData->g_hOldCursor);
                pData->bDraggingPreview = false;
                /* If dragging didn't snap, we didn't move the cursor, so don't spend time recalculating
                 * the preview; just redraw it. */
                if(pData->bDraggingSnapped)
                    pData->UpdateDisplayAfterSettingsChange(hDlg);
                else
                    pData->RedrawProxyItem(hDlg);
                pData->bDraggingSnapped = false;
            }

            return FALSE;
        }

        case WM_MOUSEMOVE:
        {
            if(pData->bDraggingPreview)
            {
                short iX = LOWORD(lParam);
                short iY = HIWORD(lParam);
                int iDistanceX = (pData->iDragAnchorX - iX);
                int iDistanceY = (pData->iDragAnchorY - iY);
                if(abs(iDistanceX) > 10 || abs(iDistanceY) > 10)
                    pData->bDraggingSnapped = true;

                if(pData->bDraggingSnapped)
                {
                    int iX = pData->iDragOrigX + iDistanceX;
                    int iY = pData->iDragOrigY + iDistanceY;
                    pData->SetPreviewPosition(hDlg, iX, iY);

                    pData->RedrawProxyItem(hDlg);
                }
                return TRUE;
            }
            return FALSE;
        }
        case WM_MOUSEWHEEL:
        {
            short iDelta = HIWORD(wParam);
            Mosaic::Options &options = pData->m_pFilter->CurrentSettings;

            if(pData->g_iFocusedEditControl != -1)
            {
                const EditControlData &control = GetControlDataFromID(pData->g_iFocusedEditControl);

                int iVal;
                float fVal;
                GetControlValue(control, options, iVal, fVal);

                float fChange = control.fMouseWheelDelta;
                if(iDelta < 0)
                    fChange *= -1.0f;
                if((GetKeyState(VK_LSHIFT) & 0x8000) || (GetKeyState(VK_RSHIFT) & 0x8000))
                    fChange *= 10.0f;
                fVal += fChange;
                iVal += int(fChange);

                ClampControlValue(control, iVal, fVal);
                SaveControlValue(control, options, iVal, fVal);

                SaveControlValueToDialog(control, hDlg, iVal, fVal);
                pData->UpdateDisplayAfterSettingsChange(hDlg);
            }

            return FALSE;
        }

        default:
            return FALSE;
        }
    }

    BOOL WINAPI UIProcExceptions(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
    {
        UIData *pData = (UIData *) GetWindowLongPtr(hDlg, GWLP_USERDATA);

        try
        {
            BOOL ret = UIProc(hDlg, wMsg, wParam, lParam);
            return ret;
        } catch(const exception &e) {
            // XXX: pData == NULL if too early
            pData->sExceptionMessage = e.what();
            EndDialog(hDlg, 0);
            pData->iUIResult = false;
        }
        return FALSE;
    }
}

bool DoUI(Mosaic::Options &options, shared_ptr<const Image> image)
{
    PreviewRenderer pr;
    pr.SetSourceImage(image);
    pr.CurrentSettings = options;

    UIData data;
    data.m_pFilter = &pr;

    PlatformData *pPlatform = (PlatformData *)(gFilterRecord->platformData);

    DialogBoxParam(g_hInstance, (LPSTR) "MOSAIX_ANISO_DIALOG", (HWND) pPlatform->hwnd, (DLGPROC) UIProcExceptions, (LPARAM) &data);

    if(!data.sExceptionMessage.empty())
        MessageBox((HWND) pPlatform->hwnd, data.sExceptionMessage.c_str(), plugInName, MB_OK);

    if(data.iUIResult)
        options = pr.CurrentSettings;

    return data.iUIResult;
}
