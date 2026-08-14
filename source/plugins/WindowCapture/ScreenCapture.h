#pragma once

// ScreenCapture: vuelca el contenido de un HWND a un buffer RGBA en memoria.
//
// Usa PrintWindow(PW_RENDERFULLCONTENT) en vez de BitBlt puro porque funciona
// correctamente con ventanas compuestas por GPU (Chrome, Electron, apps con
// aceleración por hardware), que con BitBlt clásico salen en negro.
//
// Nota: si en el futuro quieres más rendimiento (captura por GPU sin copiar a
// RAM cada frame), la alternativa es la Windows.Graphics.Capture API (WinRT),
// bastante más compleja de integrar (requiere C++/WinRT). Este enfoque con
// GDI es el punto de partida razonable: simple, compatible con Windows 7+,
// y suficiente para la mayoría de fuentes en directo.

#include <Windows.h>
#include <cstdint>
#include <vector>

struct CapturedFrame
{
	std::vector<uint8_t> pixels; // RGBA, top-down, 4 bytes/píxel
	int width  = 0;
	int height = 0;
};

// Captura el contenido actual de la ventana. Devuelve false si la ventana
// ya no existe o no se pudo capturar.
bool CaptureWindow( HWND hwnd, CapturedFrame& outFrame );
