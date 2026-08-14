#include "ScreenCapture.h"
#include <algorithm>

bool CaptureWindow( HWND hwnd, CapturedFrame& outFrame )
{
	if( !hwnd || !IsWindow( hwnd ) )
		return false;

	RECT rect;
	if( !GetClientRect( hwnd, &rect ) )
		return false;

	int width  = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	if( width <= 0 || height <= 0 )
		return false;

	HDC windowDC = GetDC( hwnd );
	if( !windowDC )
		return false;

	HDC memDC             = CreateCompatibleDC( windowDC );
	HBITMAP memBitmap     = CreateCompatibleBitmap( windowDC, width, height );
	HGDIOBJ previousObj   = SelectObject( memDC, memBitmap );

	// PW_RENDERFULLCONTENT (Windows 8.1+) es clave para capturar ventanas
	// que renderizan con DirectX/GPU (navegadores, apps modernas).
	BOOL captured = PrintWindow( hwnd, memDC, PW_RENDERFULLCONTENT );
	if( !captured )
	{
		// Fallback para ventanas antiguas que no soportan PrintWindow bien.
		captured = BitBlt( memDC, 0, 0, width, height, windowDC, 0, 0, SRCCOPY );
	}

	if( captured )
	{
		BITMAPINFOHEADER bmi = {};
		bmi.biSize        = sizeof( BITMAPINFOHEADER );
		bmi.biWidth       = width;
		bmi.biHeight      = -height; // negativo = top-down (evita voltear la imagen)
		bmi.biPlanes      = 1;
		bmi.biBitCount    = 32;
		bmi.biCompression = BI_RGB;

		outFrame.pixels.resize( static_cast<size_t>( width ) * height * 4 );

		int linesCopied = GetDIBits(
			memDC, memBitmap, 0, height,
			outFrame.pixels.data(),
			reinterpret_cast<BITMAPINFO*>( &bmi ),
			DIB_RGB_COLORS );

		if( linesCopied > 0 )
		{
			outFrame.width  = width;
			outFrame.height = height;

			// GDI entrega BGRA; OpenGL espera RGBA.
			for( size_t i = 0; i + 3 < outFrame.pixels.size(); i += 4 )
				std::swap( outFrame.pixels[ i ], outFrame.pixels[ i + 2 ] );
		}
		else
		{
			captured = FALSE;
		}
	}

	SelectObject( memDC, previousObj );
	DeleteObject( memBitmap );
	DeleteDC( memDC );
	ReleaseDC( hwnd, windowDC );

	return captured != FALSE;
}
