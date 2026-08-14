#include "WindowCaptureSource.h"

#include <GL/glew.h>
#include <cstdio>

namespace
{
	// Refresca la lista de ventanas como mucho cada 2 segundos, para no
	// llamar a EnumWindows en cada frame.
	constexpr auto kRefreshInterval = std::chrono::seconds( 2 );

	const char* kVertexShader = R"(
		#version 120
		attribute vec2 position;
		attribute vec2 texCoord;
		varying vec2 vTexCoord;
		void main()
		{
			vTexCoord = texCoord;
			gl_Position = vec4( position, 0.0, 1.0 );
		}
	)";

	const char* kFragmentShader = R"(
		#version 120
		varying vec2 vTexCoord;
		uniform sampler2D uTexture;
		void main()
		{
			gl_FragColor = texture2D( uTexture, vTexCoord );
		}
	)";

	unsigned int CompileShaderProgram()
	{
		auto compile = []( GLenum type, const char* src ) -> GLuint {
			GLuint shader = glCreateShader( type );
			glShaderSource( shader, 1, &src, nullptr );
			glCompileShader( shader );

			GLint ok = 0;
			glGetShaderiv( shader, GL_COMPILE_STATUS, &ok );
			if( !ok )
			{
				char log[ 1024 ];
				glGetShaderInfoLog( shader, sizeof( log ), nullptr, log );
				FFGLLog::LogToHost( log );
				glDeleteShader( shader );
				return 0;
			}
			return shader;
		};

		GLuint vs = compile( GL_VERTEX_SHADER, kVertexShader );
		GLuint fs = compile( GL_FRAGMENT_SHADER, kFragmentShader );
		if( !vs || !fs )
			return 0;

		GLuint program = glCreateProgram();
		glAttachShader( program, vs );
		glAttachShader( program, fs );
		glBindAttribLocation( program, 0, "position" );
		glBindAttribLocation( program, 1, "texCoord" );
		glLinkProgram( program );

		glDeleteShader( vs );
		glDeleteShader( fs );

		GLint linked = 0;
		glGetProgramiv( program, GL_LINK_STATUS, &linked );
		if( !linked )
		{
			glDeleteProgram( program );
			return 0;
		}
		return program;
	}
}

WindowCaptureSource::WindowCaptureSource()
{
	SetMinInputs( 0 );
	SetMaxInputs( 0 );

	// [SDK-CHECK] Registro de parámetros. En SDKs recientes esto se hace con
	// SetParamInfo(id, "nombre", tipo, valorPorDefecto) y, para opciones
	// dinámicas, un SetParamElements(id, listaDeNombres) posterior (soportado
	// desde Resolume 7.4.1 según el SDK — ver issue #63 del repo resolume/ffgl).
	SetParamInfo( PARAM_WINDOW, "Window", FF_TYPE_OPTION, 0.0f );
	SetParamInfo( PARAM_REFRESH, "Refresh List", FF_TYPE_EVENT, 0.0f );
	SetParamInfo( PARAM_MAX_FPS, "Max Capture FPS", FF_TYPE_STANDARD, FpsToParam( m_maxCaptureFps ) );

	RefreshWindowList();
}

WindowCaptureSource::~WindowCaptureSource()
{
}

FFResult __stdcall WindowCaptureSource::CreateInstance( CFFGLPluginInfo* /*pluginInfo*/, CFreeFrameGLPlugin** ppOutInstance )
{
	*ppOutInstance = new WindowCaptureSource();
	return *ppOutInstance ? FF_SUCCESS : FF_FAIL;
}

void WindowCaptureSource::RefreshWindowList()
{
	m_windows = EnumerateCapturableWindows();

	std::vector<std::string> names;
	names.reserve( m_windows.size() );
	for( const auto& w : m_windows )
	{
		// Conversión simple wide -> utf8 para el nombre mostrado en Resolume.
		int len = WideCharToMultiByte( CP_UTF8, 0, w.title.c_str(), -1, nullptr, 0, nullptr, nullptr );
		std::string utf8( len > 0 ? len - 1 : 0, '\0' );
		if( len > 0 )
			WideCharToMultiByte( CP_UTF8, 0, w.title.c_str(), -1, &utf8[ 0 ], len, nullptr, nullptr );
		names.push_back( utf8 );
	}

	// [SDK-CHECK] Nombre/firma exacta de la función que actualiza los
	// elementos de un parámetro FF_TYPE_OPTION en caliente. En el momento de
	// escribir esto el mantenedor del SDK la llama SetParamElements(id, elementos)
	// (ver github.com/resolume/ffgl issue #63). Ajusta si tu snapshot difiere.
	SetParamElements( PARAM_WINDOW, names );

	if( m_selectedIndex < 0 && !m_windows.empty() )
		m_selectedIndex = 0;

	m_lastRefresh = std::chrono::steady_clock::now();
}

