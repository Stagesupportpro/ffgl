#pragma once

// WindowCaptureSource: plugin FFGL de tipo "Source" (sin entradas de vídeo)
// que expone como parámetro un desplegable con las ventanas abiertas del
// sistema, y renderiza el contenido capturado de la ventana elegida.
//
// IMPORTANTE — LÉEME PRIMERO:
// Este archivo asume la API de la rama "master" del SDK oficial
// https://github.com/resolume/ffgl (clase base CFFGLPlugin, definida en
// FFGLPluginSDK.h). Los nombres exactos de un par de métodos de
// registro/actualización de parámetros ("SetParamInfo" / "SetParamElements")
// han cambiado de firma entre snapshots del SDK a lo largo de los años.
// Si al compilar el linker o el compilador se quejan de esos dos métodos,
// abre FFGLPluginSDK.h de tu copia local del SDK y ajusta las llamadas
// marcadas con "// [SDK-CHECK]" más abajo — es la única parte de este
// proyecto que depende de una firma exacta que puede variar.
//
// El resto (captura de ventana, subida de textura, dibujo del quad) es
// OpenGL estándar y no debería necesitar cambios.

#include "FFGLPluginSDK.h"
#include "ScreenCapture.h"
#include "WindowEnumerator.h"

#include <chrono>
#include <vector>

class WindowCaptureSource : public CFFGLPlugin
{
public:
	WindowCaptureSource();
	~WindowCaptureSource() override;

	// Ciclo de vida FFGL
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

	// Parámetros
	FFResult SetFloatParameter( unsigned int index, float value ) override;
	float    GetFloatParameter( unsigned int index ) override;

	static FFResult __stdcall CreateInstance( CFFGLPluginInfo* pluginInfo, CFreeFrameGLPlugin** ppOutInstance );

private:
	enum ParamID : unsigned int
	{
		PARAM_WINDOW = 0,   // FF_TYPE_OPTION — desplegable de ventanas
		PARAM_REFRESH,      // FF_TYPE_EVENT/BOOLEAN — botón "refrescar lista"
		PARAM_MAX_FPS,      // FF_TYPE_STANDARD (0..1) — limita la frecuencia de captura
		PARAM_COUNT
	};

	// El parámetro FFGL estándar va de 0.0 a 1.0; lo mapeamos a un rango de
	// FPS de captura razonable (1–60) para no tener que capturar en cada
	// frame de render si no hace falta.
	static constexpr float kMinCaptureFps = 1.0f;
	static constexpr float kMaxCaptureFps = 60.0f;
	float ParamToFps( float param ) const { return kMinCaptureFps + param * ( kMaxCaptureFps - kMinCaptureFps ); }
	float FpsToParam( float fps ) const { return ( fps - kMinCaptureFps ) / ( kMaxCaptureFps - kMinCaptureFps ); }

	void RefreshWindowList();
	void UploadFrameToTexture( const CapturedFrame& frame );
	void DrawFullscreenQuad();
	void EnsureGLResourcesCreated();
	void ReleaseGLResources();

	std::vector<WindowInfo> m_windows;
	int                     m_selectedIndex = -1;
	std::chrono::steady_clock::time_point m_lastRefresh{};

	float m_maxCaptureFps = 30.0f;
	std::chrono::steady_clock::time_point m_lastCapture{};

	unsigned int m_textureId  = 0;
	int          m_texWidth   = 0;
	int          m_texHeight  = 0;

	unsigned int m_shaderProgram = 0;
	unsigned int m_vbo           = 0;
};
