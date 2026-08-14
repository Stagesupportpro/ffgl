#include "WindowEnumerator.h"

namespace
{
	BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam )
	{
		auto* list = reinterpret_cast<std::vector<WindowInfo>*>( lParam );

		if( !IsWindowVisible( hwnd ) )
			return TRUE;

		// El plugin corre dentro del proceso de Resolume: si no excluimos sus
		// propias ventanas, seleccionar "Resolume Arena" en la lista crearía
		// un bucle de feedback (la ventana capturándose a sí misma).
		DWORD windowProcessId = 0;
		GetWindowThreadProcessId( hwnd, &windowProcessId );
		if( windowProcessId == GetCurrentProcessId() )
			return TRUE;

		// Ignora ventanas "herramienta" (no aparecen en la barra de tareas:
		// tooltips, paneles flotantes, etc.)
		LONG_PTR exStyle = GetWindowLongPtrW( hwnd, GWL_EXSTYLE );
		if( exStyle & WS_EX_TOOLWINDOW )
			return TRUE;

		int len = GetWindowTextLengthW( hwnd );
		if( len == 0 )
			return TRUE;

		std::wstring title( static_cast<size_t>( len ), L'\0' );
		GetWindowTextW( hwnd, &title[ 0 ], len + 1 );

		if( title.empty() )
			return TRUE;

		// Salta ventanas minimizadas del todo a 0x0 (algunas apps las dejan vivas)
		RECT rect;
		if( GetClientRect( hwnd, &rect ) )
		{
			if( ( rect.right - rect.left ) <= 0 || ( rect.bottom - rect.top ) <= 0 )
				return TRUE;
		}

		list->push_back( { hwnd, title } );
		return TRUE;
	}
}

std::vector<WindowInfo> EnumerateCapturableWindows()
{
	std::vector<WindowInfo> windows;
	EnumWindows( EnumWindowsProc, reinterpret_cast<LPARAM>( &windows ) );
	return windows;
}

const WindowInfo* FindWindowInfo( const std::vector<WindowInfo>& windows, HWND hwnd )
{
	for( const auto& w : windows )
	{
		if( w.hwnd == hwnd )
			return &w;
	}
	return nullptr;
}