void WindowCaptureSource::EnsureGLResourcesCreated()
{
	if( m_textureId == 0 )
	{
		glGenTextures( 1, &m_textureId );
		glBindTexture( GL_TEXTURE_2D, m_textureId );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	if( m_shaderProgram == 0 )
		m_shaderProgram = CompileShaderProgram();

	if( m_vbo == 0 )
	{
		// Quad a pantalla completa en NDC + coords de textura.
		// x, y, u, v
		const float verts[] = {
			-1.f, -1.f, 0.f, 1.f,
			 1.f, -1.f, 1.f, 1.f,
			-1.f,  1.f, 0.f, 0.f,
			 1.f,  1.f, 1.f, 0.f,
		};
		glGenBuffers( 1, &m_vbo );
		glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
		glBufferData( GL_ARRAY_BUFFER, sizeof( verts ), verts, GL_STATIC_DRAW );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}
}

void WindowCaptureSource::ReleaseGLResources()
{
	if( m_textureId )
	{
		glDeleteTextures( 1, &m_textureId );
		m_textureId = 0;
	}
	if( m_shaderProgram )
	{
		glDeleteProgram( m_shaderProgram );
		m_shaderProgram = 0;
	}
	if( m_vbo )
	{
		glDeleteBuffers( 1, &m_vbo );
		m_vbo = 0;
	}
}

void WindowCaptureSource::UploadFrameToTexture( const CapturedFrame& frame )
{
	if( frame.width <= 0 || frame.height <= 0 )
		return;

	glBindTexture( GL_TEXTURE_2D, m_textureId );

	if( frame.width != m_texWidth || frame.height != m_texHeight )
	{
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, frame.width, frame.height, 0,
		              GL_RGBA, GL_UNSIGNED_BYTE, frame.pixels.data() );
		m_texWidth  = frame.width;
		m_texHeight = frame.height;
	}
	else
	{
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height,
		                 GL_RGBA, GL_UNSIGNED_BYTE, frame.pixels.data() );
	}

	glBindTexture( GL_TEXTURE_2D, 0 );
}

void WindowCaptureSource::DrawFullscreenQuad()
{
	glUseProgram( m_shaderProgram );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_textureId );
	glUniform1i( glGetUniformLocation( m_shaderProgram, "uTexture" ), 0 );

	glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)0 );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), (void*)( 2 * sizeof( float ) ) );

	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	glDisableVertexAttribArray( 0 );
	glDisableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glUseProgram( 0 );
}

FFResult WindowCaptureSource::InitGL( const FFGLViewportStruct* vp )
{
	EnsureGLResourcesCreated();
	return CFFGLPlugin::InitGL( vp );
}

FFResult WindowCaptureSource::ProcessOpenGL( ProcessOpenGLStruct* /*pGL*/ )
{
	auto now = std::chrono::steady_clock::now();
	double minInterval = 1.0 / static_cast<double>( m_maxCaptureFps > 0.0f ? m_maxCaptureFps : 1.0f );
	bool dueForCapture = std::chrono::duration<double>( now - m_lastCapture ).count() >= minInterval;

	if( std::chrono::steady_clock::now() - m_lastRefresh > kRefreshInterval )
		RefreshWindowList();

	// Capturar cuesta (copia GDI + subida a textura), así que lo hacemos como
	// mucho a m_maxCaptureFps por segundo, no en cada frame de render de
	// Resolume — el quad se sigue dibujando cada frame con la última textura.
	if( dueForCapture && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>( m_windows.size() ) )
	{
		CapturedFrame frame;
		if( CaptureWindow( m_windows[ m_selectedIndex ].hwnd, frame ) )
			UploadFrameToTexture( frame );
		m_lastCapture = now;
	}

	DrawFullscreenQuad();
	return FF_SUCCESS;
}

FFResult WindowCaptureSource::DeInitGL()
{
	ReleaseGLResources();
	return FF_SUCCESS;
}

FFResult WindowCaptureSource::SetFloatParameter( unsigned int index, float value )
{
	switch( index )
	{
	case PARAM_WINDOW:
		m_selectedIndex = static_cast<int>( value );
		break;
	case PARAM_REFRESH:
		if( value > 0.5f )
			RefreshWindowList();
		break;
	case PARAM_MAX_FPS:
		m_maxCaptureFps = ParamToFps( value );
		break;
	default:
		return FF_FAIL;
	}
	return FF_SUCCESS;
}

float WindowCaptureSource::GetFloatParameter( unsigned int index )
{
	switch( index )
	{
	case PARAM_WINDOW:
		return static_cast<float>( m_selectedIndex );
	case PARAM_MAX_FPS:
		return FpsToParam( m_maxCaptureFps );
	default:
		return 0.0f;
	}
}
