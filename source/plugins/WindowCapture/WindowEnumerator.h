#pragma once

// WindowEnumerator: lista las ventanas "capturables" del escritorio de Windows.
// Se usa para rellenar dinámicamente el parámetro "Window" del plugin FFGL.

#include <Windows.h>
#include <string>
#include <vector>

struct WindowInfo
{
	HWND hwnd = nullptr;
	std::wstring title;
};

// Devuelve la lista actual de ventanas visibles con título, excluyendo
// ventanas de utilidad (barras de tareas, tooltips, etc.) y la propia
// ventana de Resolume si se detecta por título (opcional, ver .cpp).
std::vector<WindowInfo> EnumerateCapturableWindows();

// Busca por HWND dentro de una lista ya obtenida. Devuelve nullptr si no está.
const WindowInfo* FindWindowInfo( const std::vector<WindowInfo>& windows, HWND hwnd );
