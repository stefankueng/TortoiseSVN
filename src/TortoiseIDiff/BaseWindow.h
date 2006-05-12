#pragma once
#include <string>

// helper class to load resource strings
class ResString
{
	enum { MAX_RESSTRING = 255 };
public:
	ResString (HINSTANCE hInst, int resId);
	operator TCHAR const * () const { return _buf; }
private:
	TCHAR _buf [MAX_RESSTRING + 1];
};


class CWindow
{
public:
	virtual bool RegisterWindow(UINT style, HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground, 
		LPCTSTR lpszMenuName, LPCTSTR lpszClassName, HICON hIconSm);
	virtual bool RegisterWindow(CONST WNDCLASSEX* wcx);

	// static message handler to put in WNDCLASSEX structure
	static LRESULT CALLBACK stWinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/**
	 * Sets the window title. 
	 */
	void SetWindowTitle(const std::wstring& sTitle) 
	{
		sWindowTitle = sTitle;
	};

	/**
	 * Sets the transparency of the window.
	 * \remark note that this also sets the WS_EX_LAYERED style!
	 */
	void SetTransparency(BYTE alpha, COLORREF color = 0xFF000000);

	virtual bool Create();
	virtual bool Create(DWORD dwStyles, HWND hParent = NULL, RECT* rect = NULL);

	//void MsgLoop();
	bool IsWindowClosed() { return bWindowClosed; };

	operator HWND() {return m_hwnd;}
protected:
	HINSTANCE hInstance;
	HWND m_hwnd;
	bool bWindowClosed;
	std::wstring sClassName;
	std::wstring sWindowTitle;

	//constructor 
	CWindow(HINSTANCE hInst, CONST WNDCLASSEX* wcx = NULL) : m_hwnd(NULL)
		, hInstance(NULL)
		, bWindowClosed(FALSE)
	{
		hInstance = hInst; 
		if (wcx != NULL)
			RegisterWindow(wcx);
	};

	// the real message handler
	virtual LRESULT CALLBACK WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	// returns a pointer the window (stored as the WindowLong)
	inline static CWindow * GetObjectFromWindow(HWND hWnd)
	{
		return (CWindow *)GetWindowLongPtr(hWnd, GWL_USERDATA);
	}
};

